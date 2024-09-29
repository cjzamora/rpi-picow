#include <math.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/clocks.h"

#define LED_PIN 16

/**
 * Convert frequency to string
 * 
 * @param freq frequency in Hz
 * @return frequency in kHz, MHz or Hz
 */
char* to_freq(u_int32_t freq) {
    static char str[10];
    if (freq >= 1000 && freq <= 1000000) {
        snprintf(str, sizeof(str), "%ld kHz", freq / 1000);
    } else if (freq >= 1000000) {
        snprintf(str, sizeof(str), "%ld MHz", freq / 1000000);
    } else {
        snprintf(str, sizeof(str), "%ld Hz", freq);
    }

    return str;
}

/**
 * Emulates how PWM works
 * 
 * @return void
 */
void run_pwm() {
    // get sys clock as base
    u_int32_t sys_clk = clock_get_hz(clk_sys);
    // target frequency
    u_int32_t freq = 20;
    // set duty cycle
    u_int32_t duty = 50;

    // calculate divider
    u_int32_t div = ceil((float) sys_clk / (4096 * freq) / 16);
    // calculate wrap
    u_int32_t wrap = sys_clk / div / freq;
    // calculate period for high/low based on duty cycle
    u_int32_t period = wrap * duty / 100;
    // calculate output frequency
    u_int32_t out = sys_clk / div / wrap;

    // calculate ms per whole cycle
    u_int32_t ms = 1000 / freq;
    // calculate ms for high period
    u_int32_t hi = ms * duty / 100;
    // calculate ms for low period
    u_int32_t low = ms - hi;

    printf("out: %s, div %ld, wrap: %ld\n", to_freq(out), div, wrap);

    while(1) {
        // pwm counter counts from 0 to wrap on every
        // clock cycle, for 1hz frequency it will be 1 second
        // and if the duty cycle is 50% then it will be high for
        // 0.5 seconds and low for 0.5 seconds to complete 1 cycle
        for(u_int32_t i = 0; i <= wrap; i++) {
            if (i <= period) {
                gpio_put(LED_PIN, 1);
            } else {
                gpio_put(LED_PIN, 0);
            }

            if (i == period) {
                sleep_ms(hi);
            } else if (i == wrap) {
                sleep_ms(low);
            }
        }
    }
}

/**
 * Emulates how PIO works
 * 
 * @return void
 */
void run_pio() {
    // get sys clock as base
    u_int32_t sys_clk = clock_get_hz(clk_sys);
    // target divider
    u_int32_t div = 62500;

    // let's say our pio instructions has 1928 cycles
    int cycles = 1928;
    // calculate output frequency
    u_int32_t out = sys_clk / div;
    // calculate us per cycle
    u_int32_t us = 1000000 / out;

    while(1) {
        // get start time
        uint64_t start = time_us_32();

        for(int i = 0; i < cycles; i++) {
            sleep_us(us);
        }

        // get end time
        uint64_t end = time_us_32();
        
        // clear
        printf("\033[2J\033[1;1H");
        printf("out: %s, div %ld, us: %ld\n", to_freq(out), div, us);
        // total seconds
        float total = (end - start) / 1000000.0f;
        printf("Time taken: %0.3f seconds\n", total);

        sleep_ms(1000);
    }
}

int main() {
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

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // run_pwm();
    run_pio();
    
    return 0;
}