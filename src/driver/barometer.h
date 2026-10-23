#pragma once

#include "esp_err.h"

// BMP280 at I2C address 0x76

#define BAROMETER_I2C_ADDRESS 0x76

typedef struct {
    float temperature_c;
    float pressure_hpa;
} barometer_data_t;

// Checks chip ID, loads calibration and starts continuous (normal mode) sampling.
esp_err_t barometer_init(void);

esp_err_t barometer_read(barometer_data_t *out);
