#include "timer_variant.h"
#include "esp_timer.h"

static esp_timer_handle_t timer_handle;

static void setup_software_timer(TimerCallback callback)
{
    esp_timer_create_args_t args = {
        .callback = callback,
        .name = "off_timer",
    };

    esp_timer_create(&args, &timer_handle);
}

static void start_timer(uint64_t duration_us)
{
    esp_timer_start_once(timer_handle, duration_us);
}

TimerVariant getSoftwareVariant()
{
    TimerVariant v = {
        .start = &start_timer,
        .setup = &setup_software_timer,
    };

    return v;
};
