#include "display.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include <stdlib.h>
#include <string.h>

// Display pin definitions
#define TFT_SPI         spi0
#define TFT_CLK         2
#define TFT_MOSI        3
#define TFT_CS          5
#define TFT_DC          6
#define TFT_RST         7

void tft_write_command(uint8_t cmd) {
    gpio_put(TFT_DC, 0);
    gpio_put(TFT_CS, 0);
    spi_write_blocking(TFT_SPI, &cmd, 1);
    gpio_put(TFT_CS, 1);
}

void tft_write_data(uint8_t data) {
    gpio_put(TFT_DC, 1);
    gpio_put(TFT_CS, 0);
    spi_write_blocking(TFT_SPI, &data, 1);
    gpio_put(TFT_CS, 1);
}

void tft_write_data16(uint16_t data) {
    uint8_t buf[2] = {data >> 8, data & 0xFF};
    gpio_put(TFT_DC, 1);
    gpio_put(TFT_CS, 0);
    spi_write_blocking(TFT_SPI, buf, 2);
    gpio_put(TFT_CS, 1);
}

void tft_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    tft_write_command(0x2A); // Column address set
    tft_write_data16(x0);
    tft_write_data16(x1);

    tft_write_command(0x2B); // Row address set
    tft_write_data16(y0);
    tft_write_data16(y1);

    tft_write_command(0x2C); // Memory write
}

void tft_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    tft_set_window(x, y, x + w - 1, y + h - 1);

    gpio_put(TFT_DC, 1);
    gpio_put(TFT_CS, 0);

    uint8_t buf[2] = {color >> 8, color & 0xFF};
    for (uint32_t i = 0; i < w * h; i++) {
        spi_write_blocking(TFT_SPI, buf, 2);
    }

    gpio_put(TFT_CS, 1);
}

void tft_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    // Draw circle using Bresenham's algorithm
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        // Draw 8 octants
        tft_fill_rect(x0 + x, y0 + y, 1, 1, color);
        tft_fill_rect(x0 - x, y0 + y, 1, 1, color);
        tft_fill_rect(x0 + x, y0 - y, 1, 1, color);
        tft_fill_rect(x0 - x, y0 - y, 1, 1, color);
        tft_fill_rect(x0 + y, y0 + x, 1, 1, color);
        tft_fill_rect(x0 - y, y0 + x, 1, 1, color);
        tft_fill_rect(x0 + y, y0 - x, 1, 1, color);
        tft_fill_rect(x0 - y, y0 - x, 1, 1, color);
    }
}

void tft_fill_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    tft_fill_rect(x0 - r, y0, 2 * r + 1, 1, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        tft_fill_rect(x0 - x, y0 + y, 2 * x + 1, 1, color);
        tft_fill_rect(x0 - x, y0 - y, 2 * x + 1, 1, color);
        tft_fill_rect(x0 - y, y0 + x, 2 * y + 1, 1, color);
        tft_fill_rect(x0 - y, y0 - x, 2 * y + 1, 1, color);
    }
}

// ===== SPRITE BUFFER FUNCTIONS =====

sprite_t* sprite_create(uint16_t width, uint16_t height) {
    sprite_t *sprite = malloc(sizeof(sprite_t));
    if (!sprite) return NULL;

    sprite->width = width;
    sprite->height = height;
    sprite->buffer = malloc(width * height * sizeof(uint16_t));

    if (!sprite->buffer) {
        free(sprite);
        return NULL;
    }

    return sprite;
}

void sprite_free(sprite_t *sprite) {
    if (sprite) {
        if (sprite->buffer) free(sprite->buffer);
        free(sprite);
    }
}

void sprite_fill(sprite_t *sprite, uint16_t color) {
    for (uint32_t i = 0; i < sprite->width * sprite->height; i++) {
        sprite->buffer[i] = color;
    }
}

void sprite_fill_circle(sprite_t *sprite, int16_t x0, int16_t y0, uint16_t r, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    // Helper to set pixel in sprite buffer
    auto void set_pixel(int16_t px, int16_t py) {
        if (px >= 0 && px < sprite->width && py >= 0 && py < sprite->height) {
            sprite->buffer[py * sprite->width + px] = color;
        }
    }

    // Fill horizontal line
    for (int16_t i = x0 - r; i <= x0 + r; i++) {
        set_pixel(i, y0);
    }

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        // Fill horizontal lines for filled circle
        for (int16_t i = x0 - x; i <= x0 + x; i++) {
            set_pixel(i, y0 + y);
            set_pixel(i, y0 - y);
        }
        for (int16_t i = x0 - y; i <= x0 + y; i++) {
            set_pixel(i, y0 + x);
            set_pixel(i, y0 - x);
        }
    }
}

void sprite_push(sprite_t *sprite, uint16_t x, uint16_t y) {
    tft_set_window(x, y, x + sprite->width - 1, y + sprite->height - 1);

    gpio_put(TFT_DC, 1);
    gpio_put(TFT_CS, 0);

    // Send sprite buffer to display
    for (uint32_t i = 0; i < sprite->width * sprite->height; i++) {
        uint8_t buf[2] = {sprite->buffer[i] >> 8, sprite->buffer[i] & 0xFF};
        spi_write_blocking(TFT_SPI, buf, 2);
    }

    gpio_put(TFT_CS, 1);
}

// ===== DISPLAY INITIALIZATION =====

void display_init(void) {
    // Initialize SPI for display
    spi_init(TFT_SPI, 62500 * 1000); // 62.5 MHz
    gpio_set_function(TFT_CLK, GPIO_FUNC_SPI);
    gpio_set_function(TFT_MOSI, GPIO_FUNC_SPI);

    // CS, DC, RST as outputs
    gpio_init(TFT_CS);
    gpio_set_dir(TFT_CS, GPIO_OUT);
    gpio_put(TFT_CS, 1);

    gpio_init(TFT_DC);
    gpio_set_dir(TFT_DC, GPIO_OUT);

    gpio_init(TFT_RST);
    gpio_set_dir(TFT_RST, GPIO_OUT);
    gpio_put(TFT_RST, 1);

    // Hardware reset
    gpio_put(TFT_RST, 0);
    sleep_ms(10);
    gpio_put(TFT_RST, 1);
    sleep_ms(120);

    // ST7796 initialization sequence
    tft_write_command(0x01); // Software reset
    sleep_ms(120);

    tft_write_command(0x11); // Sleep out
    sleep_ms(120);

    tft_write_command(0x3A); // Pixel format
    tft_write_data(0x55);    // 16-bit color

    tft_write_command(0x29); // Display on
}
