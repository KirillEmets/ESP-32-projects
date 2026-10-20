#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

#define INPUT_PIN GPIO_NUM_39
#define LED_PIN GPIO_NUM_36
#define REPORT_INTERVAL_MS 50UL
#define UART_PORT UART_NUM_0
#define UART_BAUD_RATE 9600
#define UART_RX_BUF_SIZE 256
#define UART_TX_BUF_SIZE 256

static void apply_led_state(const char incoming)
{
    if (incoming == '1')
    {
        gpio_set_level(LED_PIN, 1);
    }
    else if (incoming == '0')
    {
        gpio_set_level(LED_PIN, 0);
    }
}

void app_main(void)
{
    // Configure LED as output
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    gpio_set_level(LED_PIN, 0);

    // Configure input pin as pull-up
    gpio_config_t input_conf = {
        .pin_bit_mask = (1ULL << INPUT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&input_conf);

    // Configure UART0
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_RX_BUF_SIZE, UART_TX_BUF_SIZE, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));

    // Use default UART0 pins on ESP32:
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, 1, 2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    uint32_t last_report_time_ms = 0;

    while (1)
    {
        // Read all available UART bytes and apply them like Arduino Serial.read()
        size_t buffered_len = 0;
        uart_get_buffered_data_len(UART_PORT, &buffered_len);

        while (buffered_len > 0)
        {
            uint8_t ch = 0;
            const int read_len = uart_read_bytes(UART_PORT, &ch, 1, 0);
            if (read_len > 0)
            {
                apply_led_state((char)ch);
            }
            uart_get_buffered_data_len(UART_PORT, &buffered_len);
        }

        const uint32_t now = esp_timer_get_time() / 1000ULL;
        if ((now - last_report_time_ms) >= REPORT_INTERVAL_MS)
        {
            last_report_time_ms = now;

            const bool button_pressed = (gpio_get_level(INPUT_PIN) == 0);
            const char output = button_pressed ? '1' : '0';

            uart_write_bytes(UART_PORT, &output, 1);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
