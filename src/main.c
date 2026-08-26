#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#define PIN_LED_1 GPIO_NUM_4
#define PIN_LED_2 GPIO_NUM_5
#define PIN_LED_3 GPIO_NUM_6

typedef struct
{
    gpio_num_t pin;
    uint64_t delay;
    uint64_t last_blink_ms;
    bool enabled;
} led_data;

static led_data leds[3] = {
    {PIN_LED_1, 200, 0, false},
    {PIN_LED_2, 500, 0, false},
    {PIN_LED_3, 1000, 0, false}};

static size_t count = sizeof(leds) / sizeof(led_data);

void setup_gpio()
{
    size_t count = sizeof(leds) / sizeof(led_data);

    for (size_t i = 0; i < count; i++)
    {
        gpio_config_t config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1 << leds[i].pin};

        gpio_config(&config);
    }
};

void app_main()
{
    setup_gpio();
    uint64_t millis = 0;

    // This would be a loop() function in arduino
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1));
        millis = esp_timer_get_time() / 1000;

        for (size_t i = 0; i < count; i++)
        {
            bool shouldBlink = (millis - leds[i].last_blink_ms) > leds[i].delay;
            if (shouldBlink)
            {
                leds[i].enabled = !leds[i].enabled;
                leds[i].last_blink_ms = millis;
                int level = leds[i].enabled ? 1 : 0;
                gpio_set_level(leds[i].pin, level);
            }
        }
    }
};