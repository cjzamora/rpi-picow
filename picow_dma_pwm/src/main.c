
/**
 * @note: Memory alignment __attribute__((aligned(2048)))
 * 
 * NOTES: __attribute__((aligned(x)))
 *
 * Why use memory alignment?
 * 
 * Memory alignment is used to optimize memory access
 * by aligning memory to a specific boundary.
 * 
 * Given an array of 32-bit integers (4 bytes) * 512 elements
 * which is 2048 bytes, we align the memory to 2048 bytes.
 * 
 * The compiler will allocate memory in addresses that is
 * divisible by 2048.
 * 
 * Ex: Unaligned memory
 * 0x1000 - unused
 * 0x1001 - unused
 * 0x1002 - unused
 * 0x1003 - fade_a[0]
 * 0x1004 - fade_a[1]
 * ....
 * 0x2000 - fade_a[512]
 * 
 * Noticed that the memory address is not aligned to 2048
 * meaning that the hardware will require extra steps to access
 * this memory locations. ex. 0x1003 = 4099 / 2048 = not divisible.
 * the hardware will need to access 2 memory locations to get the
 * value of fade_a[0].
 * 
 * Ex: Aligned memory
 * 0x1000 - fade_a[0]
 * 0x1004 - fade_a[1]
 * 0x1008 - fade_a[2]
 * ....
 * 0x2000 - fade_a[512]
 * 
 * Noticed that the memory address is aligned to 2048 and and
 * the addresses is divisible by 2048. the hardware will only
 * need to access 1 memory location to get the value of fade_a[0].
 * also noticed that the address is divisible by 4 since each element
 * holds 4 bytes.
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"

int main() {
    // initialize stdio
    stdio_init_all();

    // initialize Wi-Fi
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        return -1;
    }

    // setup to write to 8 pins
    uint led_pwm_slice_num[8];
    for(int i = 0; i < 8; i++) {
        gpio_set_function(i, GPIO_FUNC_PWM);
        led_pwm_slice_num[i] = pwm_gpio_to_slice_num(i);
    }

    sleep_ms(2000);

    // get default PWM config
    pwm_config config = pwm_get_default_config();
    // set pwm clock divider
    pwm_config_set_clkdiv(&config, 8.f);

    // initialize 4 PWM slices
    pwm_init(0, &config, true);
    pwm_init(1, &config, true);
    pwm_init(2, &config, true);
    pwm_init(3, &config, true);

    // loads increasing/decreasing values to PWM channel a(0)
    int fade_dma_chan_a = dma_claim_unused_channel(true);
    // loads increasing/decreasing values to PWM channel b(1)
    int fade_dma_chan_b = dma_claim_unused_channel(true);

    // writes to fade_dma_chan_a write pointer register
    int control_dma_chan_b = dma_claim_unused_channel(true);
    // writes to fade_dma_chan_b write pointer register
    int control_dma_chan_a = dma_claim_unused_channel(true);

    // declare to wavetables for fade-in and fade-out
    uint32_t fade_a[512] __attribute__((aligned(2048)));
    uint32_t fade_b[512] __attribute__((aligned(2048)));

    // fill the wavetables
    for(int i = 0; i <= 256; i++) {
        uint32_t fade = i * i;

        // make sure fade does not exceed 65535
        fade = fade > 65535 ? 65535 : fade;

        // loads the LSBs of the first half with increasing values
        fade_a[i] = fade;
        // loads the LSBs of the second half with decreasing values
        fade_a[512 - i] = fade;

        // loads the LSBs of the first half with increasing values
        fade_b[i] = fade << 16u;
        // loads the LSBs of the second half with decreasing values
        fade_b[512 - i] = fade << 16u;
    }

    // make sure last element is 0
    fade_a[511] = 0;
    fade_b[511] = 0;

    // set PWM compare counter address locations
    // for each of the PWM slices.
    uint32_t pwm_set_level_locations[4];
    pwm_set_level_locations[0] = 0x4005000c; // pwm_hw->slice[0].cc
    pwm_set_level_locations[1] = 0x40050020; // pwm_hw->slice[1].cc
    pwm_set_level_locations[2] = 0x40050034; // pwm_hw->slice[2].cc
    pwm_set_level_locations[3] = 0x40050048; // pwm_hw->slice[3].cc

    // setup the fade_a DMA channel
    dma_channel_config fade_dma_chan_a_config = dma_channel_get_default_config(fade_dma_chan_a);
    // transfer 32-bits at a time
    channel_config_set_transfer_data_size(&fade_dma_chan_a_config, DMA_SIZE_32);
    // increment read address so we pick up a new fade value each time
    channel_config_set_read_increment(&fade_dma_chan_a_config, true);
    // don't increment write address so we always transfer to the same PWM register
    channel_config_set_write_increment(&fade_dma_chan_a_config, false);
    // after block has been transferred, start control_dma_chan_b
    channel_config_set_chain_to(&fade_dma_chan_a_config, control_dma_chan_b);
    // transfer when the PWM slice asks for a new value
    channel_config_set_dreq(&fade_dma_chan_a_config, DREQ_PWM_WRAP0 + led_pwm_slice_num[0]);
    // set the address wrapping to 512 words (2^11 = 2048 bytes)
    channel_config_set_ring(&fade_dma_chan_a_config, false, 11);

    // setup the fade_b DMA channel
    dma_channel_config fade_dma_chan_b_config = dma_channel_get_default_config(fade_dma_chan_b);
    // transfer 32-bits at a time
    channel_config_set_transfer_data_size(&fade_dma_chan_b_config, DMA_SIZE_32);
    // increment read address so we pick up a new fade value each time
    channel_config_set_read_increment(&fade_dma_chan_b_config, true);
    // don't increment write address so we always transfer to the same PWM register
    channel_config_set_write_increment(&fade_dma_chan_b_config, false);
    // after block has been transferred, start control_dma_chan_a
    channel_config_set_chain_to(&fade_dma_chan_b_config, control_dma_chan_a);
    // transfer when the PWM slice asks for a new value
    channel_config_set_dreq(&fade_dma_chan_b_config, DREQ_PWM_WRAP0 + led_pwm_slice_num[0]);
    // set the address wrapping to 512 words (2^11 = 2048 bytes)
    channel_config_set_ring(&fade_dma_chan_b_config, false, 11);

    // setup the control_dma_chan_a DMA channel
    dma_channel_config control_dma_chan_a_config = dma_channel_get_default_config(control_dma_chan_a);
    // transfer 32-bits at a time
    channel_config_set_transfer_data_size(&control_dma_chan_a_config, DMA_SIZE_32);
    // increment read address so we pick up a new fade value each time
    channel_config_set_read_increment(&control_dma_chan_a_config, true);
    // don't increment write address so we always transfer to the same DMA register
    channel_config_set_write_increment(&control_dma_chan_a_config, false);
    // after block has been transferred, start fade_dma_chan_a
    channel_config_set_chain_to(&control_dma_chan_a_config, fade_dma_chan_a);
    // set the address wrapping to 4 words = (2^4 = 16 bytes)
    channel_config_set_ring(&control_dma_chan_a_config, false, 4);

    // setup the control_dma_chan_b DMA channel
    dma_channel_config control_dma_chan_b_config = dma_channel_get_default_config(control_dma_chan_b);
    // transfer 32-bits at a time
    channel_config_set_transfer_data_size(&control_dma_chan_b_config, DMA_SIZE_32);
    // increment read address so we pick up a new fade value each time
    channel_config_set_read_increment(&control_dma_chan_b_config, true);
    // don't increment write address so we always transfer to the same DMA register
    channel_config_set_write_increment(&control_dma_chan_b_config, false);
    // after block has been transferred, start fade_dma_chan_b
    channel_config_set_chain_to(&control_dma_chan_b_config, fade_dma_chan_b);
    // set the address wrapping to 4 words = (2^4 = 16 bytes)
    channel_config_set_ring(&control_dma_chan_b_config, false, 4);

    // link configuration to fade_dma_chan_a
    dma_channel_configure(
        // DMA channel
        fade_dma_chan_a,
        // DMA config structure
        &fade_dma_chan_a_config,
        // write directly to PWM compare counter register
        &pwm_hw->slice[led_pwm_slice_num[0]].cc,
        // read from fade_a
        fade_a,
        // transfer 512 words (2^11 = 2048 bytes)
        512,
        // dont' start yet
        false
    );

    // link configuration to fade_dma_chan_b
    dma_channel_configure(
        // DMA channel
        fade_dma_chan_b,
        // DMA config structure
        &fade_dma_chan_b_config,
        // write directly to PWM compare counter register
        &pwm_hw->slice[led_pwm_slice_num[0]].cc,
        // read from fade_b
        fade_b, 
        // transfer 512 words (2^11 = 2048 bytes)
        512,
        // don't start yet
        false
    );

    // link configuration to control_dma_chan_a
    dma_channel_configure(
        // DMA channel
        control_dma_chan_a,
        // DMA config structure
        &control_dma_chan_a_config,
        // write PWM .cc register to fade_dma_chan_a
        // alias 2 write address and trigger
        &dma_hw->ch[fade_dma_chan_a].al2_write_addr_trig,
        // read values from the list of PWM compare counter register addresses
        &pwm_set_level_locations[0],
        // 1 word transfer
        1,
        // don't start yet
        false
    );

    // link configuration to control_dma_chan_b
    dma_channel_configure(
        // DMA channel
        control_dma_chan_b,
        // DMA config structure
        &control_dma_chan_b_config,
        // write PWM .cc register to fade_dma_chan_b
        // alias 2 write address and trigger
        &dma_hw->ch[fade_dma_chan_b].al2_write_addr_trig,
        // read values from the list of PWM compare counter register addresses
        &pwm_set_level_locations[0],
        // 1 word transfer
        1,
        // don't start yet
        false
    );

    // tell the control channel to load the first control block.
    // everything is automatic from here...
    dma_start_channel_mask(1u << control_dma_chan_a);

    while (true) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        tight_loop_contents();
    }

    return 0;
}