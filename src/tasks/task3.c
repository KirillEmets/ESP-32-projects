#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

static const gpio_num_t ISR_PIN = GPIO_NUM_6;

static volatile uint32_t counter = 0;
static volatile bool log_flag = false;

static volatile bool isr_flag = false;

static const char *TAG = "Task 3";

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

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    isr_flag = true;
}

static void configure_gpio()
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << ISR_PIN),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };

    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(ISR_PIN, gpio_isr_handler, nullptr);
}

void task3()
{
    configure_gpio();

    while (true)
    {
        uint32_t now_ms = esp_timer_get_time() / 1000;
        int pin_level = gpio_get_level(ISR_PIN);

        switch (state)
        {
        case StableHigh:
        {
            if (isr_flag)
            {
                isr_flag = false;
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
                isr_flag = false;
                break;
            }

            if (now_ms - debounce_start_time_ms >= check_threshold_ms)
            {
                state = StableLow;

                counter++;
                log_flag = true;
                isr_flag = false;
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