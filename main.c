/**
 * Lil Guy - Handheld Console Project
 * Hardware: 52Pi Pico Breadboard Kit Plus (EP-0172) + Pico 2W
 */

#include <stdio.h>
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

    printf("Display SPI initialized\n");
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
