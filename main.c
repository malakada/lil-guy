/**
 * Lil Guy - Handheld Console Project
 * Hardware: 52Pi Pico Breadboard Kit Plus (EP-0172) + Pico 2W
 */

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

// ===== HARDWARE PIN DEFINITIONS =====

// Display (ST7796SU1) - SPI
#define TFT_SPI         spi0
#define TFT_CLK         2
#define TFT_MOSI        3
#define TFT_CS          5
#define TFT_DC          6
#define TFT_RST         7

// Touch Screen - I2C
#define TOUCH_I2C       i2c0
#define TOUCH_SDA       8
#define TOUCH_SCL       9

// Buzzer
#define BUZZER_PIN      13

// RGB LED
#define RGB_LED_PIN     12

// Buttons
#define BTN1_PIN        15
#define BTN2_PIN        14

// Onboard Joystick (Analog)
#define JOY_ONBOARD_X   26  // ADC0
#define JOY_ONBOARD_Y   27  // ADC1

// Extra Joystick 2 (Digital: 4 directions + button)
#define JOY2_UP         18
#define JOY2_DOWN       19
#define JOY2_LEFT       20
#define JOY2_RIGHT      21
#define JOY2_BTN        22

// Extra Joystick 3 (Digital: 4 directions + button)
#define JOY3_UP         0
#define JOY3_DOWN       1
#define JOY3_LEFT       4
#define JOY3_RIGHT      10
#define JOY3_BTN        11

// Status LEDs
#define LED_D1          16
#define LED_D2          17

// ===== DISPLAY DRIVER FUNCTIONS =====

#define TFT_WIDTH  320
#define TFT_HEIGHT 480

// Colors (RGB565 format)
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_YELLOW  0xFFE0
#define COLOR_BLUE    0x001F

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

void draw_smiley_face() {
    // Clear screen with white background
    tft_fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, COLOR_WHITE);

    // Face (yellow circle in center)
    uint16_t center_x = TFT_WIDTH / 2;
    uint16_t center_y = TFT_HEIGHT / 2;
    uint16_t face_radius = 100;

    tft_fill_circle(center_x, center_y, face_radius, COLOR_YELLOW);

    // Left eye
    tft_fill_circle(center_x - 35, center_y - 30, 10, COLOR_BLACK);

    // Right eye
    tft_fill_circle(center_x + 35, center_y - 30, 10, COLOR_BLACK);

    // Smile (arc made of small circles)
    for (int angle = 20; angle <= 160; angle += 5) {
        float rad = angle * 3.14159 / 180.0;
        int16_t x = center_x + (int16_t)(50 * cos(rad));
        int16_t y = center_y + (int16_t)(50 * sin(rad));
        tft_fill_circle(x, y, 3, COLOR_BLACK);
    }
}

// ===== INITIALIZATION FUNCTIONS =====

void init_display() {
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

    printf("Display initialized\n");
}

void init_touch() {
    // Initialize I2C for touch
    i2c_init(TOUCH_I2C, 400 * 1000); // 400 kHz
    gpio_set_function(TOUCH_SDA, GPIO_FUNC_I2C);
    gpio_set_function(TOUCH_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(TOUCH_SDA);
    gpio_pull_up(TOUCH_SCL);

    printf("Touch I2C initialized\n");
}

void init_buttons() {
    gpio_init(BTN1_PIN);
    gpio_set_dir(BTN1_PIN, GPIO_IN);
    gpio_pull_up(BTN1_PIN);

    gpio_init(BTN2_PIN);
    gpio_set_dir(BTN2_PIN, GPIO_IN);
    gpio_pull_up(BTN2_PIN);

    printf("Buttons initialized\n");
}

void init_buzzer() {
    gpio_init(BUZZER_PIN);
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice_num, true);

    printf("Buzzer initialized\n");
}

void init_rgb_led() {
    gpio_init(RGB_LED_PIN);
    gpio_set_dir(RGB_LED_PIN, GPIO_OUT);

    printf("RGB LED initialized\n");
}

