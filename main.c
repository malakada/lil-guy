/**
 * Lil Guy - Handheld Console Project
 * Hardware: 52Pi Pico Breadboard Kit Plus (EP-0172) + Pico 2W
 */

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "display.h"

// ===== HARDWARE PIN DEFINITIONS =====

// Touch Screen - I2C (GT911)
#define TOUCH_I2C       i2c0
#define TOUCH_SDA       8
#define TOUCH_SCL       9
#define GT911_ADDR      0x5D
#define GT911_STATUS    0x814E
#define GT911_POINT1    0x814F

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

// ===== HELPER FUNCTIONS =====

void play_tone(uint frequency_hz, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);

    // Set PWM frequency
    uint32_t clock = 125000000; // 125 MHz system clock
    uint32_t divider = clock / (frequency_hz * 4096);
    pwm_set_clkdiv(slice_num, divider);

    // Set 50% duty cycle
    pwm_set_wrap(slice_num, 4095);
    pwm_set_gpio_level(BUZZER_PIN, 2048);

    // Enable PWM
    pwm_set_enabled(slice_num, true);
    sleep_ms(duration_ms);

    // Disable PWM (silence)
    pwm_set_enabled(slice_num, false);
}

// ===== SCREEN DRAWING FUNCTIONS =====

void draw_smiley_face_to_sprite(sprite_t *sprite, bool is_happy, uint16_t face_color) {
    // Clear sprite with white background
    sprite_fill(sprite, COLOR_WHITE);

    // Face (colored circle at center of sprite)
    uint16_t center_x = sprite->width / 2;
    uint16_t center_y = sprite->height / 2;
    uint16_t face_radius = 100;

    sprite_fill_circle(sprite, center_x, center_y, face_radius, face_color);

    // Left eye
    sprite_fill_circle(sprite, center_x - 35, center_y - 30, 10, COLOR_BLACK);

    // Right eye
    sprite_fill_circle(sprite, center_x + 35, center_y - 30, 10, COLOR_BLACK);

    // Mouth (happy smile or sad frown)
    if (is_happy) {
        // Smile (arc made of small circles)
        for (int angle = 20; angle <= 160; angle += 5) {
            float rad = angle * 3.14159 / 180.0;
            int16_t x = center_x + (int16_t)(50 * cos(rad));
            int16_t y = center_y + (int16_t)(50 * sin(rad));
            sprite_fill_circle(sprite, x, y, 3, COLOR_BLACK);
        }
    } else {
        // Frown (inverted arc)
        for (int angle = 200; angle <= 340; angle += 5) {
            float rad = angle * 3.14159 / 180.0;
            int16_t x = center_x + (int16_t)(50 * cos(rad));
            int16_t y = center_y + 20 + (int16_t)(50 * sin(rad));
            sprite_fill_circle(sprite, x, y, 3, COLOR_BLACK);
        }
    }
}

// ===== INITIALIZATION FUNCTIONS =====

