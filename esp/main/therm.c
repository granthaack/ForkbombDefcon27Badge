#include "therm.h"

//The resolution of the thermal sensor array is 0.25C, so divide the raw data
//by 4 to get the temperature in celsius
void therm_read_frame(int16_t* buf){
  for(uint8_t i = 0; i < 127; i+=2) {
    //Low and high bytes of the pixel
    uint8_t temp_lo = 0, temp_hi = 0;
    //Read the first byte of the pixel
    i2c_read_reg(THERM_I2C_ADDR, THERM_START_PIXEL_ADDR + i, &temp_lo);
    //Read the second byte of the pixel
    i2c_read_reg(THERM_I2C_ADDR, THERM_START_PIXEL_ADDR + i + 1, &temp_hi);
    //Convert the two bytes into an 11 bit pixel value in celsius
    buf[i/2] = (((temp_hi & 0b111) << 8) | temp_lo);
    //If the sign bit is set, the pixel value is negative
    if((temp_hi & (1 << 4))){
      buf[i/2] = -buf[i/2];
    }
  }
}

void therm_read_frame_float(float* buf){
  for(uint8_t i = 0; i < 127; i+=2) {
    //Low and high bytes of the pixel
    uint8_t temp_lo = 0, temp_hi = 0;
    //Read the first byte of the pixel
    i2c_read_reg(THERM_I2C_ADDR, THERM_START_PIXEL_ADDR + i, &temp_lo);
    //Read the second byte of the pixel
    i2c_read_reg(THERM_I2C_ADDR, THERM_START_PIXEL_ADDR + i + 1, &temp_hi);
    //Convert the two bytes into an 11 bit pixel value in celsius
    buf[i/2] = (((temp_hi & 0b111) << 8) | temp_lo);
    //If the sign bit is set, the pixel value is negative
    if((temp_hi & (1 << 4))){
      buf[i/2] = -buf[i/2];
    }
  }
}

//The resolution of the thermistor is 0.0625, so divide the raw data by 16 to
//get the temperature in celsius
  int16_t therm_get_thermis_temp(){
    //Low and high bytes of the pixel
    uint8_t temp_lo = 0, temp_hi = 0;
    int16_t thermistor_temp = 0;
    //Read the first byte of the pixel
    i2c_read_reg(THERM_I2C_ADDR, THERM_THRMST_LO_ADDR, &temp_lo);
    //Read the second byte of the pixel
    i2c_read_reg(THERM_I2C_ADDR, THERM_THRMST_HI_ADDR, &temp_hi);
    //Convert the two bytes into an 11 bit pixel value
    //The register of the thermistor is 0.0625, so divide by 16
    thermistor_temp = (((temp_hi & 0b111) << 8) | temp_lo);
    //If the sign bit is set, the pixel value is negative
    if((temp_hi & (1 << 4))){
      thermistor_temp = -thermistor_temp;
    }
    return thermistor_temp;
  }
