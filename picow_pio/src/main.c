#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/pio.h"
#include "program.pio.h"

#define LED_PIN 16
#define LED2_PIN 17

int main() 
{
    // initialize stdio
    stdio_init_all();

    // initialize Wi-Fi
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        return -1;
    }

    sleep_ms(2000);
    // light up onboard LED
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    gpio_init(LED2_PIN);
    gpio_set_dir(LED2_PIN, GPIO_IN);

    // use pio 0 (there are two pio blocks on the RP2040)
    PIO pio = pio0;
    // claim unused state machine
    uint sm = pio_claim_unused_sm(pio, true);

    // add program and get the program offset
    uint offset = pio_add_program(pio, &program_program);
    // get default config (side set pins, clock divider, etc.)
    pio_sm_config config = program_program_get_default_config(offset);

    // initialize pio to the gpio
    pio_gpio_init(pio, LED_PIN);

    // map the state machines `SET pin` group to one pin
    sm_config_set_set_pins(&config, LED_PIN, 1);
    // set the pin direction to output from LED_PIN to pin_count
    pio_sm_set_consecutive_pindirs(pio, sm, LED_PIN, 1, true);

    // set the state machine clock rate
    sm_config_set_clkdiv(&config, 62500);

    // load the program and jump to the start of the program
    pio_sm_init(pio, sm, offset, &config);
    // set the state machine to enabled
    pio_sm_set_enabled(pio, sm, true);

    while (true) {
        tight_loop_contents();
    }

    return 0;
}