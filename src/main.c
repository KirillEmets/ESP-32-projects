#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "i2c_connection.h"
#include "driver/barometer.h"
#include "driver/display.h"

static const char *TAG = "MAIN";

#define I2C_SDA GPIO_NUM_8
#define I2C_SCL GPIO_NUM_9

#define POLL_PERIOD_MS 500

void app_main(void)
{
    ESP_ERROR_CHECK(i2c_connection_init(I2C_SDA, I2C_SCL));
    ESP_ERROR_CHECK(display_init());

    esp_err_t baro_err = barometer_init();
    if (baro_err != ESP_OK) {
        ESP_LOGE(TAG, "Barometer init failed: %s", esp_err_to_name(baro_err));
        display_clear();
        display_draw_text(0, 0, "Barometer not found");
        display_update();
        return;
    }

    char line[32];
    barometer_data_t data;
    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        display_clear();
        display_draw_text(0, 0, "Weather station");

        if (barometer_read(&data) == ESP_OK) {
            snprintf(line, sizeof(line), "Temp: %.2f C", data.temperature_c);
            display_draw_text(2, 0, line);

            snprintf(line, sizeof(line), "Pres: %.2f hPa", data.pressure_hpa);
            display_draw_text(4, 0, line);

            ESP_LOGI(TAG, "T=%.2f C  P=%.2f hPa", data.temperature_c, data.pressure_hpa);
        } else {
            display_draw_text(2, 0, "Read error");
            ESP_LOGW(TAG, "Barometer read failed");
        }

        display_update();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(POLL_PERIOD_MS));
    }
}
