#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

// Our assembled program
// Each gets the name <pio_filename.pio.h>
#include "hsync.pio.h"
#include "vsync.pio.h"
#include "rgb.pio.h"

// Header file
#include "vga_graphics.h"


// VGA timing constants
#define H_ACTIVE 655      // (active + frontporch - 1) - one cycle delay for mov
#define V_ACTIVE 479      // (active - 1)
#define RGB_ACTIVE 319    // (horizontal active)/2 - 1

// Length of the pixel array, and number of DMA transfers
#define TXCOUNT 153600     // Total pixels/2 (since we have 2 pixels per byte)

// Pixel color array - DMA to PIO
// a pointer to the address of this color array
// color array initialized as all 0s (all black)
unsigned char vga_data_array[TXCOUNT];
char *address_pointer = &vga_data_array[0];

// Bit mask for drawPixel routine
#define TOPMASK 0b11000111
#define BOTTOMMASK 0b11111000

// For drawLine
#define swap(a, b) { short t = a; a = b; b = t; }

// Screen width and height
#define _width 640
#define _height 480

void init_VGA()
{
    // Choose pio instance to use (there are two)
    PIO pio = pio0;

    // Loading our assembled program in PIO in a location(offset)
    // Only 32 instructions are allowed in total
    uint hsync_offset = pio_add_program(pio, &hsync_program);
    uint vsync_offset = pio_add_program(pio, &vsync_program);
    uint rgb_offset = pio_add_program(pio, &rgb_program); 

    // Manually Select a few state machines from pio instance pio0
    uint hsync_sm = 0;
    uint vsync_sm = 1;
    uint rgb_sm = 2;

    // Call the initialization functions that are defined in each pio file
    hsync_program_init(pio, hsync_sm, hsync_offset, HSYNC);
    vsync_program_init(pio, vsync_sm, vsync_offset, VSYNC);
    rgb_program_init(pio, rgb_sm, rgb_offset, RED_PIN);


    // ========= PIO DMA ===========
    // DMA channel 0 sends color data, channel 1 reconfigures and restarts 0
    int rgb_chan_0 = 0;
    int rgb_chan_1 = 1;

    // Channel Zero - Sends color data to PIO VGA machine
    dma_channel_config c0 = dma_channel_get_default_config(rgb_chan_0); // Default config
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_8);             // 8-bit transfer
    channel_config_set_read_increment(&c0, true);                       // yes read incrementing
    channel_config_set_write_increment(&c0, false);                     // no write incrementing
    channel_config_set_dreq(&c0, DREQ_PIO0_TX2);                        // DMA channel is paced by DREQ (only transfer when TX FIFO is emptied)
    channel_config_set_chain_to(&c0, rgb_chan_1);                       // chain to other channel

    dma_channel_configure(
        rgb_chan_0,                 // Channel to be configured
        &c0,                        // The configuration we just created
        &pio->txf[rgb_sm],          // write address (RGB PIO TX FIFO)
        &vga_data_array,            // The initial read address (pixel color array)
        TXCOUNT,                    // Number of transfers; in this case each is 1 byte
        false                       // Don't start immediately
    );

    // Channel One (reconfigures the first channel)
    dma_channel_config c1 = dma_channel_get_default_config(rgb_chan_1);   // default configs
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c1, false);                        // no read incrementing
    channel_config_set_write_increment(&c1, false);                       // no write incrementing
    channel_config_set_chain_to(&c1, rgb_chan_0);                         // chain to other channel

    dma_channel_configure(
        rgb_chan_1,                         // Channel to be configured
        &c1,                                // The configuration we just created
        &dma_hw->ch[rgb_chan_0].read_addr,  // Write address (channel 0 read address)
        &address_pointer,                   // Read address (POINTER TO AN ADDRESS)
        1,                                  // Number of transfers, in this case each is 4 byte
        false                               // Don't start immediately.
    );

    // Initialize PIO state machine counters. This passes the information to the state machines
    // that they retrieve in the first 'pull' instructions, before the .wrap_target directive
    // in the assembly. Each uses these values to initialize some counting registers.
    pio_sm_put_blocking(pio, hsync_sm, H_ACTIVE);
    pio_sm_put_blocking(pio, vsync_sm, V_ACTIVE);
    pio_sm_put_blocking(pio, rgb_sm, RGB_ACTIVE);

    // Start the state machines in sync
    pio_enable_sm_mask_in_sync(pio, ((1u << hsync_sm) | (1u << vsync_sm) | (1u << rgb_sm)));

    // Start DMA channel 0. Once started, the contents of the pixel color array
    // will be continously DMA's to the PIO machines that are driving the screen.
    // To change the contents of the screen, we need only change the contents
    // of that array.
    dma_start_channel_mask((1u << rgb_chan_0)) ;
}

void drawPixel(short x, short y, char color)
{
    // Range checks (640x480 display)
    if (x > 639) x = 639 ;
    if (x < 0) x = 0 ;
    if (y < 0) y = 0 ;
    if (y > 479) y = 479 ;

    // Which pixel is it?
    int pixel = ((640 * y) + x) ;

    // Is this pixel stored in the first 3 bits
    // of the vga data array index, or the second
    // 3 bits? Check, then mask.
    // pixel & 1: checking if even
    if (pixel & 1) {
        vga_data_array[pixel>>1] = (vga_data_array[pixel>>1] & TOPMASK) | (color << 3) ;
    }
    else {
        vga_data_array[pixel>>1] = (vga_data_array[pixel>>1] & BOTTOMMASK) | (color) ;
    }
}

void drawVLine(short x, short y, short h, char color) {
    for (short i=y; i<(y+h); i++) {
        drawPixel(x, i, color) ;
    }
}

void drawHLine(short x, short y, short w, char color) {
    for (short i=x; i<(x+w); i++) {
        drawPixel(i, y, color) ;
    }
}

// Draw a rectangle
void drawRect(short x, short y, short w, short h, char color) {
/* Draw a rectangle outline with top left vertex (x,y), width w
 * and height h at given color
 * Parameters:
 *      x:  x-coordinate of top-left vertex. The x-coordinate of
 *          the top-left of the screen is 0. It increases to the right.
 *      y:  y-coordinate of top-left vertex. The y-coordinate of
 *          the top-left of the screen is 0. It increases to the bottom.
 *      w:  width of the rectangle
 *      h:  height of the rectangle
 *      color:  16-bit color of the rectangle outline
 * Returns: Nothing
 */
  drawHLine(x, y, w, color);
  drawHLine(x, y+h-1, w, color);
  drawVLine(x, y, h, color);
  drawVLine(x+w-1, y, h, color);
}
