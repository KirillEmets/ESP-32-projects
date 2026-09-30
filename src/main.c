#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/ledc.h"
#include "driver/gptimer.h"
#include "esp_attr.h"
#include "esp_log.h"

#define TAG "Lesson 3.4"

#define PWM_GPIO 4
#define PWM_FREQUENCY 5000

#define PWM_RESOLUTION LEDC_TIMER_10_BIT

#define PWM_TIMER LEDC_TIMER_0
#define PWM_CHANNEL LEDC_CHANNEL_0
#define PWM_SPEED_MODE LEDC_LOW_SPEED_MODE

#define NUM_SAMPLES 112
#define SAMPLE_RATE 10000

#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523

static const uint8_t sine_table[NUM_SAMPLES] = {
    128, 131, 135, 138, 142, 145, 149, 152, 155, 158, 162, 165,
    167, 170, 173, 175, 178, 180, 182, 184, 185, 187, 188, 189,
    190, 191, 191, 191, 192, 191, 191, 191, 190, 189, 188, 187,
    185, 184, 182, 180, 178, 175, 173, 170, 167, 165, 162, 158,
    155, 152, 149, 145, 142, 138, 135, 131, 128, 124, 120, 117,
    113, 110, 106, 103, 100, 97, 93, 90, 88, 85, 82, 80,
    77, 75, 73, 71, 70, 68, 67, 66, 65, 64, 64, 64,
    64, 64, 64, 64, 65, 66, 67, 68, 70, 71, 73, 75,
    77, 80, 82, 85, 88, 90, 93, 97, 100, 103, 106, 110,
    113, 117, 120, 124};

static volatile uint32_t sample_position = 0; // Fixed-point: 16 бітів цілого числа і 16 бітів після коми.
static volatile uint32_t sample_step = 0;

static bool IRAM_ATTR audio_timer_callback(
    gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata,
    void *user_ctx)
{
    int index = sample_position >> 16;
    uint8_t sample = sine_table[index];

    uint32_t duty = ((uint32_t)sample * 1023) / 255;

    ledc_set_duty(PWM_SPEED_MODE, PWM_CHANNEL, duty);
    ledc_update_duty(PWM_SPEED_MODE, PWM_CHANNEL);

    sample_position += sample_step;
    if (sample_position >= (NUM_SAMPLES << 16))
    {
        sample_position -= (NUM_SAMPLES << 16);
    }

    return false;
}

static void pwm_init(void)
{
    ledc_timer_config_t timer_config = {
        .speed_mode = PWM_SPEED_MODE,
        .timer_num = PWM_TIMER,
        .duty_resolution = PWM_RESOLUTION,
        .freq_hz = PWM_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK,
    };

    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    ledc_channel_config_t channel_config = {
        .gpio_num = PWM_GPIO,
        .speed_mode = PWM_SPEED_MODE,
        .channel = PWM_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = PWM_TIMER,
        .duty = 512,
        .hpoint = 0,
    };

    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
}

static void audio_timer_init(void)
{
    gptimer_handle_t timer = NULL;

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1 MHz = 1 us
    };

    ESP_ERROR_CHECK(
        gptimer_new_timer(&timer_config, &timer));

    gptimer_event_callbacks_t callbacks = {
        .on_alarm = audio_timer_callback,
    };

    ESP_ERROR_CHECK(
        gptimer_register_event_callbacks(
            timer,
            &callbacks,
            NULL));

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = 50, // 10 us = 100 kHz
        .flags.auto_reload_on_alarm = true,
    };

    ESP_ERROR_CHECK(
        gptimer_set_alarm_action(
            timer,
            &alarm_config));

    ESP_ERROR_CHECK(gptimer_enable(timer));

    ESP_ERROR_CHECK(gptimer_start(timer));
}

static void play_note(int frequency, int duration_ms)
{
    sample_step = ((uint32_t)frequency * NUM_SAMPLES << 16) / SAMPLE_RATE;

    vTaskDelay(pdMS_TO_TICKS(duration_ms));

    sample_step = 0;

    ledc_set_duty(
        PWM_SPEED_MODE,
        PWM_CHANNEL,
        512);

    ledc_update_duty(
        PWM_SPEED_MODE,
        PWM_CHANNEL);

    vTaskDelay(pdMS_TO_TICKS(50));
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing PWM buzzer...");

    pwm_init();
    audio_timer_init();

    ESP_LOGI(TAG, "PWM buzzer ready!");

    while (1)
    {
        // Baby Shark
        play_note(NOTE_C4, 600);
        play_note(NOTE_D4, 600);
        play_note(NOTE_F4, 300);
        play_note(NOTE_F4, 300);
        play_note(NOTE_F4, 300);
        play_note(NOTE_F4, 300);
        play_note(NOTE_F4, 300);

        play_note(NOTE_C4, 300);
        play_note(NOTE_D4, 300);
        play_note(NOTE_F4, 300);
        play_note(NOTE_F4, 300);
        play_note(NOTE_F4, 300);
        play_note(NOTE_F4, 300);
        play_note(NOTE_F4, 300);

        play_note(NOTE_C4, 300);
        play_note(NOTE_D4, 300);
        play_note(NOTE_F4, 300);
        play_note(NOTE_F4, 300);
        play_note(NOTE_F4, 300);
        play_note(NOTE_F4, 300);
        play_note(NOTE_F4, 300);

        play_note(NOTE_C4, 300);
        play_note(NOTE_D4, 300);
        play_note(NOTE_F4, 300);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}