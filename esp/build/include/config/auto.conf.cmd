deps_config := \
	/home/ghaack/esp/esp-idf/components/app_trace/Kconfig \
	/home/ghaack/esp/esp-idf/components/aws_iot/Kconfig \
	/home/ghaack/esp/esp-idf/components/bt/Kconfig \
	/home/ghaack/esp/esp-idf/components/driver/Kconfig \
	/home/ghaack/esp/esp-idf/components/esp32/Kconfig \
	/home/ghaack/esp/esp-idf/components/esp_adc_cal/Kconfig \
	/home/ghaack/esp/esp-idf/components/esp_event/Kconfig \
	/home/ghaack/esp/esp-idf/components/esp_http_client/Kconfig \
	/home/ghaack/esp/esp-idf/components/esp_http_server/Kconfig \
	/home/ghaack/esp/esp-idf/components/esp_https_ota/Kconfig \
	/home/ghaack/esp/esp-idf/components/ethernet/Kconfig \
	/home/ghaack/esp/esp-idf/components/fatfs/Kconfig \
	/home/ghaack/esp/esp-idf/components/freemodbus/Kconfig \
	/home/ghaack/esp/esp-idf/components/freertos/Kconfig \
	/home/ghaack/esp/esp-idf/components/heap/Kconfig \
	/home/ghaack/esp/esp-idf/components/libsodium/Kconfig \
	/home/ghaack/esp/esp-idf/components/log/Kconfig \
	/home/ghaack/esp/esp-idf/components/lwip/Kconfig \
	/home/ghaack/esp/esp-idf/components/mbedtls/Kconfig \
	/home/ghaack/esp/esp-idf/components/mdns/Kconfig \
	/home/ghaack/esp/esp-idf/components/mqtt/Kconfig \
	/home/ghaack/esp/esp-idf/components/nvs_flash/Kconfig \
	/home/ghaack/esp/esp-idf/components/openssl/Kconfig \
	/home/ghaack/esp/esp-idf/components/pthread/Kconfig \
	/home/ghaack/esp/esp-idf/components/spi_flash/Kconfig \
	/home/ghaack/esp/esp-idf/components/spiffs/Kconfig \
	/home/ghaack/esp/esp-idf/components/tcpip_adapter/Kconfig \
	/home/ghaack/esp/esp-idf/components/unity/Kconfig \
	/home/ghaack/esp/esp-idf/components/vfs/Kconfig \
	/home/ghaack/esp/esp-idf/components/wear_levelling/Kconfig \
	/home/ghaack/esp/esp-idf/components/app_update/Kconfig.projbuild \
	/home/ghaack/esp/esp-idf/components/bootloader/Kconfig.projbuild \
	/home/ghaack/esp/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/ghaack/projects/ForkbombDefcon27Badge/esp/main/Kconfig.projbuild \
	/home/ghaack/esp/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/ghaack/esp/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)

ifneq "$(IDF_TARGET)" "esp32"
include/config/auto.conf: FORCE
endif
ifneq "$(IDF_CMAKE)" "n"
include/config/auto.conf: FORCE
endif

$(deps_config): ;
