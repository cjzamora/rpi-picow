#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"

#define LED_PIN 16

int main() {
    // initialize stdio
    stdio_init_all();

    // initialize Wi-Fi
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        return -1;
    }

    // initialize led pin to use PWM
    gpio_set_function(LED_PIN, GPIO_FUNC_PWM);

    // get pwm slice num
    uint slice_num = pwm_gpio_to_slice_num(LED_PIN);
    // get default config
    pwm_config config = pwm_get_default_config();
    // set clock divider to 8 (125 Mhz / 8 = 15.625 Mhz)
    pwm_config_set_clkdiv(&config, 8.f);
    // initialize PWM
    pwm_init(slice_num, &config, true);

    // holds the fade buffer, should be 32-bit aligned 
    // since we are using 32-bit transfer size
    uint32_t fade[256];
    // fill the fade buffer
    for(int i = 0; i < 256; i++) {
        // use quadratic function to get non-linear fade effect
        // which results in a more gradual and smooth fade effect

        // calculate our scaling factor using quadratic function
        // this allows us to generate values that are non-linear
        // but will still fit into the 16-bit PWM counter resolution
        // and our buffer size of 256
        float scale = 65535.0f / (255.0f * 255.0f);
        // calculate the fade value
        fade[i] = (uint32_t)(i * i * scale);
    }

    // initialize DMA channel
    int dma_channel = dma_claim_unused_channel(true);
    // get default dma config
    dma_channel_config dma_config = dma_channel_get_default_config(dma_channel);
    // set transfer data size to 32 bits
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);
    // set read increment to true, this will increment the read address after each transfer
    // our read address will start from the fade buffer address and will increment by 4 bytes
    // (32-bit) after each transfer. We need to enable this since we are reading from the fade buffer
    channel_config_set_read_increment(&dma_config, true);
    // set write increment to false, this will keep the write address constant
    // since we are writing to the PWM slice cc register, we don't need to increment the write address
    channel_config_set_write_increment(&dma_config, false);
    // transfer when PWM slice that is connected to LED_PIN ask for a new value
    channel_config_set_dreq(&dma_config, DREQ_PWM_WRAP0 + slice_num);

    while (true) {
        // reset dma channel every 256 transfers
        if (dma_channel_is_busy(dma_channel)) {
            dma_channel_wait_for_finish_blocking(dma_channel);
        }

        // reset the dma channel
        dma_channel_configure(
            dma_channel, // channel
            &dma_config, // config
            &pwm_hw->slice[slice_num].cc, // Write directly to PWM cc register (cc = counter compare)
            fade, // src
            256, // transfer count
            true // start immediately
        );

        sleep_ms(3000);
        tight_loop_contents();
    }

    return 0;
}