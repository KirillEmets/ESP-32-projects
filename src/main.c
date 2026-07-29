#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED_4 GPIO_NUM_4
#define LED_5 GPIO_NUM_5
#define LED_6 GPIO_NUM_6

#define BTN_17 GPIO_NUM_17
#define BTN_BOOT GPIO_NUM_0

static const char *TAG = "MAIN";
static int delay = 100;

bool checkButtonPressedWithDebounce(gpio_num_t pin)
{
    int level = gpio_get_level(pin);
    if (level == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(30));
        int secondCheck = gpio_get_level(pin);
        return secondCheck == 0;
    }
    else
    {
        return false;
    }
}

void checkModeInput(void) {
    int fastModeButtonPressed = checkButtonPressedWithDebounce(BTN_17);
    if (fastModeButtonPressed) {
        ESP_LOGI(TAG, "BTN_17 pressed, enable fast mode.");
        delay = 100;
        return;
    }

    int slowModeButtonPressed = checkButtonPressedWithDebounce(BTN_BOOT);
    if (slowModeButtonPressed) {
        ESP_LOGI(TAG, "BTN_BOOT pressed, enable slow mode.");
        delay = 1000;
        return;
    }
}

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

    gpio_config_t io_conf_btn = {
        .pin_bit_mask = (1ULL << BTN_17),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };

    gpio_config_t io_conf_boot = {
        .pin_bit_mask = (1ULL << BTN_BOOT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };

    gpio_config(&io_conf4);
    gpio_config(&io_conf5);
    gpio_config(&io_conf6);
    gpio_config(&io_conf_btn);
    gpio_config(&io_conf_boot);

    ESP_LOGI(TAG, "Hello, Lesson 1.4 (15)");

    while (1)
    {
        gpio_set_level(LED_4, 1); // ON
        gpio_set_level(LED_5, 0); // OFF
        gpio_set_level(LED_6, 0); // OFF
        vTaskDelay(pdMS_TO_TICKS(delay));
        checkModeInput();

        gpio_set_level(LED_4, 0); // ON
        gpio_set_level(LED_5, 1); // OFF
        gpio_set_level(LED_6, 0); // OFF
        vTaskDelay(pdMS_TO_TICKS(delay));
        checkModeInput();

        gpio_set_level(LED_4, 0); // ON
        gpio_set_level(LED_5, 0); // OFF
        gpio_set_level(LED_6, 1); // OFF
        vTaskDelay(pdMS_TO_TICKS(delay));
        checkModeInput();
    }
}
