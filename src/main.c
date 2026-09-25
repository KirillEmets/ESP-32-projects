#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#define ADC_CHANNEL ADC_CHANNEL_5
#define ADC_ATTEN ADC_ATTEN_DB_12

#define LED_GPIO GPIO_NUM_4
#define MOTOR_GPIO GPIO_NUM_5

#define SMA_SIZE 3

#define ADC_SAMPLE_INTERVAL_MS 100

static const char *TAG = "Lesson 3.3";

static adc_oneshot_unit_handle_t adc_handle;

int map_range_int(int x, int a, int b, int a2, int b2)
{
    return (int)((float)(x - a) * (b2 - a2) / (b - a) + a2);
}

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

void configurePWM()
{
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 20000,
        .clk_cfg = LEDC_AUTO_CLK};

    ledc_timer_config(&timer_config);

    ledc_channel_config_t channel_config = {
        .gpio_num = LED_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0};

    ledc_channel_config(&channel_config);

    ledc_channel_config_t channel_config_2 = {
        .gpio_num = MOTOR_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0};

    ledc_channel_config(&channel_config_2);
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
    configurePWM();
    configure_adc();

    int raw_adc = 0;
    int filtered_adc = 0;
    int duty = 0;
    int percent = 0;
    int motor_p = 0;
    int motor_duty = 0;

    while (true)
    {
        ESP_ERROR_CHECK(
            adc_oneshot_read(
                adc_handle,
                ADC_CHANNEL,
                &raw_adc));

        filtered_adc = calculate_sma(raw_adc);

        percent = (int)(powf(filtered_adc / 4095.0f, 2) * 100);
        duty = (int)(1024.0f * percent / 100);

        motor_p = map_range_int(percent, 0, 100, 60, 100);
        motor_duty = (int)(1024.0f * motor_p / 100);

        ESP_LOGI(
            TAG,
            "Filtered ADC: %d, LED duty: %d - %d%, Motor duty: %d - %d%",
            filtered_adc,
            duty,
            percent,
            motor_duty,
            motor_p);

        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, motor_duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);

        vTaskDelay(pdMS_TO_TICKS(ADC_SAMPLE_INTERVAL_MS));
    }
}