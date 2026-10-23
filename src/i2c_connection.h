#pragma once

#include <stddef.h>
#include <stdint.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

typedef i2c_master_dev_handle_t i2c_device_t;

// Creates the single shared I2C master bus. Call once before adding devices.
esp_err_t i2c_connection_init(gpio_num_t sda, gpio_num_t scl);

// Attaches a 7-bit address device to the bus.
esp_err_t i2c_connection_add_device(uint8_t address, uint32_t speed_hz, i2c_device_t *out_device);

// Plain write of a raw buffer.
esp_err_t i2c_connection_write(i2c_device_t device, const uint8_t *data, size_t len);

// Writes `reg` followed by `data` in a single transaction.
esp_err_t i2c_connection_write_reg(i2c_device_t device, uint8_t reg, const uint8_t *data, size_t len);

// Writes `reg`, then reads `len` bytes (repeated start).
esp_err_t i2c_connection_read_reg(i2c_device_t device, uint8_t reg, uint8_t *data, size_t len);
