#include "timer_variant.h"
#include "driver/gptimer.h"

gptimer_handle_t gptimer = NULL;

static TimerCallback user_callback;

static bool example_timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    gptimer_stop(timer);
    user_callback(NULL);
    return false;
}

static void setup_gp_timer(TimerCallback callback)
{
    user_callback = callback;

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT, // Select the default clock source
        .direction = GPTIMER_COUNT_UP,      // Counting direction is up
        .resolution_hz = 1 * 1000 * 1000,   // Resolution is 1 MHz, i.e., 1 tick equals 1 microsecond
    };

    gptimer_event_callbacks_t cbs = {
        .on_alarm = example_timer_on_alarm_cb, // Call the user callback function when the alarm event occurs
    };

    gptimer_new_timer(&timer_config, &gptimer);
    gptimer_register_event_callbacks(gptimer, &cbs, NULL);
    gptimer_enable(gptimer);
}

static void start_gp_timer(uint64_t duration_us)
{
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = duration_us,           // duration_us matches raw counter value because resolution is 1MHz;
        .flags.auto_reload_on_alarm = false}; // For one-shot timer we don't need auto-reload;

    gptimer_set_raw_count(gptimer, 0);
    gptimer_set_alarm_action(gptimer, &alarm_config);
    gptimer_start(gptimer);
}

TimerVariant getHardwareVariant()
{
    TimerVariant v = {
        .start = &start_gp_timer,
        .setup = &setup_gp_timer,
    };

    return v;
}