#pragma once

#include "esp_err.h"
#include "hal/gpio_types.h"

// BMP280 over SPI (4-wire, mode 0)

#define BAROMETER_SPI_SPEED_HZ 1000000

typedef struct {
    float temperature_c;
    float pressure_hpa;
} barometer_data_t;

// Checks chip ID, loads calibration and starts continuous (normal mode) sampling.
esp_err_t barometer_init(gpio_num_t cs);

esp_err_t barometer_read(barometer_data_t *out);
