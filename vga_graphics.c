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
    
}