void init_joysticks() {
    // Onboard analog joystick
    adc_init();
    adc_gpio_init(JOY_ONBOARD_X);
    adc_gpio_init(JOY_ONBOARD_Y);

    // Extra joystick 2 (digital)
    gpio_init(JOY2_UP);
    gpio_set_dir(JOY2_UP, GPIO_IN);
    gpio_pull_up(JOY2_UP);

    gpio_init(JOY2_DOWN);
    gpio_set_dir(JOY2_DOWN, GPIO_IN);
    gpio_pull_up(JOY2_DOWN);

    gpio_init(JOY2_LEFT);
    gpio_set_dir(JOY2_LEFT, GPIO_IN);
    gpio_pull_up(JOY2_LEFT);

    gpio_init(JOY2_RIGHT);
    gpio_set_dir(JOY2_RIGHT, GPIO_IN);
    gpio_pull_up(JOY2_RIGHT);

    gpio_init(JOY2_BTN);
    gpio_set_dir(JOY2_BTN, GPIO_IN);
    gpio_pull_up(JOY2_BTN);

    // Extra joystick 3 (digital)
    gpio_init(JOY3_UP);
    gpio_set_dir(JOY3_UP, GPIO_IN);
    gpio_pull_up(JOY3_UP);

    gpio_init(JOY3_DOWN);
    gpio_set_dir(JOY3_DOWN, GPIO_IN);
    gpio_pull_up(JOY3_DOWN);

    gpio_init(JOY3_LEFT);
    gpio_set_dir(JOY3_LEFT, GPIO_IN);
    gpio_pull_up(JOY3_LEFT);

    gpio_init(JOY3_RIGHT);
    gpio_set_dir(JOY3_RIGHT, GPIO_IN);
    gpio_pull_up(JOY3_RIGHT);

    gpio_init(JOY3_BTN);
    gpio_set_dir(JOY3_BTN, GPIO_IN);
    gpio_pull_up(JOY3_BTN);

    printf("Joysticks initialized (1 analog + 2 digital)\n");
}

void init_status_leds() {
    gpio_init(LED_D1);
    gpio_set_dir(LED_D1, GPIO_OUT);

    gpio_init(LED_D2);
    gpio_set_dir(LED_D2, GPIO_OUT);

    printf("Status LEDs initialized\n");
}

// ===== MAIN =====

int main() {
    // Initialize stdio
    stdio_init_all();
    sleep_ms(2000); // Wait for USB serial

    printf("\n=== Lil Guy Starting ===\n");

    // Initialize WiFi
    if (cyw43_arch_init()) {
        printf("WiFi init failed\n");
        return -1;
    }
    printf("WiFi initialized\n");

    // Initialize all hardware
    init_display();
    init_touch();
    init_buttons();
    init_buzzer();
    init_rgb_led();
    init_joysticks();
    init_status_leds();

    printf("=== Hardware Ready ===\n\n");

    // Draw smiley face on display
    draw_smiley_face();
    printf("Smiley face drawn!\n");

    // Main loop - test all inputs
    while (true) {
        // Blink LEDs to show we're alive
        gpio_put(LED_D1, 1);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(250);

        gpio_put(LED_D1, 0);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(250);

        // Read onboard analog joystick
        adc_select_input(0);
        uint16_t joy_x = adc_read();
        adc_select_input(1);
        uint16_t joy_y = adc_read();

        // Read all buttons and joysticks (digital inputs read LOW when pressed)
        printf("Analog Joy: X=%4d Y=%4d | Buttons: BTN1=%d BTN2=%d\n",
               joy_x, joy_y,
               !gpio_get(BTN1_PIN),
               !gpio_get(BTN2_PIN));

        printf("Joy2: U=%d D=%d L=%d R=%d BTN=%d | Joy3: U=%d D=%d L=%d R=%d BTN=%d\n",
               !gpio_get(JOY2_UP), !gpio_get(JOY2_DOWN), !gpio_get(JOY2_LEFT), !gpio_get(JOY2_RIGHT), !gpio_get(JOY2_BTN),
               !gpio_get(JOY3_UP), !gpio_get(JOY3_DOWN), !gpio_get(JOY3_LEFT), !gpio_get(JOY3_RIGHT), !gpio_get(JOY3_BTN));

        printf("\n");
    }

    return 0;
}
