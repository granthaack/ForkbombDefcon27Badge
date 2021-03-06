
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "esp_log.h"

float get_point(float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y);

void set_point(float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y, float f);
void set_point_uint16(uint16_t *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y, float f);

void get_adjacents_1d(float *src, float *dest, uint8_t rows, uint8_t cols, int8_t x, int8_t y);

void get_adjacents_2d(float *src, float *dest, uint8_t rows, uint8_t cols, int8_t x, int8_t y);

float cubicInterpolate(float p[], float x);

float bicubicInterpolate(float p[], float x, float y);

void interpolate_image(float *src, uint8_t src_rows, uint8_t src_cols, float *dest, uint8_t dest_rows, uint8_t dest_cols);
void interpolate_image_uint16(float *src, uint8_t src_rows, uint8_t src_cols, uint16_t *dest, uint8_t dest_rows, uint16_t dest_cols);
