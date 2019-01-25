#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "fi2c.h"

static void gpio_task(void* arg)
{
    for(;;) {
        uint8_t buf[127];

        for(uint8_t i = 0; i < 127; i++) {
          read_i2c_reg(0x69, 0x80 + i, buf + i);
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
    init_i2c();
    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);
}
