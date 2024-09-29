#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#define RS 10
#define E 11
#define D4 12
#define D5 13
#define D6 14
#define D7 15

void lcd_init();
void lcd_send_command(uint8_t command);
void lcd_send_data(uint8_t data);
void lcd_clear();
void lcd_set_cursor(uint8_t row, uint8_t col);
void lcd_print(const char *str);
void pulse_enable();

/**
 * Initialize the LCD display
 * 
 * @return void
 */
void lcd_init() {
    // Initialize GPIO pins
    gpio_init(RS);
    gpio_init(E);
    gpio_init(D4);
    gpio_init(D5);
    gpio_init(D6);
    gpio_init(D7);

    // Set GPIO pins as output
    gpio_set_dir(RS, GPIO_OUT);
    gpio_set_dir(E, GPIO_OUT);
    gpio_set_dir(D4, GPIO_OUT);
    gpio_set_dir(D5, GPIO_OUT);
    gpio_set_dir(D6, GPIO_OUT);
    gpio_set_dir(D7, GPIO_OUT);

    // Initialization sequence
    sleep_ms(15); // Wait for power on
    lcd_send_command(0x03);  // Set to 8-bit mode
    sleep_ms(5);
    lcd_send_command(0x03);
    sleep_ms(1);
    lcd_send_command(0x03);

    lcd_send_command(0x02);  // Switch to 4-bit mode

    // Configure LCD function
    lcd_send_command(0x28);  // 4-bit mode, 2 lines, 5x8 font
    lcd_send_command(0x0C);  // Display on, cursor off, blink off
    lcd_send_command(0x06);  // Increment cursor
    lcd_clear();  // Clear display
}

/**
 * Send a command to the LCD display
 * 
 * @param uint8_t command
 * @return void
 */
void lcd_send_command(uint8_t command) {
    gpio_put(RS, 0);  // Command mode
    lcd_send_data(command);
}

/**
 * Send data to the LCD display
 * 
 * @param uint8_t data
 * @return void
 */
void lcd_send_data(uint8_t data) {
    gpio_put(D4, (data >> 4) & 0x01);
    gpio_put(D5, (data >> 4) & 0x02);
    gpio_put(D6, (data >> 4) & 0x04);
    gpio_put(D7, (data >> 4) & 0x08);
    pulse_enable();

    gpio_put(D4, data & 0x01);
    gpio_put(D5, data & 0x02);
    gpio_put(D6, data & 0x04);
    gpio_put(D7, data & 0x08);
    pulse_enable();
}

/**
 * Clear the LCD display
 * 
 * @return void
 */
void lcd_clear() {
    lcd_send_command(0x01);
    sleep_ms(2);
}

/**
 * Set the cursor position
 * 
 * @param uint8_t row
 * @param uint8_t col
 * @return void
 */
void lcd_set_cursor(uint8_t row, uint8_t col) {
    uint8_t address = (row == 0) ? col : col + 0x40;
    lcd_send_command(0x80 | address);
}

/**
 * Print a string to the LCD display
 * 
 * @param const char *str
 * @return void
 */
void lcd_print(const char *str) {
    while (*str) {
        gpio_put(RS, 1);  // Data mode
        lcd_send_data(*str++);
    }
}

/**
 * Pulse the enable pin
 * 
 * @return void
 */
void pulse_enable() {
    gpio_put(E, 1);
    sleep_us(1);
    gpio_put(E, 0);
    sleep_us(100);
}

int main() {
    // initialize stdio
    stdio_init_all();

    // initialize Wi-Fi
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        return -1;
    }

    sleep_ms(1000);

    char buffer[50];
    int n = 0;

    lcd_init();

    while (true) {
        sprintf(buffer, "Hello, World! %d", n++);
        lcd_clear();
        lcd_print(buffer);

        // turn on the LED
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(500);

        // turn off the LED
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(500);
    }

    return 0;
}