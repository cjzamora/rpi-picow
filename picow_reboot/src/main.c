#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/bootrom.h"

void print_welcome() {
    printf("\033[2J\033[H");
    printf("Welcome to Basic RPI Pico Serial Monitor!\n\n");
    printf("Commands:\n");
    printf("  on: Turn on the LED\n");
    printf("  off: Turn off the LED\n");
    printf("  clr: Clear the screen\n");
    printf("  bsel: Reboot to BOOTSEL mode\n");
    printf("\n");
}

int main() {
    // initialize stdio
    stdio_init_all();

    // initialize Wi-Fi
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        return -1;
    }

    // FIX: clear buffer before reading input, 
    // otherwise it will read EOF 
    while(getchar() == EOF);
    
    bool welcome = false;
    // input buffer
    char buff[20];
    while (true) {
        if (stdio_usb_connected()) {
            if (!welcome) {
                welcome = true;
                print_welcome();
            }

            scanf("%s", buff);
            printf("> %s\n", buff);

            // if turn on the LED
            if (strcmp(buff, "on") == 0) {
                printf("LED ON\n");
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

            // if turn off the LED
            } else if (strcmp(buff, "off") == 0) {
                printf("LED OFF\n");
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

            // if clear the screen
            } else if (strcmp(buff, "clr") == 0) {
                print_welcome();

            // if reboot to BOOTSEL mode
            } else if (strcmp(buff, "bsel") == 0) {
                printf("BOOTSEL\n");
                reset_usb_boot(0, 0);

            // if invalid command
            } else {
                printf("INVALID\n");
            }
        } else {
            printf("CONNECTING...\n");
            sleep_ms(1000);
        }
    }

    return 0;
}