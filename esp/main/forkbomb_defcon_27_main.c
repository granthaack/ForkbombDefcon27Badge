#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "fi2c.h"
#include "fscreen.h"
#include "ftherm.h"

#define SCALE_FACTOR 30
#define THERM_RES 8

//I don't like that these are globals, can I make them not globals?
SemaphoreHandle_t xDrawSemaphore;
int16_t* therm_buf;

typedef struct lcd_struct{
  spi_device_handle_t screen_spi;
} xlcd_struct;

void vDrawFrame(void* pvParameters){
  //int64_t time = 0;
  //Init the SPI screen
  spi_device_handle_t screen_spi = lcd_spi_init();
  lcd_screen_init(screen_spi);
  //Allocate a DMA framebuffer
  uint16_t *fbuf = heap_caps_malloc(SCREEN_WIDTH*SCREEN_HEIGHT*sizeof(uint16_t), MALLOC_CAP_DMA);

  //FreeRTOS housekeeping
  xlcd_struct *lcd_params;
  lcd_params = ( xlcd_struct * ) pvParameters;

  while(true) {
    if(xSemaphoreTake(xDrawSemaphore, portMAX_DELAY)){
      //time = esp_timer_get_time();
      //Scale up the data from the thermal sensor into a frame buffer
      for(uint16_t y = 0; y < SCREEN_HEIGHT; y++){
        for(uint16_t x = 0; x < SCALE_FACTOR*THERM_RES; x++){
          fbuf[(y * SCREEN_WIDTH) + x] = therm_colors[therm_buf[((y/SCALE_FACTOR)*THERM_RES + (x/SCALE_FACTOR))]];
        }
        for(uint16_t b = SCALE_FACTOR*8; b < SCREEN_WIDTH; b++){
          fbuf[(y * SCREEN_WIDTH) + b] = 0x0;
        }
      }
      lcd_send_fbuff(screen_spi, fbuf);
      //time = esp_timer_get_time() - time;
      //printf("Sent frame in %lld microseconds\n", time);
    }
  }
}

void vGetTherm(void* pvParameters){
  init_i2c();
  therm_buf = malloc(sizeof(int16_t)* 64);
  while(true){
    //vTaskDelay(1); //Set framerate here
    therm_read_frame(therm_buf);
    xSemaphoreGive(xDrawSemaphore);
  }
}

void app_main()
{

    xDrawSemaphore = xSemaphoreCreateBinary();
    spi_device_handle_t screen_spi = NULL;

    xlcd_struct lcd_params = {screen_spi};

    xTaskCreate(vDrawFrame, "vDrawFrame", 2048, &lcd_params, 2, NULL);
    xTaskCreate(vGetTherm, "vGetTherm", 2048, NULL, 1, NULL);
}
/*
vGetStats(void *p){

}
*/
