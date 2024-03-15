// -------------------------------------------------- //
// This file is autogenerated by pioasm; do not edit! //
// -------------------------------------------------- //

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

// ----- //
// vsync //
// ----- //

#define vsync_wrap_target 1
#define vsync_wrap 13

static const uint16_t vsync_program_instructions[] = {
    0x80a0, //  0: pull   block                      
            //     .wrap_target
    0xa027, //  1: mov    x, osr                     
    0x20c0, //  2: wait   1 irq, 0                   
    0xc001, //  3: irq    nowait 1                   
    0x0042, //  4: jmp    x--, 2                     
    0xe049, //  5: set    y, 9                       
    0x20c0, //  6: wait   1 irq, 0                   
    0x0086, //  7: jmp    y--, 6                     
    0xe000, //  8: set    pins, 0                    
    0x20c0, //  9: wait   1 irq, 0                   
    0x20c0, // 10: wait   1 irq, 0                   
    0xe05f, // 11: set    y, 31                      
    0x38c0, // 12: wait   1 irq, 0        side 1     
    0x008c, // 13: jmp    y--, 12                    
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program vsync_program = {
    .instructions = vsync_program_instructions,
    .length = 14,
    .origin = -1,
};

static inline pio_sm_config vsync_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + vsync_wrap_target, offset + vsync_wrap);
    sm_config_set_sideset(&c, 2, true, false);
    return c;
}

    static inline void vsync_program_init(PIO pio, uint sm, uint offset, uint pin){
        // creates a state machine configuration object c
        pio_sm_config c = vsync_program_get_default_config(offset);
        // Map the state machine's SET pin group to one pin, namely the `pin`
        // parameter to this function.
        sm_config_set_set_pins(&c, pin, 1);
        sm_config_set_sideset_pins(&c, pin);
        // Set clock division (div by 5 for 25 MHz state machine)
        sm_config_set_clkdiv(&c, 5) ;
        // Set this pin's GPIO function (connect PIO to the pad)
        pio_gpio_init(pio, pin);
        // pio_gpio_init(pio, pin+1);
        // Set the pin direction to output at the PIO
        pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
        // Load our configuration, and jump to the start of the program
        pio_sm_init(pio, sm, offset, &c);
        // Set the state machine running (commented out so can be synchronized with hsync)
        // pio_sm_set_enabled(pio, sm, true);
    }

#endif

