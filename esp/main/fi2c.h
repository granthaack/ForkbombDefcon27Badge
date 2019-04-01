#ifndef FI2C_H
#define FI2C_H

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
#define I2C_MASTER_SDA_IO 21               /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM 0                   /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 1000000         /*!< I2C master clock frequency. Set to 1MHz, ludicrous speed! */
#define I2C_MASTER_TX_BUF_LEN (2 * 255)    /*!< I2C slave tx buffer size */
#define I2C_MASTER_RX_BUF_LEN (2 * 255)    /*!< I2C slave rx buffer size */
#define ACK_CHECK_EN 0x1

//Read a single i2c register from a slave device
esp_err_t i2c_read_reg(uint8_t slave_addr, uint8_t slave_reg, uint8_t* dest);
//Initialize i2c with our defined constants
void init_i2c();

#endif
