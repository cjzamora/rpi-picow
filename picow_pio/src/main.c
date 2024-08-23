#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "blink.pio.h"

#define LED_PIN 0

int main() 
{
    // initialize stdio
    stdio_init_all();

    // initialize Wi-Fi
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        return -1;
    }

    sleep_ms(3000);

    // light up onboard LED
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    // use PIO0
    PIO pio = pio0;
    // use state machine 0
    uint sm = 0;
    // add program to PIO0
    uint offset = pio_add_program(pio, &blink_program);

    // initialize LED_PIN
    pio_gpio_init(pio, LED_PIN);
    // set LED_PIN as output
    pio_sm_set_consecutive_pindirs(pio, sm, LED_PIN, 1, true);
    // get program configuration
    pio_sm_config c = blink_program_get_default_config(offset);
    // set pins
    sm_config_set_set_pins(&c, LED_PIN, 1);
    // initialize state machine
    pio_sm_init(pio, sm, offset, &c);
    // enable state machine
    pio_sm_set_enabled(pio, sm, true);

    // set frequency to 3hz
    // this part is still a ???
    pio->txf[sm] = (clock_get_hz(clk_sys) / (2 * 3)) - 3;

    while (true) {
        tight_loop_contents();
    }

    return 0;
}