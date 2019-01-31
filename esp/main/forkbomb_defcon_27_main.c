#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "fi2c.h"
#include "fscreen.h"

#define SCALE_FACTOR 30

static void lcd_loop(void* arg)
{
    for(;;) {
        uint8_t buf[127];

        for(uint8_t i = 0; i < 127; i++) {
          i2c_read_reg(0x69, 0x80 + i, buf + i);
        }

        for(uint8_t i = 0; i < 8; i++) {
            for(uint8_t j = 0; j < 16; j += 2) {
                uint16_t temp = buf[i * 16 + j + 1];
                temp = (temp << 8) + buf[i * 16 + j];
                temp = temp / 4;
                if (temp > 65) {
                    printf("\033[22;47m");
                } else if (temp > 52) {
                    printf("\033[22;41m");
                } else if (temp > 39) {
                    printf("\033[22;43m");
                } else if (temp > 26) {
                    printf("\033[22;42m");
                } else if (temp > 13) {
                    printf("\033[22;44m");
                } else {
                    printf("\033[22;46m");
                }
                printf(" ");
            }
            printf("\033[22;0m\n");
        }
        printf("\n");
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}


void app_main()
{
    //Allocate two DMA framebuffers (active and inactive)
    uint16_t *fbuff = heap_caps_malloc(SCREEN_WIDTH*SCREEN_HEIGHT*2*sizeof(uint16_t), MALLOC_CAP_DMA);

    spi_device_handle_t screen_spi = lcd_spi_init();
    lcd_screen_init(screen_spi);
    init_i2c();

    //Fill the framebuffers with some data
    for(uint32_t i = 0; i < SCREEN_WIDTH*SCREEN_HEIGHT*2; i++){
      fbuff = esp_random();
    }

    xTaskCreate(lcd_loop, "lcd_loop", 2048, NULL, 10, NULL);
    //This is bad but I'm tired
    while(1){
      lcd_send_fbuff(screen_spi, &fbuff);
      lcd_send_fbuff(screen_spi, &fbuff+(SCREEN_HEIGHT*SCREEN_WIDTH));
    }
}
