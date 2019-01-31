#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

//Options from make menuconfig
#define EXAMPLE_WIFI_SSID RPI_WIFI_SSID
#define EXAMPLE_WIFI_PASS RPI_WIFI_PASSWORD
#define HOST_IP_ADDR RPI_HOST_IP
#define PORT TCP_PORT

const int IPV4_GOTIP_BIT = BIT0;

//Initialize the WiFi module with options set in make menuconfig
void wifi_init(void);
