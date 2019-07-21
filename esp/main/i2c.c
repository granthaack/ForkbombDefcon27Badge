#include "i2c.h"

esp_err_t i2c_read_reg(uint8_t slave_addr, uint8_t slave_reg, uint8_t* dest){
  //Init the i2c handle
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  //Queue the START command
  i2c_master_start(cmd);
  //Queue the device address shifted to the left, indicate we want to write (0), enable the ACK check
  i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
  //Queue the register address we want, with an ACK check enabled
  i2c_master_write_byte(cmd, slave_reg, ACK_CHECK_EN);
  //Queue the repeated START command
  i2c_master_start(cmd);
  //Queue the device address shifted to the left, indicate we want to read (1), enable the ACK check
  i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
  //Queue reading the data off the bus into our buffer, enable ACK check
  i2c_master_read_byte(cmd, dest, ACK_CHECK_EN);
  //Queue the STOP command
  i2c_master_stop(cmd);
  //Perform the queued i2c command
  esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
  //Delete the i2c link
  i2c_cmd_link_delete(cmd);
  //Return the result of the transaction
  return ret;
}


void init_i2c(){
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
}
