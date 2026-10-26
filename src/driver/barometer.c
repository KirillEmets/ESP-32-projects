#include "driver/barometer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "spi_connection.h"

static const char *TAG = "BAROMETER";

#define REG_CALIB      0x88
#define REG_CHIP_ID    0xD0
#define REG_RESET      0xE0
#define REG_CTRL_MEAS  0xF4
#define REG_CONFIG     0xF5
#define REG_DATA       0xF7

#define CHIP_ID        0x58
#define RESET_CMD      0xB6

#define CTRL_MEAS_VALUE    ((0x1 << 5) | (0x3 << 2) | 0x3)  // temp x1, press x4, normal mode
#define CONFIG_VALUE       ((0x2 << 5) | (0x2 << 2))        // 125 ms standby, IIR filter x4

typedef struct {
    uint16_t t1;
    int16_t t2, t3;
    uint16_t p1;
    int16_t p2, p3, p4, p5, p6, p7, p8, p9;
} calibration_t;

static spi_device_t s_dev;
static calibration_t s_calib;

static uint16_t u16_le(const uint8_t *b) { return (uint16_t) (b[0] | (b[1] << 8)); }

static esp_err_t write_byte(uint8_t reg, uint8_t value) {
    return spi_connection_write_reg(s_dev, reg, &value, 1);
}

static esp_err_t load_calibration(void) {
    uint8_t b[24];
    esp_err_t err = spi_connection_read_reg(s_dev, REG_CALIB, b, sizeof(b));
    if (err != ESP_OK) {
        return err;
    }

    s_calib.t1 = u16_le(&b[0]);
    s_calib.t2 = (int16_t) u16_le(&b[2]);
    s_calib.t3 = (int16_t) u16_le(&b[4]);
    s_calib.p1 = u16_le(&b[6]);
    s_calib.p2 = (int16_t) u16_le(&b[8]);
    s_calib.p3 = (int16_t) u16_le(&b[10]);
    s_calib.p4 = (int16_t) u16_le(&b[12]);
    s_calib.p5 = (int16_t) u16_le(&b[14]);
    s_calib.p6 = (int16_t) u16_le(&b[16]);
    s_calib.p7 = (int16_t) u16_le(&b[18]);
    s_calib.p8 = (int16_t) u16_le(&b[20]);
    s_calib.p9 = (int16_t) u16_le(&b[22]);

    return ESP_OK;
}

esp_err_t barometer_init(gpio_num_t cs) {
    esp_err_t err = spi_connection_add_device(cs, BAROMETER_SPI_SPEED_HZ, &s_dev);
    if (err != ESP_OK) {
        return err;
    }

    uint8_t chip_id = 0;
    err = spi_connection_read_reg(s_dev, REG_CHIP_ID, &chip_id, 1);
    if (err != ESP_OK) {
        return err;
    }

    if (chip_id != CHIP_ID) {
        ESP_LOGE(TAG, "Unexpected chip ID 0x%02X (expected 0x%02X)", chip_id, CHIP_ID);
        return ESP_ERR_NOT_FOUND;
    }

    write_byte(REG_RESET, RESET_CMD);
    vTaskDelay(pdMS_TO_TICKS(10));

    err = load_calibration();
    if (err != ESP_OK) {
        return err;
    }

    write_byte(REG_CONFIG, CONFIG_VALUE);
    return write_byte(REG_CTRL_MEAS, CTRL_MEAS_VALUE);
}

// Compensation formulas from the Bosch BMP280 datasheet (integer versions)

static int32_t compensate_temperature(int32_t adc_t, int32_t *t_fine) {
    int32_t var1 = ((((adc_t >> 3) - ((int32_t) s_calib.t1 << 1))) * s_calib.t2) >> 11;
    int32_t var2 = (((((adc_t >> 4) - (int32_t) s_calib.t1) * ((adc_t >> 4) - (int32_t) s_calib.t1)) >> 12) * s_calib.
                    t3) >> 14;
    *t_fine = var1 + var2;
    return (*t_fine * 5 + 128) >> 8; // 0.01 degC
}

static uint32_t compensate_pressure(int32_t adc_p, int32_t t_fine) {
    int64_t var1 = (int64_t) t_fine - 128000;
    int64_t var2 = var1 * var1 * s_calib.p6;
    var2 += (var1 * s_calib.p5) << 17;
    var2 += (int64_t) s_calib.p4 << 35;
    var1 = ((var1 * var1 * s_calib.p3) >> 8) + ((var1 * s_calib.p2) << 12);
    var1 = ((((int64_t) 1 << 47) + var1) * s_calib.p1) >> 33;
    if (var1 == 0) {
        return 0;
    }

    int64_t p = 1048576 - adc_p;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = ((int64_t) s_calib.p9 * (p >> 13) * (p >> 13)) >> 25;
    var2 = ((int64_t) s_calib.p8 * p) >> 19;
    p = ((p + var1 + var2) >> 8) + ((int64_t) s_calib.p7 << 4);
    return (uint32_t) p; // Pa in Q24.8
}

esp_err_t barometer_read(barometer_data_t *out) {
    // Burst read keeps pressure and temperature from the same sample
    uint8_t raw[6];
    esp_err_t err = spi_connection_read_reg(s_dev, REG_DATA, raw, sizeof(raw));
    if (err != ESP_OK) {
        return err;
    }

    int32_t adc_p = (raw[0] << 12) | (raw[1] << 4) | (raw[2] >> 4);
    int32_t adc_t = (raw[3] << 12) | (raw[4] << 4) | (raw[5] >> 4);

    int32_t t_fine;
    out->temperature_c = compensate_temperature(adc_t, &t_fine) / 100.0f;
    out->pressure_hpa = compensate_pressure(adc_p, t_fine) / 256.0f / 100.0f;

    return ESP_OK;
}
