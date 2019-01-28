#include "fscreen.h"

void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=8;                     //Command is 8 bits
    t.tx_buffer=&cmd;               //The data is the cmd itself
    t.user=(void*)0;                //D/C needs to be set to 0
    ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

void lcd_data(spi_device_handle_t spi, const uint8_t *data, uint32_t len)
{
    esp_err_t ret;
    spi_transaction_t t;
    if (len==0) return;             //no need to send anything
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=len*8;                 //Len is in bytes, transaction length is in bits.
    t.tx_buffer=data;               //Data
    t.user=(void*)1;                //D/C needs to be set to 1
    ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc=(int)t->user;
    gpio_set_level(SCREEN_PIN_DC, dc);
}

void lcd_screen_init(spi_device_handle_t spi)
{
    int cmd=0;
    //Initialize non-SPI GPIOs
    gpio_set_direction(SCREEN_PIN_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(SCREEN_PIN_RST, GPIO_MODE_OUTPUT);

    //Reset the display
    gpio_set_level(SCREEN_PIN_RST, 0);
    vTaskDelay(100 / portTICK_RATE_MS);
    gpio_set_level(SCREEN_PIN_RST, 1);
    vTaskDelay(100 / portTICK_RATE_MS);

    //Send all the commands
    while (ili_init_cmds[cmd].databytes!=0xff) {
        lcd_cmd(spi, ili_init_cmds[cmd].cmd);
        lcd_data(spi, ili_init_cmds[cmd].data, ili_init_cmds[cmd].databytes&0x1F);
        if (ili_init_cmds[cmd].databytes&0x80) {
            vTaskDelay(100 / portTICK_RATE_MS);
        }
        cmd++;
    }
}

spi_device_handle_t lcd_spi_init(){
    esp_err_t ret;
    spi_device_handle_t spi;
    spi_bus_config_t buscfg={
        .miso_io_num=SCREEN_PIN_MISO,
        .mosi_io_num=SCREEN_PIN_MOSI,
        .sclk_io_num=SCREEN_PIN_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=240*320*2+8
    };
    spi_device_interface_config_t devcfg={
        .clock_speed_hz=SPI_CLK,                //Clock out at 26 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=SCREEN_PIN_CS,            //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
        .pre_cb=lcd_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
    };
    //Initialize the SPI bus
    ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    ESP_ERROR_CHECK(ret);
    //Attach the LCD to the SPI bus
    ret=spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
    return spi;
}

/*  To send the frame buffer we have to send the following to the ILI9341:
    1) Column address set (command)
    2) Start column high/low bytes, end column high/low bytes (data)
    3) Page address set (command)
    4) Start page high/low bytes, end page high/low bytes (data)
    5) Memory write (command)
    6) The actual image data, length of the image data, and the flags (data)
    We can't put all of this in just one transaction because the D/C line needs to be toggled in the middle.)
    This routine queues these commands up as interrupt transactions so they get
    sent faster (compared to calling spi_device_transmit several times) and free
    the CPU to do other things
*/
void lcd_send_fbuff(spi_device_handle_t spi, uint16_t *fbuff){
    esp_err_t ret;
    uint8_t x;
    //Transaction descriptors, 6 of them. Declared static so they're not allocated on the stack; we need this memory even when this
    //function is finished because the SPI driver needs access to it even while we're already calculating the next line.
    static spi_transaction_t trans[6];

    //In theory, it's better to initialize trans and data only once and hang on to the initialized
    //variables. We allocate them on the stack, so we need to re-init them each call.
    for (x=0; x<6; x++) {
        memset(&trans[x], 0, sizeof(spi_transaction_t));
        if ((x&1)==0) {
            //Even transfers are commands
            trans[x].length=8;
            trans[x].user=(void*)0;
        } else {
            //Odd transfers are data
            trans[x].length=8*4;
            trans[x].user=(void*)1;
        }
        trans[x].flags=SPI_TRANS_USE_TXDATA;
    }
    trans[0].tx_data[0]=0x2A;                 //Column Address Set (ILI9341 command)

    trans[1].tx_data[0]=0;                    //Start Col High (ILI9341 data)
    trans[1].tx_data[1]=0;                    //Start Col Low (ILI9341 data)
    trans[1].tx_data[2]=(SCREEN_WIDTH)>>8;    //End Col High (ILI9341 data)
    trans[1].tx_data[3]=(SCREEN_WIDTH)&0xff;  //End Col Low (ILI9341 data)

    trans[2].tx_data[0]=0x2B;                 //Page address set (ILI9341 command)

    trans[3].tx_data[0]=0;                    //Start page high (ILI9341 data)
    trans[3].tx_data[1]=0;                    //start page low (ILI9341 data)
    trans[3].tx_data[2]=(SCREEN_HEIGHT)>>8;   //end page high (ILI9341 data)
    trans[3].tx_data[3]=(SCREEN_HEIGHT)&0xff; //end page low (ILI9341 data)

    trans[4].tx_data[0]=0x2C;                 //memory write (ILI9341 command)

    trans[5].tx_buffer=fbuff;                 //finally send the line data
    trans[5].length=2*8*SCREEN_HEIGHT*SCREEN_WIDTH;//Data length, in bits
    trans[5].flags=0;                         //undo SPI_TRANS_USE_TXDATA flag

    //Queue all transactions.
    for (x=0; x<6; x++) {
        ret=spi_device_queue_trans(spi, &trans[x], portMAX_DELAY);
        assert(ret==ESP_OK);
    }

    //When we are here, the SPI driver is busy (in the background) getting the transactions sent. That happens
    //mostly using DMA, so the CPU doesn't have much to do here. We're not going to wait for the transaction to
    //finish because we may as well spend the time calculating the next line. When that is done, we can call
    //send_line_finish, which will wait for the transfers to be done and check their status.
}
