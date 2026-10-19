#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <driver/pulse_cnt.h>
#include <esp_log.h>

#define ENCODER_CLK         GPIO_NUM_38
#define ENCODER_DT          GPIO_NUM_37
#define ENCODER_LIMIT       1000        // PCNT wraps here, accum_count keeps the total
#define ENCODER_GLITCH_NS   1000        // Ignore contact bounce shorter than 1 us
#define ENCODER_STEPS_PER_DETENT 4      // Full quadrature: 4 edges per click

#define SERVO_PIN           36
#define SERVO_FREQ          50
#define SERVO_PERIOD_US     (1000000 / SERVO_FREQ)
#define SERVO_RESOLUTION    LEDC_TIMER_12_BIT
#define SERVO_MAX_DUTY      ((1 << SERVO_RESOLUTION) - 1)
#define SERVO_UNIT          LEDC_TIMER_0
#define SERVO_CHANNEL       LEDC_CHANNEL_0

#define BUTTON_PIN          GPIO_NUM_48
#define BUTTON_DEBOUNCE_MS  30          // Level must hold this long to count

#define DEG_PER_DETENT      5
#define DEG_PER_DETENT_FAST 15
#define SUPERLOOP_DELAY     20

// Calibration variables
static int SERVO_MIN_DEG = 10;
static int SERVO_MAX_DEG = 170;

static int SERVO_MIN_US = 500;
static int SERVO_MAX_US = 2500;

static pcnt_unit_handle_t pcnt_unit;

static bool isFastMode = false;

static const char *TAG = "Mini Project 3";


static void encoder_init(void) {
    pcnt_unit_config_t unit_config = {
        .low_limit = -ENCODER_LIMIT,
        .high_limit = ENCODER_LIMIT,
        .flags.accum_count = 1,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = ENCODER_GLITCH_NS,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    // Two channels = full quadrature decoding (counts every edge of both pins)
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = ENCODER_CLK,
        .level_gpio_num = ENCODER_DT,
    };
    pcnt_channel_handle_t chan_a;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &chan_a));

    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = ENCODER_DT,
        .level_gpio_num = ENCODER_CLK,
    };
    pcnt_channel_handle_t chan_b;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &chan_b));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(chan_a,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(chan_a,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(chan_b,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(chan_b,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    // Watch points on the limits are required for accum_count to work
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, -ENCODER_LIMIT));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, ENCODER_LIMIT));

    // Enable internal pull-ups (harmless if the module has its own)
    gpio_set_pull_mode(ENCODER_CLK, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(ENCODER_DT, GPIO_PULLUP_ONLY);

    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
}

static void servo_init(void) {
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = SERVO_UNIT,
        .duty_resolution = SERVO_RESOLUTION,
        .freq_hz = SERVO_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    ledc_channel_config_t channel_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = SERVO_CHANNEL,
        .timer_sel = SERVO_UNIT,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = SERVO_PIN,
        .duty = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
}

static int val_clamp(int value, int val_min, int val_max) {
    if (value < val_min) return val_min;
    if (value > val_max) return val_max;
    return value;
}

static int mad_map(int value, int in_min, int in_max, int out_min, int out_max) {
    if (in_min == in_max) return out_max;

    value = val_clamp(value, in_min, in_max);

    return out_min + (value - in_min) * (out_max - out_min) / (in_max - in_min);
}

static void set_angle(int deg) {
    int val_us = mad_map(deg, SERVO_MIN_DEG, SERVO_MAX_DEG, SERVO_MIN_US, SERVO_MAX_US);
    uint32_t duty = (uint32_t) val_us * (SERVO_MAX_DUTY + 1) / SERVO_PERIOD_US;

    ESP_LOGI(TAG, "angle = %d deg, pulse = %d us, duty = %" PRIu32, deg, val_us, duty);

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL));
}


static void button_init(void) {
    gpio_config_t io_config = {
        .pin_bit_mask = 1ULL << BUTTON_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, // SW shorts to GND when pressed
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_config));
}

// Call once per loop. Returns true once per press: the new level must stay
// unchanged for BUTTON_DEBOUNCE_MS before it's accepted, so bounce is ignored
static bool button_pressed(void) {
    static int last_raw = 1;
    static int stable = 1;
    static TickType_t changed_at = 0;

    int raw = gpio_get_level(BUTTON_PIN);
    TickType_t now = xTaskGetTickCount();

    if (raw != last_raw) {
        last_raw = raw;
        changed_at = now;
        return false;
    }

    if (raw != stable && (now - changed_at) >= pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS)) {
        stable = raw;
        return stable == 0; // Act on press (HIGH -> LOW), not on release
    }

    return false;
}


void app_main(void) {
    encoder_init();
    servo_init();
    button_init();

    int angle = (SERVO_MIN_DEG + SERVO_MAX_DEG) / 2;
    int last_detents = 0;
    set_angle(angle);

    while (1) {
        if (button_pressed()) {
            isFastMode = !isFastMode;
            ESP_LOGI(TAG, "Fast mode: %s", isFastMode ? "ON" : "OFF");
        }

        int count = 0;
        ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &count));

        // Move relative to the last position, so turning past an end stop
        // doesn't need to be "unwound" before the servo reacts again
        int detents = count / ENCODER_STEPS_PER_DETENT;
        int delta = detents - last_detents;
        last_detents = detents;

        if (delta != 0) {
            int step = isFastMode ? DEG_PER_DETENT_FAST : DEG_PER_DETENT;
            int new_angle = val_clamp(angle + delta * step, SERVO_MIN_DEG, SERVO_MAX_DEG);
            if (new_angle != angle) {
                angle = new_angle;
                set_angle(angle);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(SUPERLOOP_DELAY));
    }
}
