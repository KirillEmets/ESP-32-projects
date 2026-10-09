#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

#define SERVO_PIN GPIO_NUM_36

#define ADC_UNIT_USED ADC_UNIT_1
#define ADC_CHANNEL_USED ADC_CHANNEL_1

#define SERVO_MIN_PULSE_US 500
#define SERVO_MAX_PULSE_US 2500
#define SERVO_PERIOD_US 20000 // 50 Hz

#define ADC_MAX_VALUE 4095
#define SERVO_MAX_ANGLE 180

static adc_oneshot_unit_handle_t adc_handle;

static void adc_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_USED,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(
        adc_handle,
        ADC_CHANNEL_USED,
        &channel_config));
}

static void servo_init(void)
{
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_14_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 50,
        .clk_cfg = LEDC_AUTO_CLK,
    };

    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    ledc_channel_config_t channel_config = {
        .gpio_num = SERVO_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };

    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
}

static void servo_set_angle(int angle)
{
    if (angle < 0)
    {
        angle = 0;
    }
    else if (angle > SERVO_MAX_ANGLE)
    {
        angle = SERVO_MAX_ANGLE;
    }

    const uint32_t pulse_us =
        SERVO_MIN_PULSE_US +
        (uint32_t)angle *
            (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) /
            SERVO_MAX_ANGLE;

    // 14-bit duty resolution: 16384 steps per PWM period.
    const uint32_t duty =
        (uint32_t)pulse_us * (1U << 14) / SERVO_PERIOD_US;

    ESP_ERROR_CHECK(ledc_set_duty(
        LEDC_LOW_SPEED_MODE,
        LEDC_CHANNEL_0,
        duty));

    ESP_ERROR_CHECK(ledc_update_duty(
        LEDC_LOW_SPEED_MODE,
        LEDC_CHANNEL_0));
}

void app_main(void)
{
    adc_init();
    servo_init();

    servo_set_angle(0);

    while (1)
    {
        int raw = 0;

        ESP_ERROR_CHECK(adc_oneshot_read(
            adc_handle,
            ADC_CHANNEL_USED,
            &raw));

        const int angle = raw * SERVO_MAX_ANGLE / ADC_MAX_VALUE;

        servo_set_angle(angle);

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}