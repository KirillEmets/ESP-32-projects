#include "driver/gpio.h"
#include "soc/gpio_num.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define GPIO_SET GPIO_NUM_4
#define GPIO_ISR GPIO_NUM_6

volatile bool isrFlag = false;
volatile uint32_t isrMicros = 0;
uint32_t startMicros = 0;

void configure_gpio()
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << GPIO_SET),
    };

    gpio_config_t io_conf_isr = {
        .intr_type = GPIO_INTR_NEGEDGE, // Rising edge interrupt trigger
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << GPIO_ISR),
        .pull_down_en = 0,
        .pull_up_en = 1};

    gpio_config(&io_conf);
    gpio_config(&io_conf_isr);
    gpio_set_level(GPIO_SET, 1);
}

void IRAM_ATTR isr_handler(void *arg)
{
    isrFlag = true;
    isrMicros = esp_timer_get_time();
};

void tickRelay()
{
    startMicros = esp_timer_get_time();
    gpio_set_level(GPIO_SET, 0);
};

void app_main()
{
    configure_gpio();
    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_ISR, isr_handler, NULL);

    vTaskDelay(pdMS_TO_TICKS(3000)); // Initial dealy

    ESP_LOGI("Lesson 2.2", "Measurements starting.");
    tickRelay();

    const int maxCount = 10;
    int counter = 1;
    int sum = 0;

    while (counter <= maxCount)
    {
        if (isrFlag == true)
        {
            // Reset state
            isrFlag = false;
            gpio_set_level(GPIO_SET, 1);

            // log
            uint32_t difference = isrMicros - startMicros;
            sum += difference;
            ESP_LOGI("Lesson 2.2", "Measurement %d: %d microseconds", counter, difference);
            counter++;

            // Delay
            if (counter <= maxCount)
            {
                vTaskDelay(pdMS_TO_TICKS(100));
                tickRelay();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Avoid busy waiting
    }

    float avgMs = (float)sum / maxCount / 1000.0f;
    ESP_LOGI("Lesson 2.2", "Measurements completed. Avg: %.2f ms", avgMs);
};