#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"

void core1_main() {
    while(true) {
        printf("Hello from Core 1\n");
        sleep_ms(1000);
    }
}

int main() {
    // initialize the standard io
    stdio_init_all();

    // initialize Wi-Fi
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        return -1;
    }

    // launch core 1
    multicore_launch_core1(core1_main);

    while(true) {
        printf("Hello from Core 0\n");
        sleep_ms(1000);
    }

    return 0;
}