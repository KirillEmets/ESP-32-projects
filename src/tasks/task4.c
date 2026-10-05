#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

static const gpio_num_t POLL_PIN = GPIO_NUM_6;

static volatile uint32_t counter = 0;
static volatile bool log_flag = false;

static const char *TAG = "Task 4";

typedef enum
{
    StableHigh,
    StableLow,
    DebouncingLow
} DebounceState;

static const uint32_t check_time_ms = 10;
static const uint32_t check_threshold_ms = 30;

static uint32_t debounce_start_time_ms = 0;
static DebounceState state = StableHigh;

static void configure_gpio()
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << POLL_PIN),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };

    gpio_config(&io_conf);
}

void task4()
{
    configure_gpio();

    while (true)
    {
        uint32_t now_ms = esp_timer_get_time() / 1000;
        int pin_level = gpio_get_level(POLL_PIN);

        switch (state)
        {
        case StableHigh:
        {
            if (pin_level == 0)
            {
                debounce_start_time_ms = now_ms;
                state = DebouncingLow;
            }

            break;
        }

        case DebouncingLow:
        {
            if (pin_level == 1)
            {
                state = StableHigh;
                break;
            }

            if (now_ms - debounce_start_time_ms >= check_threshold_ms)
            {
                state = StableLow;

                counter++;
                log_flag = true;
            }

            break;
        }

        case StableLow:
        {
            // Wait for the button/input to be released.
            if (pin_level == 1)
            {
                state = StableHigh;
            }

            break;
        }
        }

        if (log_flag)
        {
            log_flag = false;
            uint32_t delay_ms = now_ms - debounce_start_time_ms;
            ESP_LOGI(TAG, "Counter = %lu; delay_ms: %d", counter, delay_ms);
        }

        vTaskDelay(pdMS_TO_TICKS(check_time_ms));
    }
}