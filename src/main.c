#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED_4 GPIO_NUM_4
#define LED_5 GPIO_NUM_5
#define LED_6 GPIO_NUM_6

static const char *TAG = "MAIN";
static const int DELAY = 100;

void app_main(void)
{
    gpio_config_t io_conf4 = {
        .pin_bit_mask = (1ULL << LED_4),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };

    gpio_config_t io_conf5 = {
        .pin_bit_mask = (1ULL << LED_5),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };

    gpio_config_t io_conf6 = {
        .pin_bit_mask = (1ULL << LED_6),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };

    gpio_config(&io_conf4);
    gpio_config(&io_conf5);
    gpio_config(&io_conf6);

    int a = 0;

    ESP_LOGI(TAG, "Hello, ESP32!");

    while (1)
    {
        a++;
        ESP_LOGI(TAG, "Loop %d", a);
        gpio_set_level(LED_4, 1); // ON
        gpio_set_level(LED_5, 0); // OFF
        gpio_set_level(LED_6, 0); // OFF
        vTaskDelay(pdMS_TO_TICKS(DELAY));

        gpio_set_level(LED_4, 0); // ON
        gpio_set_level(LED_5, 1); // OFF
        gpio_set_level(LED_6, 0); // OFF
        vTaskDelay(pdMS_TO_TICKS(DELAY));

        gpio_set_level(LED_4, 0); // ON
        gpio_set_level(LED_5, 0); // OFF
        gpio_set_level(LED_6, 1); // OFF
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
}