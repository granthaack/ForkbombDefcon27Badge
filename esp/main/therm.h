#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "i2c.h"

#define THERM_I2C_ADDR            0x69
#define THERM_START_PIXEL_ADDR    0x80
#define THERM_THRMST_LO_ADDR      0x0e
#define THERM_THRMST_HI_ADDR      0x0f

//Read a frame from the thermal sensor into a buffer of 64 signed 16 bit ints
//Pixel conversion factor = 0.25
//Divide the raw data by 4 to get the temperature in Celsius
void therm_read_frame(int16_t* buf);
void therm_read_frame_float(float* buf);

//Get the value of the thermistor in the thermal sensor
//This gets RAW data, not temperature values in Celsius. Divide the raw data
//Thermistor conversion factor = 0.0625
//by 16 to get the temperature in Celsius
int16_t therm_get_thermis_temp();
//TODO: Get status reg of thermal sensor, clear status reg of thermal sensor
