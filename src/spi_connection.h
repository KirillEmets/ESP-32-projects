#pragma once

#include <stddef.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"

typedef spi_device_handle_t spi_device_t;

// Creates the single shared SPI master bus. Call once before adding devices.
esp_err_t spi_connection_init(gpio_num_t sclk, gpio_num_t mosi, gpio_num_t miso);

// Attaches a device with its own chip select pin to the bus (SPI mode 0).
esp_err_t spi_connection_add_device(gpio_num_t cs, uint32_t speed_hz, spi_device_t *out_device);

// Sends `reg` with bit 7 cleared (write), followed by `data`.
esp_err_t spi_connection_write_reg(spi_device_t device, uint8_t reg, const uint8_t *data, size_t len);

// Sends `reg` with bit 7 set (read), then clocks in `len` bytes.
esp_err_t spi_connection_read_reg(spi_device_t device, uint8_t reg, uint8_t *data, size_t len);
