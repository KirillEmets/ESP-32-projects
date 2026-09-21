#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"

#define ADC_CHANNEL ADC_CHANNEL_5
#define ADC_ATTEN ADC_ATTEN_DB_12

#define LED_GPIO GPIO_NUM_4

#define SMA_SIZE 2

#define DARK_THRESHOLD 1200
#define LIGHT_THRESHOLD 1600

#define ADC_SAMPLE_INTERVAL_MS 500

static const char *TAG = "Lesson 3.2";

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t adc_cali_handle;

static int calculate_sma(int new_value)
{
    static int samples[SMA_SIZE] = {0};
    static int index = 0;
    static int sum = 0;
    static int count = 0;

    sum -= samples[index];

    samples[index] = new_value;
    sum += new_value;

    index = (index + 1) % SMA_SIZE;

    if (count < SMA_SIZE)
    {
        count++;
    }

    return sum / count;
}
void configure_gpio()
{
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&led_config);
    gpio_set_level(LED_GPIO, 0);
}

void configure_adc()
{
    adc_oneshot_unit_init_cfg_t adc_config = {
        .unit_id = ADC_UNIT_1,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_config, &adc_handle));

    adc_oneshot_chan_cfg_t channel_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };

    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            adc_handle,
            ADC_CHANNEL,
            &channel_config));

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .chan = ADC_CHANNEL};

    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle));
}

void app_main(void)
{
    configure_gpio();
    configure_adc();

    int raw_adc = 0;
    int filtered_adc = 0;

    float calculated_voltage = 0;
    int cali_voltage_mv = 0;
    float cali_voltage = 0;
    float error = 0;

    while (true)
    {
        ESP_ERROR_CHECK(
            adc_oneshot_read(
                adc_handle,
                ADC_CHANNEL,
                &raw_adc));

        filtered_adc = calculate_sma(raw_adc);

        calculated_voltage = 3.312f * filtered_adc / 4095.0f;
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, filtered_adc, &cali_voltage_mv));
        cali_voltage = cali_voltage_mv / 1000.0f;

        error = fabs(calculated_voltage - cali_voltage) / cali_voltage * 100.0f;

        ESP_LOGI(
            TAG,
            "Filtered ADC: %d, calculated: %.2fV, calibrated: %dmV = %.2fV, error: %.2f\%",
            filtered_adc,
            calculated_voltage,
            cali_voltage_mv,
            cali_voltage,
            error);

        vTaskDelay(pdMS_TO_TICKS(ADC_SAMPLE_INTERVAL_MS));
    }
}