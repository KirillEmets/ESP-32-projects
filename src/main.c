#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"

#define ADC_CHANNEL ADC_CHANNEL_5
#define ADC_ATTEN ADC_ATTEN_DB_12

#define LED_GPIO GPIO_NUM_4

#define SMA_SIZE 10

#define DARK_THRESHOLD 1200
#define LIGHT_THRESHOLD 1600

#define ADC_SAMPLE_INTERVAL_MS 100

static const char *TAG = "Lesson 3.1";

static adc_oneshot_unit_handle_t adc_handle;

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
}

void app_main(void)
{
    configure_gpio();
    configure_adc();

    bool led_on = false;

    while (true)
    {
        int raw_adc;

        ESP_ERROR_CHECK(
            adc_oneshot_read(
                adc_handle,
                ADC_CHANNEL,
                &raw_adc));

        int filtered_adc = calculate_sma(raw_adc);

        ESP_LOGI(
            TAG,
            "Raw ADC=%d, SMA=%d",
            raw_adc,
            filtered_adc);

        if (!led_on && filtered_adc < DARK_THRESHOLD)
        {
            led_on = true;
            gpio_set_level(LED_GPIO, 1);

            ESP_LOGI(TAG, "Dark -> LED ON, ADC=%d", filtered_adc);
        }
        else if (led_on && filtered_adc > LIGHT_THRESHOLD)
        {
            led_on = false;
            gpio_set_level(LED_GPIO, 0);

            ESP_LOGI(TAG, "Light -> LED OFF, ADC=%d", filtered_adc);
        }

        vTaskDelay(pdMS_TO_TICKS(ADC_SAMPLE_INTERVAL_MS));
    }
}