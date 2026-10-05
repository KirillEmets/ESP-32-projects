#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

static const gpio_num_t ISR_PIN = GPIO_NUM_6;

static const uint32_t debounce_time_ms = 50;
static volatile uint32_t last_trigger_time_ms = 0;

static volatile uint32_t counter = 0;
static volatile bool flag = false;

static const char *TAG = "Task 2";

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    int isr_time_ms = esp_timer_get_time() / 1000;

    if (isr_time_ms - last_trigger_time_ms > debounce_time_ms)
    {
        counter++;
        flag = true;
    }
    last_trigger_time_ms = isr_time_ms;
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

void task2()
{
    configure_gpio();

    while (true)
    {
        uint32_t now_ms = esp_timer_get_time() / 1000;

        if (flag)
        {
            // Clear the flag
            flag = false;
            uint32_t delay_ms = now_ms - last_trigger_time_ms;
            ESP_LOGI(TAG, "Counter = %lu; delay_ms: %d", counter, delay_ms);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}