#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED_GPIO        GPIO_NUM_6
#define BUTTON_GPIO     GPIO_NUM_5

#define NORMAL_PERIOD_MS 500
#define FAST_PERIOD_MS   (NORMAL_PERIOD_MS / 5)
#define POLL_MS          10

void app_main(void)
{
    gpio_config_t led_conf = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);

    gpio_config_t button_conf = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&button_conf);

    int led_state = 0;
    int elapsed_ms = 0;

    while (1) {
        // Poll the button: LOW means pressed (pull-up enabled)
        int period_ms = gpio_get_level(BUTTON_GPIO) == 0 ? FAST_PERIOD_MS : NORMAL_PERIOD_MS;

        if (elapsed_ms >= period_ms) {
            led_state = !led_state;
            gpio_set_level(LED_GPIO, led_state);
            elapsed_ms = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(POLL_MS));
        elapsed_ms += POLL_MS;
    }
}
