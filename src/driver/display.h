#pragma once

#include <stdint.h>
#include "esp_err.h"

// SSD1306 128x64 monochrome OLED at I2C address 0x3C

#define DISPLAY_I2C_ADDRESS 0x3C
#define DISPLAY_WIDTH       128
#define DISPLAY_HEIGHT      64
#define DISPLAY_LINES       (DISPLAY_HEIGHT / 8)

esp_err_t display_init(void);

// Clears the local framebuffer (call display_update() to push it).
void display_clear(void);

// Draws text with a 5x7 font into the framebuffer. `line` is 0..7 (8 px rows),
// `x` is the pixel column. Text past the right edge is clipped.
void display_draw_text(uint8_t line, uint8_t x, const char *text);

// Sends the framebuffer to the panel.
esp_err_t display_update(void);
