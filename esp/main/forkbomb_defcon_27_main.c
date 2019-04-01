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

SemaphoreHandle_t xDrawMutex;
static int16_t* therm_buf;

typedef struct lcd_struct{
  spi_device_handle_t screen_spi;
} xlcd_struct;

void vDrawFrame(void* pvParameters){
  //Init the SPI screen
  spi_device_handle_t screen_spi = lcd_spi_init();
  lcd_screen_init(screen_spi);
  //Allocate a DMA framebuffer
  uint16_t *fbuf = heap_caps_malloc(SCREEN_WIDTH*SCREEN_HEIGHT*sizeof(uint16_t), MALLOC_CAP_DMA);
  //FreeRTOS housekeeping
  xlcd_struct *lcd_params;
  lcd_params = ( xlcd_struct * ) pvParameters;
  while(true) {
    if(xSemaphoreTake(xDrawMutex, portMAX_DELAY)){
      for(uint16_t y = 0; y < SCREEN_HEIGHT; y++){
        for(uint16_t x = 0; x < SCALE_FACTOR*THERM_RES; x++){
          fbuf[(y * SCREEN_WIDTH) + x] = therm_colors[therm_buf[((y/SCALE_FACTOR)*THERM_RES + (x/SCALE_FACTOR))]];
        }
        for(uint16_t b = SCALE_FACTOR*8; b < SCREEN_WIDTH; b++){
          fbuf[(y * SCREEN_WIDTH) + b] = 0x0;
        }
      }
      lcd_send_fbuff(screen_spi, fbuf);
      printf("fbuff sent\n");
      xSemaphoreGive(xDrawMutex);
    }
  }
}

void vGetTherm(void* pvParameters){
  init_i2c();
  therm_buf = malloc(sizeof(int16_t)* 64);
  while(true){
    if(xSemaphoreTake(xDrawMutex, portMAX_DELAY)){
      vTaskDelay(1); //Set framerate here
      therm_read_frame(therm_buf);
      printf("therm buff sent\n");
      xSemaphoreGive(xDrawMutex);
    }
  }
}

void app_main()
{
    xDrawMutex = xSemaphoreCreateMutex();
    spi_device_handle_t screen_spi = NULL;
    xlcd_struct lcd_params = {screen_spi};
    //These tasks need to have the same priority for the mutex to work
    xTaskCreate(vDrawFrame, "vDrawFrame", 2048, &lcd_params, 1, NULL);
    xTaskCreate(vGetTherm, "vGetTherm", 2048, NULL, 1, NULL);
}
/*
vGetStats(void *p){

}
*/