void init_touch() {
    // Initialize I2C for touch
    i2c_init(TOUCH_I2C, 400 * 1000); // 400 kHz
    gpio_set_function(TOUCH_SDA, GPIO_FUNC_I2C);
    gpio_set_function(TOUCH_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(TOUCH_SDA);
    gpio_pull_up(TOUCH_SCL);

    printf("Touch GT911 initialized\n");
}

bool read_touch(uint16_t *x, uint16_t *y) {
    // Read GT911 status register
    uint8_t reg[2] = {(GT911_STATUS >> 8) & 0xFF, GT911_STATUS & 0xFF};
    uint8_t status;

    // Write register address
    if (i2c_write_blocking(TOUCH_I2C, GT911_ADDR, reg, 2, true) < 0) {
        return false;
    }

    // Read status
    if (i2c_read_blocking(TOUCH_I2C, GT911_ADDR, &status, 1, false) < 0) {
        return false;
    }

    // Check if touch detected (bit 7 = buffer status, bits 0-3 = touch points)
    uint8_t touch_points = status & 0x0F;
    if (touch_points == 0) {
        return false; // No touch
    }

    // Read touch point data
    reg[0] = (GT911_POINT1 >> 8) & 0xFF;
    reg[1] = GT911_POINT1 & 0xFF;

    if (i2c_write_blocking(TOUCH_I2C, GT911_ADDR, reg, 2, true) < 0) {
        return false;
    }

    uint8_t data[6];
    if (i2c_read_blocking(TOUCH_I2C, GT911_ADDR, data, 6, false) < 0) {
        return false;
    }

    // Parse coordinates (little endian)
    *x = data[0] | (data[1] << 8);
    *y = data[2] | (data[3] << 8);

    // Clear status register
    uint8_t clear_buf[3] = {(GT911_STATUS >> 8) & 0xFF, GT911_STATUS & 0xFF, 0};
    i2c_write_blocking(TOUCH_I2C, GT911_ADDR, clear_buf, 3, false);

    return true;
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
    display_init();
    init_touch();
    init_buttons();
    init_buzzer();
    init_rgb_led();
    init_joysticks();
    init_status_leds();

    printf("=== Hardware Ready ===\n\n");

    // Create sprite buffer for smiley (220x220 to fit face + padding)
    const uint16_t sprite_size = 220;
    sprite_t *smiley_sprite = sprite_create(sprite_size, sprite_size);
    if (!smiley_sprite) {
        printf("Failed to create sprite buffer!\n");
        return -1;
    }

    // Clear screen once
    tft_fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, COLOR_WHITE);

    // Smiley face state variables
    bool is_happy = true;
    uint8_t color_index = 0;
    uint16_t rainbow_colors[] = {COLOR_YELLOW, COLOR_RED, COLOR_ORANGE, COLOR_GREEN, COLOR_CYAN, COLOR_BLUE, COLOR_MAGENTA};
    uint8_t num_colors = 7;

    // Position tracking (start at center)
    int16_t smiley_x = TFT_WIDTH / 2 - sprite_size / 2;
    int16_t smiley_y = TFT_HEIGHT / 2 - sprite_size / 2;
    int16_t old_x = smiley_x;
    int16_t old_y = smiley_y;
    const int16_t face_radius = 100;

    // Button state tracking for debouncing
    bool btn1_last = false;
    bool btn2_last = false;

    // Touch state tracking
    bool touch_active = false;
    uint32_t last_color_change = 0;

    // Draw initial smiley face
    draw_smiley_face_to_sprite(smiley_sprite, is_happy, rainbow_colors[color_index]);
    sprite_push(smiley_sprite, smiley_x, smiley_y);
    printf("Smiley face drawn! BTN1=toggle happy/sad, BTN2=change color, Joystick=move, Touch=cycle colors\n");

    // Main loop
    while (true) {
        // Blink LEDs to show we're alive
        gpio_put(LED_D1, 1);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(25);

        gpio_put(LED_D1, 0);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(25);

        // Read button states (buttons read LOW when pressed)
        bool btn1_pressed = !gpio_get(BTN1_PIN);
        bool btn2_pressed = !gpio_get(BTN2_PIN);

        bool needs_redraw = false;

        // BTN1: Toggle happy/sad on button press (edge detection)
        if (btn1_pressed && !btn1_last) {
            is_happy = !is_happy;
            needs_redraw = true;
            // Play a very gentle, low tone (200 Hz for 80ms - much softer)
            play_tone(200, 80);
            printf("Toggled mood: %s\n", is_happy ? "Happy :)" : "Sad :(");
        }
        btn1_last = btn1_pressed;

        // BTN2: Cycle through colors on button press (edge detection)
        if (btn2_pressed && !btn2_last) {
            color_index = (color_index + 1) % num_colors;
            needs_redraw = true;
            printf("Changed color to index %d\n", color_index);
        }
        btn2_last = btn2_pressed;

        // Touch: Cycle through colors fluidly while held
        uint16_t touch_x, touch_y;
        bool is_touched = read_touch(&touch_x, &touch_y);

        if (is_touched) {
            // Blink D2 LED to show touch is detected
            gpio_put(LED_D2, 1);

            // Cycle colors every 150ms while holding touch
            uint32_t now = to_ms_since_boot(get_absolute_time());
            if (now - last_color_change > 150) {
                color_index = (color_index + 1) % num_colors;
                needs_redraw = true;
                last_color_change = now;
            }
            touch_active = true;
        } else {
            gpio_put(LED_D2, 0);
            touch_active = false;
        }

        // Read onboard analog joystick
        adc_select_input(0);
        uint16_t joy_x = adc_read();
        adc_select_input(1);
        uint16_t joy_y = adc_read();

        // Joystick movement (ADC values are 0-4095, center ~2048)
        // Dead zone to avoid drift
        const int16_t dead_zone = 200;
        const int16_t center = 2048;

        int16_t dx = 0;
        int16_t dy = 0;

        // Note: X controls vertical, Y controls horizontal (rotated 90 degrees)
        if (joy_y < center - dead_zone) {
            dx = ((center - joy_y) / 100);  // Move left
        } else if (joy_y > center + dead_zone) {
            dx = -((joy_y - center) / 100);   // Move right
        }

        if (joy_x < center - dead_zone) {
            dy = -((center - joy_x) / 100);  // Move up
        } else if (joy_x > center + dead_zone) {
            dy = ((joy_x - center) / 100);   // Move down
        }

        // Update position with boundary checking
        if (dx != 0 || dy != 0) {
            int16_t new_x = smiley_x + dx;
            int16_t new_y = smiley_y + dy;

            // Keep smiley sprite on screen
            if (new_x < 0) new_x = 0;
            if (new_x > TFT_WIDTH - sprite_size) new_x = TFT_WIDTH - sprite_size;
            if (new_y < 0) new_y = 0;
            if (new_y > TFT_HEIGHT - sprite_size) new_y = TFT_HEIGHT - sprite_size;

            smiley_x = new_x;
            smiley_y = new_y;
            needs_redraw = true;
        }

        // Redraw if anything changed
        if (needs_redraw) {
            // Erase old position if moved
            if (old_x != smiley_x || old_y != smiley_y) {
                tft_fill_rect(old_x, old_y, sprite_size, sprite_size, COLOR_WHITE);
            }

            // Redraw sprite to buffer and push to screen
            draw_smiley_face_to_sprite(smiley_sprite, is_happy, rainbow_colors[color_index]);
            sprite_push(smiley_sprite, smiley_x, smiley_y);

            old_x = smiley_x;
            old_y = smiley_y;
        }
    }

    return 0;
}
