/**
 * @brief This example demonstrates how PWM and DMA works together
 * 
 * DMA: we are using the DMA to transfer data from the fade buffer
 * to the PWM slice's CC register. The DMA will increment the read
 * address after each transfer.
 * 
 * PWM: we are using the PWM to fade the LED. The PWM will read the
 * CC register value and set the duty cycle accordingly.
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/pwm.h"

#define LED_PIN 16

int main() 
{
    // initialize stdio
    stdio_init_all();

    // initialize Wi-Fi
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        return -1;
    }

    // initialize led pin to use PWM
    gpio_set_function(LED_PIN, GPIO_FUNC_PWM);

    // get slice num
    uint slice_num = pwm_gpio_to_slice_num(LED_PIN);
    // get channel number
    uint channel_num = pwm_gpio_to_channel(LED_PIN);

    // set clock div to 256 this will give us 125 Mhz / 256 = 488.28 kHz
    pwm_set_clkdiv(slice_num, 256);
    // set wrap to 65535 this will give us 125 Mhz / 65535 / 256 = 7.63 Hz
    pwm_set_wrap(slice_num, 65535);
    // set the duty cycle to 50%
    pwm_set_chan_level(slice_num, channel_num, 32768);
    // enable PWM
    pwm_set_enabled(slice_num, true);
    
    while (true) {
        tight_loop_contents();
    }

    return 0;
}