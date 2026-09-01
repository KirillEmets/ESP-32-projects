#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"


static const gpio_num_t ISR_PIN = GPIO_NUM_6;

static volatile uint32_t isr_time = 0;
static volatile uint32_t counter = 0;
static volatile bool flag = false;

static const char *TAG = "Task 1";

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    counter++;
    isr_time = esp_timer_get_time() / 1000;
    flag = true;
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

void task1()
{
    configure_gpio();

    while (true)
    {
        uint32_t now_ms = esp_timer_get_time() / 1000;

        if (flag)
        {
            // Clear the flag
            flag = false;
            uint32_t delay_ms = now_ms - isr_time;
            ESP_LOGI(TAG, "Counter = %lu; delay_ms: %d", counter, delay_ms);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}