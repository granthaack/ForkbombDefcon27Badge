#include "i2c.h"
#include "MLX90640_I2C_Driver.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
static const char* TAG = "Therm I2C";
#define LOG_LOCAL_LEVEL ESP_LOG_ERROR
#include "esp_log.h"
//Note: Thermal sensor register addresses and data are 16 bits!
//      This seems pretty nonstandard but it should work fine

//Init the Thermal Sensor
void MLX90640_I2CInit()
{
    init_i2c();
}

//Multibyte I2C read.
int MLX90640_I2CRead(uint8_t slave_addr, uint16_t start_addr, uint16_t len, uint16_t *data){
  //Init the i2c handle
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  //Queue the START command
  i2c_master_start(cmd);
  //Queue the device address shifted to the left, indicate we want to write (0), enable the ACK check
  i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
  //Queue the MSB of the register address we want, with an ACK check enabled
  i2c_master_write_byte(cmd, ((start_addr & 0xFF00) >> 8), ACK_CHECK_EN);
  //Queue the LSB of the register address we want, with an ACK check enabled
  i2c_master_write_byte(cmd, (start_addr & 0xFF), ACK_CHECK_EN);
  //Queue the repeated START command
  i2c_master_start(cmd);
  //Queue the device address shifted to the left, indicate we want to read (1), enable the ACK check
  i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
  //Perform all the setup I2C commands.
  //This has to be done before we start recieving data due to how the ESP32 works with the MLX90640
  ESP_LOGD(TAG, "cmd is 0x%x", (unsigned int)cmd);
  esp_err_t ret = i2c_master_cmd_begin((i2c_port_t)I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
  ESP_LOGD(TAG, "cmd is 0x%x", (unsigned int)cmd);
  ESP_LOGD(TAG, "I2C setup returned 0x%x", ret);
  //Make sure the setup happened properly
  if(ret != ESP_OK){
    //Return a -1 on error because that's how melexis wrote the driver >:(
    return -1;
  }
  //Clear the command handle
  i2c_cmd_link_delete(cmd);
  uint16_t i = 1;
  uint8_t msb = 0;
  uint8_t lsb = 0;
  //Begin repeated reads. Only go through this loop if reading more than two 16 bit words
  while(i < len){
    //Create a new command handle for the next round of I2C transactions
    cmd = i2c_cmd_link_create();
    //Queue reading the MSB , Send an ack
    i2c_master_read_byte(cmd, &msb, I2C_MASTER_ACK);
    //Queue reading the LSB, Send an ack
    i2c_master_read_byte(cmd, &lsb, I2C_MASTER_ACK);
    //Get the MSB and LSB
    ret = i2c_master_cmd_begin((i2c_port_t)I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    ESP_LOGD(TAG, "I2C repeated read %i returned 0x%x", i-1, ret);
    //Make sure it was successful
    if(ret != ESP_OK){return -1;}
    //Put the result into dest as a uint16_t
    data[i - 1] = (msb << 8) | lsb;
    i++;
    //Clear the command handle
    i2c_cmd_link_delete(cmd);
  }
  //Create a new command handle
  cmd = i2c_cmd_link_create();
  //Queue reading the MSB, send an ACK
  i2c_master_read_byte(cmd, &msb, I2C_MASTER_ACK);
  //Queue reading the LSB, send a NACK because we're done
  i2c_master_read_byte(cmd, &lsb, I2C_MASTER_NACK);
  //Queue the STOP command
  i2c_master_stop(cmd);
  //Perform the queued i2c commands
  ret = i2c_master_cmd_begin((i2c_port_t)I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
  ESP_LOGD(TAG, "I2C tail returned 0x%x", ret);
  //Write the result to dest as a uint16_t
  data[len - 1] = (msb << 8) | lsb;
  //Delete the i2c link
  i2c_cmd_link_delete(cmd);
  //Check for errors
  if(ret != ESP_OK){return -1;}
  //Return 0 to signal success
  return 0;
}

//Single Byte I2C Write
int MLX90640_I2CWrite(uint8_t slave_addr, uint16_t write_addr, uint16_t data){
  //Init the i2c handle
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  //Queue the START command
  i2c_master_start(cmd);
  //Queue the device address shifted to the left, indicate we want to write (0), enable the ACK check
  i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
  //Queue the MSB of the register address we want, with an ACK check enabled
  i2c_master_write_byte(cmd, ((write_addr & 0xFF00) >> 8), ACK_CHECK_EN);
  //Queue the LSB of the register address we want, with an ACK check enabled
  i2c_master_write_byte(cmd, (write_addr & 0xFF), ACK_CHECK_EN);
  //Queue the MSB of the data, with an ACK check enabled
  i2c_master_write_byte(cmd, ((data & 0xFF00) >> 8), ACK_CHECK_EN);
  //Queue the LSB of the data, with an ACK check enabled
  i2c_master_write_byte(cmd, (data & 0xFF), ACK_CHECK_EN);
  //Queue the STOP command
  i2c_master_stop(cmd);
  //Perform the queued i2c command
  esp_err_t ret = i2c_master_cmd_begin((i2c_port_t)I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
  //Delete the i2c link
  i2c_cmd_link_delete(cmd);
  //Check for errors
  if(ret != ESP_OK){return -1;}
  //Melexis recommends checking to make sure the right value got written
  uint16_t data_check;
  MLX90640_I2CRead(slave_addr, write_addr, 1, &data_check);
  if (data_check != data){return -2;}
  //Return 0 to signify success
  return 0;
}
