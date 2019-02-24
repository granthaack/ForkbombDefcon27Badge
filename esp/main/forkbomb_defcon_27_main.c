#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "fi2c.h"
#include "fscreen.h"
#include "ftherm.h"

#define SCALE_FACTOR 30

static void lcd_loop(void* arg)
{   int16_t buf[64];
    while(true) {
      therm_read_frame(buf);
      printf("Room Temp: %i\n", therm_get_thermis_temp()/16);
      for(uint8_t i = 0; i < 64; i++){
        printf("%i\t", buf[i]/4);
        if(!((i + 1)%8)){
          printf("\n");
        }
      }
      printf("\n\n");
    }
    vTaskDelay(100);
}


void app_main()
{
    //Allocate two DMA framebuffers (active and inactive)
    uint16_t *fbuff = heap_caps_malloc(SCREEN_WIDTH*SCREEN_HEIGHT*2*sizeof(uint16_t), MALLOC_CAP_DMA);

    spi_device_handle_t screen_spi = lcd_spi_init();
    lcd_screen_init(screen_spi);
    init_i2c();

    //Fill the framebuffers with some data
    for(uint32_t i = 0; i < SCREEN_WIDTH*SCREEN_HEIGHT*2; i++){
      fbuff = esp_random();
    }

    xTaskCreate(lcd_loop, "lcd_loop", 2048, NULL, 10, NULL);
    //This is bad but I'm tired
    //while(1){
    //  lcd_send_fbuff(screen_spi, &fbuff);
    //  lcd_send_fbuff(screen_spi, &fbuff+(SCREEN_HEIGHT*SCREEN_WIDTH));
    //  }
}
