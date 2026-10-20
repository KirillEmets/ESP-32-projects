#include <Arduino.h>

namespace
{
    constexpr uint8_t input_pin = 5;
    constexpr uint8_t led_pin = 4;
    constexpr unsigned long report_interval_ms = 50UL;

    unsigned long last_report_time_ms = 0;

    void apply_led_state(const char incoming)
    {
        if (incoming == '1')
        {
            digitalWrite(led_pin, HIGH);
        }
        else if (incoming == '0')
        {
            digitalWrite(led_pin, LOW);
        }
    }
}

void setup()
{
    pinMode(led_pin, OUTPUT);
    pinMode(input_pin, INPUT_PULLUP);
    digitalWrite(led_pin, LOW);

    Serial.begin(9600);
}

void loop()
{
    while (Serial.available() > 0)
    {
        const char incoming = static_cast<char>(Serial.read());
        apply_led_state(incoming);
    }

    const unsigned long now = millis();
    if (now - last_report_time_ms >= report_interval_ms)
    {
        last_report_time_ms = now;

        const bool button_pressed = (digitalRead(input_pin) == LOW);
        const char output = button_pressed ? '1' : '0';
        Serial.print(output);
    }
}