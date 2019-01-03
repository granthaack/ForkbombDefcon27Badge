/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "esp_log.h"

#define ESP_INTR_FLAG_DEFAULT 0
#define GPIO_INTR_PIN 27
#define GPIO_INTR_PIN_SEL  (1ULL<<GPIO_INTR_PIN)

#define I2C_MASTER_SCL_IO 22               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 23               /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM 0 /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 100000        /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_LEN (2 * 255)             /*!< I2C slave tx buffer size */
#define I2C_MASTER_RX_BUF_LEN (2 * 255)              /*!< I2C slave rx buffer size */


static void gpio_task(void* arg)
{
    for(;;) {
        uint8_t buf[127];
        
        for(uint8_t i = 0; i < 127; i++) {
            i2c_cmd_handle_t cmd = i2c_cmd_link_create();
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (0x69 << 1) | 0, 1);
            i2c_master_write_byte(cmd, 0x80 + i, 1);
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (0x69 << 1) | 1, 1);
            i2c_master_read_byte(cmd, buf + i, 1);
            i2c_master_stop(cmd);     
            esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
            i2c_cmd_link_delete(cmd);
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
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode,
                              I2C_MASTER_TX_BUF_LEN,
                              I2C_MASTER_RX_BUF_LEN, 0);

    printf("Hello world!\n");


    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);
}
