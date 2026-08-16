#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

enum class LedState
{
    On,
    Off
};

class Led
{
private:
    gpio_num_t _pin;

public:
    Led(gpio_num_t pin)
    {
        _pin = pin;
    }

    void init()
    {
        gpio_config_t config = {};
        config.pin_bit_mask = (1ULL << _pin);
        config.mode = GPIO_MODE_OUTPUT;
        config.pull_up_en = GPIO_PULLUP_DISABLE;
        config.pull_down_en = GPIO_PULLDOWN_DISABLE;
        config.intr_type = GPIO_INTR_DISABLE;

        gpio_config(&config);
    };

    void set(LedState state)
    {
        const int level = (state == LedState::On) ? 1 : 0;
        gpio_set_level(_pin, level);
    }
};

namespace AppConfig
{
    static constexpr gpio_num_t pin_led = GPIO_NUM_4;
    static constexpr uint32_t blink_delay_ms = 100;
};

uint32_t millis()
{
    return esp_timer_get_time() / 1000;
}

extern "C" void app_main()
{
    // Init led instance on pin GPIO_4;
    Led led(AppConfig::pin_led);
    led.init();

    // Keep state and last blink time in the stack;
    LedState state = LedState::Off;
    uint32_t lastBlinkTime = millis();

    while (true)
    {
        // Check time instead of blocking execution with delays;
        const uint32_t now = millis();

        if (now >= lastBlinkTime + AppConfig::blink_delay_ms)
        {
            lastBlinkTime = now;

            state = (state == LedState::Off) ? LedState::On : LedState::Off;

            led.set(state);
        }
    }
}