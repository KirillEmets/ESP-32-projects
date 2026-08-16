#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BUTTON GPIO_NUM_15

static const char *TAG = "button_app";
static QueueHandle_t gpio_evt_queue = NULL;
static volatile int16_t button_counter = 0;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t event = 1;
    xQueueSendFromISR(gpio_evt_queue, &event, NULL);
}

static void button_task(void *arg)
{
    uint32_t event;

    while (true)
    {
        if (xQueueReceive(gpio_evt_queue, &event, portMAX_DELAY))
        {
            button_counter++;
            ESP_LOGI(TAG, "BUTTON Pressed! Count: %d", button_counter);
        }
    }
}

void app_main(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON),
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    if (gpio_evt_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create GPIO event queue");
        return;
    }

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON, gpio_isr_handler, (void *)BUTTON);

    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
}
