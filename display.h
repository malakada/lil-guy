#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

// Display dimensions
#define TFT_WIDTH  320
#define TFT_HEIGHT 480

// Colors (RGB565 format)
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_YELLOW  0xFFE0
#define COLOR_BLUE    0x001F
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_ORANGE  0xFD20

// Low-level display functions
void tft_write_command(uint8_t cmd);
void tft_write_data(uint8_t data);
void tft_write_data16(uint16_t data);
void tft_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

// Drawing primitives
void tft_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void tft_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void tft_fill_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

// Sprite buffer for flicker-free rendering
typedef struct {
    uint16_t *buffer;
    uint16_t width;
    uint16_t height;
} sprite_t;

sprite_t* sprite_create(uint16_t width, uint16_t height);
void sprite_free(sprite_t *sprite);
void sprite_fill(sprite_t *sprite, uint16_t color);
void sprite_fill_circle(sprite_t *sprite, int16_t x0, int16_t y0, uint16_t r, uint16_t color);
void sprite_push(sprite_t *sprite, uint16_t x, uint16_t y);

// Initialization
void display_init(void);

#endif // DISPLAY_H
