#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "fi2c.h"
#include "fscreen.h"
#include "ftherm.h"

#define SCALE_FACTOR 30
#define THERM_RES 8

typedef struct lcd_struct{
  spi_device_handle_t screen_spi;
  uint16_t *fbuf;
  uint8_t test;
} xlcd_struct;

void lcd_loop(void* pvParameters){
  //Init the SPI screen
  spi_device_handle_t screen_spi = lcd_spi_init();
  lcd_screen_init(screen_spi);
  //Allocate a DMA framebuffer
  uint16_t *fbuf = heap_caps_malloc(SCREEN_WIDTH*SCREEN_HEIGHT*sizeof(uint16_t), MALLOC_CAP_DMA);

  //FreeRTOS housekeeping
  xlcd_struct *lcd_params;
  lcd_params = ( xlcd_struct * ) pvParameters;

  //Allocate a buffer for the thermal data
  int16_t therm_buf[64];
  while(true) {
    //Read a frame from the thermal sensor
    therm_read_frame(therm_buf);
    //Scale up the data from the thermal sensor into a frame buffer
    for(uint16_t y = 0; y < SCREEN_HEIGHT; y++){
      for(uint16_t x = 0; x < SCALE_FACTOR*THERM_RES; x++){
        fbuf[(y * SCREEN_WIDTH) + x] = therm_colors[therm_buf[((y/SCALE_FACTOR)*THERM_RES + (x/SCALE_FACTOR))]];
      }
      for(uint16_t b = SCALE_FACTOR*8; b < SCREEN_WIDTH; b++){
        fbuf[(y * SCREEN_WIDTH) + b] = 0x0;
      }
    }
    printf("Inside of function, test is %i\n", lcd_params->test);
    lcd_send_fbuff(screen_spi, fbuf);
  }
}


void app_main()
{
    init_i2c();
    uint16_t *fbuf = 123;
    spi_device_handle_t screen_spi = NULL;
    xlcd_struct lcd_params = {screen_spi, fbuf, 99};
    xlcd_struct *lcd_params_ptr = &lcd_params;
    printf("Outside of function, test is %i\n", lcd_params_ptr->test);
    xTaskCreate(lcd_loop, "lcd_loop", 2048, &lcd_params, 10, NULL);
}
