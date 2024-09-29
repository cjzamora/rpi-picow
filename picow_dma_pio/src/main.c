/**
 * @brief This example demonstrates how DMA and PIO works together
 *
 * DMA: we are using the DMA to transfer data from the 
 * wavetable address into the PIO's TX FIFO when it detects
 * the data request signal (DREQ_PIO0_TX0) from the PIO.
 *
 * PIO: the pio has a TX FIFO which acts as a buffer for the
 * data that will be shifted out to the LED pin. The state machine
 * shifts out bits from the FIFO into the OSR (output shift register)
 * and then to the GPIO pin.
 *
 * Summary:
 * 
 * 1. DMA Transfers Data to FIFO
 *    - DMA transfers data from the wavetable to the PIO's TX FIFO
 * 2. PIO State Machine
 *    - PIO state machine reads data from the TX FIFO into the OSR
 *      and then shifts it out to the GPIO pin
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "dma_pio.pio.h"

// define the LED pin
#define LED_PIN 16
// state machine will run at 125 Mhz / 10 = 12.5 Mhz
#define PIO_CLK_DIV 10.f
// number of samples to transfer
#define DMA_TRANSFER_SIZE 10000
// number of PWM levels
#define PWM_LEVELS 32

int dma_channel;

/**
 * Initialize the DMA PIO program, this can be placed in dma_pio.pio.h
 * 
 * @param pio - pointer to the PIO instance
 * @param sm - state machine number
 * @param offset - offset of the PIO program
 * @param pin - pin number
 * @param clk_div - clock divider
 * 
 * @return void
 */
void dma_pio_program_init(PIO pio, uint sm, uint offset, uint pin, float clk_div) {
    // initialize led pin to use PIO
    pio_gpio_init(pio, pin);
    // set the direction of the pin to output
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
    // get the dma pio program default config (see: dma_pio.pio.h)
    pio_sm_config c = dma_pio_program_get_default_config(offset);

    // configure state machine
    // set out pins
    sm_config_set_out_pins(&c, pin, 1);
    // join FIFO as TX fifo (we have 2 FIFOs, TX and RX)
    // each has 4 slots, 32-bits each, so we now have 8 slots
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    // set the clock divider
    sm_config_set_clkdiv(&c, clk_div);
    // set out shift direction
    // * config - pointer to the state machine configuration
    // * shift_right: true if the shift direction is right (LSB first), false if left (MSB first)
    // * autopull: true if the shift register should automatically pull in new data from the FIFO
    // * pull_treshold: tells the state machine to automatically pull a new word from the FIFO into the OSR
    //                  when the number of bits left in the OSR drops below this threshold
    sm_config_set_out_shift(&c, true, true, 32);

    // initialize pio state machine
    pio_sm_init(pio, sm, offset, &c);
    // enable pio state machine
    pio_sm_set_enabled(pio, sm, true);
}

/**
 * DMA interrupt request handler
 * 
 * This function gets called when the DMA channel is done transferring
 * data which is set to 32-bits at a time. This function will then set
 * the read address to the wavetable and increment the pwm level.
 * 
 * The reason why we set the read_increment to false is because we want
 * to keep the read address constant and only increment the pwm level
 * each time the DMA transfer is done.
 * 
 * @return void
 */
void dma_handler() {
    // pwm level index
    static int pwm_level = 0;
    // wavetable data from lowest to highest
    static uint32_t wavetable[PWM_LEVELS];
    // first run flag
    static bool first_run = true;

    // if first run, generate the wavetable
    if (first_run) {
        first_run = false;

        //
        // Generate the wavetable (emulates PWM levels)
        //
        // this part is really clever, for us to be able to
        // generate a gradual fade effect with PIO and DMA we
        // need to generate a bit pattern that graudually increases
        // the number of bits set to 1 starting from the lowest bit
        // 
        // ex: (note that DMA transfers 32-bits at a time)
        // 
        // 00000000000000000000000000000000 = 0 (off for 31 cycles)
        // 00000000000000000000000000000001 = 1 (on for 1 cycle)
        // 00000000000000000000000000000011 = 3 (on for 2 cycles)
        // 00000000000000000000000000000111 = 7
        // ....
        // 00000000000000011111111111111111 = 65535 (50% duty cycle, on for half of the cycles)
        // ....
        // 01111111111111111111111111111111 = 2147483647 (100% duty cycle, on for all cycles)
        for(int i = 0; i < PWM_LEVELS; i++) {
            wavetable[i] = ~(~0u << i);
            printf("wavetable[%d] = %lu\n", i, wavetable[i]);
        } 
    }

    // by default ints0 = 0, meaning no interrupts are pending
    // when the DMA transfer completes, the DMA channel will
    // set the ints0 bit to 1 for the corresponding channel,
    // signaling an interrupt. To clear the flag, we need to write
    // to it again. haha! this is a bit confusing but that's how it works.
    // but basically it uses the write-to-clear mechanism.
    //
    // because writing 0 can accidentally clear other interrupts
    // so writting 1 given the channel number will clear the interrupt
    // for that channel only. confusing right? haha!
    //
    // internally, it's like this:
    // 
    // ints0 = 1001 (channel 0, 3 are pending)
    // channel = 3 (clear channel 3) 1u << 3 = 1000
    //
    // ints0 ^ channel = 1001 ^ 1000 = 0001 (clears the bit flag for channel 3)
    // or:
    // ints0 = ints0 & ~channel = 1001 & ~1000 = 0001 (clears the bit flag for channel 3)
    dma_hw->ints0 = 1u << dma_channel;
    // set the read address to the wavetable
    dma_channel_set_read_addr(dma_channel, &wavetable[pwm_level], true);

    // increment the pwm level making sure it wraps around
    pwm_level = (pwm_level + 1) % PWM_LEVELS;
}

int main() {
    // initialize stdio
    stdio_init_all();

    // initialize Wi-Fi
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        return -1;
    }

    // slight delay...
    sleep_ms(2000);

    // add the pio program to the PIO instance and get the offset
    uint offset = pio_add_program(pio0, &dma_pio_program);
    // call the dma_pio_program_init function to initialize the PIO program
    dma_pio_program_init(pio0, 0, offset, LED_PIN, PIO_CLK_DIV);

    // claim an unused DMA channel
    dma_channel = dma_claim_unused_channel(true);
    // get the default dma channel config
    dma_channel_config dma_config = dma_channel_get_default_config(dma_channel);
    // set the transfer data size to 32 bits
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);
    // set the read increment to false, this will keep the read address constant
    channel_config_set_read_increment(&dma_config, false);
    // set the data request signal to PIO0 TX0
    channel_config_set_dreq(&dma_config, DREQ_PIO0_TX0);

    // configure the DMA channel
    dma_channel_configure(
        dma_channel, // channel
        &dma_config, // config
        &pio0_hw->txf[0], // write address, in this case PIO0 TX FIFO state machine 0
        NULL, // will be set later
        DMA_TRANSFER_SIZE, // transfer count
        false // do not start immediately (PIO will start it on first request)
    );

    // set the interrupt handler for the DMA channel
    dma_channel_set_irq0_enabled(dma_channel, true);

    // enable the DMA channel
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    // enable the DMA channel interrupt
    irq_set_enabled(DMA_IRQ_0, true);

    // call the handler manually to setup the
    // wavetable and the initial dma read address
    dma_handler();

    while (true) {
        tight_loop_contents();
    }

    return 0;
}