#include "screen.h"

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


const uint16_t therm_colors[] = {0x480F,
0x400F,0x400F,0x400F,0x4010,0x3810,0x3810,0x3810,0x3810,0x3010,0x3010,
0x3010,0x2810,0x2810,0x2810,0x2810,0x2010,0x2010,0x2010,0x1810,0x1810,
0x1811,0x1811,0x1011,0x1011,0x1011,0x0811,0x0811,0x0811,0x0011,0x0011,
0x0011,0x0011,0x0011,0x0031,0x0031,0x0051,0x0072,0x0072,0x0092,0x00B2,
0x00B2,0x00D2,0x00F2,0x00F2,0x0112,0x0132,0x0152,0x0152,0x0172,0x0192,
0x0192,0x01B2,0x01D2,0x01F3,0x01F3,0x0213,0x0233,0x0253,0x0253,0x0273,
0x0293,0x02B3,0x02D3,0x02D3,0x02F3,0x0313,0x0333,0x0333,0x0353,0x0373,
0x0394,0x03B4,0x03D4,0x03D4,0x03F4,0x0414,0x0434,0x0454,0x0474,0x0474,
0x0494,0x04B4,0x04D4,0x04F4,0x0514,0x0534,0x0534,0x0554,0x0554,0x0574,
0x0574,0x0573,0x0573,0x0573,0x0572,0x0572,0x0572,0x0571,0x0591,0x0591,
0x0590,0x0590,0x058F,0x058F,0x058F,0x058E,0x05AE,0x05AE,0x05AD,0x05AD,
0x05AD,0x05AC,0x05AC,0x05AB,0x05CB,0x05CB,0x05CA,0x05CA,0x05CA,0x05C9,
0x05C9,0x05C8,0x05E8,0x05E8,0x05E7,0x05E7,0x05E6,0x05E6,0x05E6,0x05E5,
0x05E5,0x0604,0x0604,0x0604,0x0603,0x0603,0x0602,0x0602,0x0601,0x0621,
0x0621,0x0620,0x0620,0x0620,0x0620,0x0E20,0x0E20,0x0E40,0x1640,0x1640,
0x1E40,0x1E40,0x2640,0x2640,0x2E40,0x2E60,0x3660,0x3660,0x3E60,0x3E60,
0x3E60,0x4660,0x4660,0x4E60,0x4E80,0x5680,0x5680,0x5E80,0x5E80,0x6680,
0x6680,0x6E80,0x6EA0,0x76A0,0x76A0,0x7EA0,0x7EA0,0x86A0,0x86A0,0x8EA0,
0x8EC0,0x96C0,0x96C0,0x9EC0,0x9EC0,0xA6C0,0xAEC0,0xAEC0,0xB6E0,0xB6E0,
0xBEE0,0xBEE0,0xC6E0,0xC6E0,0xCEE0,0xCEE0,0xD6E0,0xD700,0xDF00,0xDEE0,
0xDEC0,0xDEA0,0xDE80,0xDE80,0xE660,0xE640,0xE620,0xE600,0xE5E0,0xE5C0,
0xE5A0,0xE580,0xE560,0xE540,0xE520,0xE500,0xE4E0,0xE4C0,0xE4A0,0xE480,
0xE460,0xEC40,0xEC20,0xEC00,0xEBE0,0xEBC0,0xEBA0,0xEB80,0xEB60,0xEB40,
0xEB20,0xEB00,0xEAE0,0xEAC0,0xEAA0,0xEA80,0xEA60,0xEA40,0xF220,0xF200,
0xF1E0,0xF1C0,0xF1A0,0xF180,0xF160,0xF140,0xF100,0xF0E0,0xF0C0,0xF0A0,
0xF080,0xF060,0xF040,0xF020,0xF800,};
