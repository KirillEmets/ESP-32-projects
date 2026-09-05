#include "driver/gpio.h"
#include "esp_log.h"
#include "variants/timer_variant.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

const char *TAG = "Lesson 2.5";
static bool fan_on = true;

#define PIN_OUT GPIO_NUM_6

// Fan is OFF for 10 seconds and ON for 5 seconds. Can be changed to 1 hour and 15 minutes per task requirements.
#define OFF_DURATION_US 10ULL * 1000000
#define ON_DURATION_US 5ULL * 1000000

TimerVariant timer;
bool callback_triggered = false;

static void timer_callback(void *arg)
{
    callback_triggered = true;
}

static void configure_gpio()
{
    gpio_config_t conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_OUT),
    };

    gpio_config(&conf);
}

void app_main()
{
    ESP_LOGI(TAG, "App started");

    // Set timer interface variable to Software/Hardware implementation;
    timer = getHardwareVariant();

    configure_gpio();
    timer.setup(&timer_callback);

    fan_on = 1;
    gpio_set_level(PIN_OUT, 1);
    timer.start(ON_DURATION_US);

    while (1)
    {
        if (callback_triggered)
        {
            callback_triggered = false;
            uint8_t next_level = fan_on ? 0 : 1;
            uint64_t next_duration = fan_on ? ON_DURATION_US : OFF_DURATION_US;

            ESP_LOGI(TAG, "Fan turn %s", fan_on ? "OFF" : "ON");
            gpio_set_level(PIN_OUT, next_level);
            fan_on = !fan_on;
            timer.start(next_duration);
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}