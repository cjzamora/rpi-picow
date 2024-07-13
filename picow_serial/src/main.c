#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

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

    // input buffer
    char buff[5];
    while (true) {
        if (stdio_usb_connected()) {
            printf("Turn led [on/off]: ");
            scanf("%s", buff);

            // if turn on the LED
            if (strcmp(buff, "on") == 0) {
                printf("\nLED ON\n");
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

            // if turn off the LED
            } else if (strcmp(buff, "off") == 0) {
                printf("\nLED OFF\n");
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

            // if invalid command
            } else {
                printf("\nInvalid command\n");
            }
        } else {
            printf("Waiting for USB connection...\n");
            sleep_ms(1000);
        }
    }

    return 0;
}