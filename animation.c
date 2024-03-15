// Include Vga graphics library
#include "vga_graphics.h"

// Include standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Include Pico libraries
#include "pico/stdlib.h"
#include "pico/divider.h"
#include "pico/multicore.h"
// Include hardware libraries
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
// Include protothreads
#include "pt_cornell_rp2040_v1.h"

// === the fixed point macros ========================================
typedef signed int fix15 ;
#define multfix15(a,b) ((fix15)((((signed long long)(a))*((signed long long)(b)))>>15))
#define float2fix15(a) ((fix15)((a)*32768.0)) // 2^15
#define fix2float15(a) ((float)(a)/32768.0)
#define absfix15(a) abs(a) 
#define int2fix15(a) ((fix15)(a << 15))
#define fix2int15(a) ((int)(a >> 15))
#define char2fix15(a) (fix15)(((fix15)(a)) << 15)
#define divfix(a,b) (fix15)(div_s64s64( (((signed long long)(a)) << 15), ((signed long long)(b))))

// Wall detection
#define hitBottom(b) (b>int2fix15(380))
#define hitTop(b) (b<int2fix15(100))
#define hitLeft(a) (a<int2fix15(100))
#define hitRight(a) (a>int2fix15(540))

// uS per frame
#define FRAME_RATE 33000

// the color of the boid
char color = WHITE ;

// Boid on core 0
fix15 boid0_x ;
fix15 boid0_y ;
fix15 boid0_vx ;
fix15 boid0_vy ;

// Boid on core 1
fix15 boid1_x ;
fix15 boid1_y ;
fix15 boid1_vx ;
fix15 boid1_vy ;

// Create a boid
void spawnBoid(fix15 *x, fix15 *y, fix15 *vx, fix15 *vy, int direction)
{
    // Start in center of screen
    *x = int2fix15(320);
    *y = int2fix15(240);
    // Choose left or right
    if (direction)
        *vx = int2fix15(3);
    else
        *vx = int2fix15(-3);
    // Moving down
    *vy = int2fix15(1);
}

// Draw the boundaries
void drawArena()
{
    drawVLine(100, 100, 280, WHITE);
    drawVLine(540, 100, 280, WHITE);
    drawHLine(100, 100, 440, WHITE);
    drawHLine(100, 380, 440, WHITE);
}

// Detect wallstrikes, update velocity and position
void wallsAndEdges(fix15 *x, fix15 *y, fix15 *vx, fix15 *vy)
{
    // Reverse direction if we've hit a wall
    if (hitTop(*y))
    {
        *vy = (-*vy);
        *y = (*y + int2fix15(5));
    }
    if (hitBottom(*y))
    {
        *vy = (-*vy);
        *y = (*y - int2fix15(5));
    }
    if (hitRight(*x))
    {
        *vx = (-*vx);
        *x = (*x - int2fix15(5));
    }
    if (hitLeft(*x))
    {
        *vx = (-*vx);
        *x = (*x + int2fix15(5));
    }

    // Update position using velocity
    *x = *x + *vx;
    *y = *y + *vy;
}

// Animation on core 0
static PT_THREAD (protothread_anim(struct pt *pt))
{
    // Mark Beginning of thread
    PT_BEGIN(pt);

    // Variables for maintaining frame rate
    static int begin_time;
    static int spare_time;

    // Spawn a boid
    spawnBoid(&boid0_x, &boid0_y, &boid0_vx, &boid0_vy, 0);
    
    while(1)
    {
        // Measure time at the start of the thread
        begin_time = time_us_32();
        // Erase boid
        drawRect(fix2int15(boid0_x), fix2int15(boid0_y), 2, 2, BLACK);
        // Update boid position and velocity
        wallsAndEdges(&boid0_x, &boid0_y, &boid0_vx, &boid0_vy);
        // draw boundaries
        drawArena();
        // Delay in accordance with frame rate
        spare_time = FRAME_RATE - (time_us_32() - begin_time);
        // Yield for necessary amount of time
        PT_YIELD_usec(spare_time);
    }
    PT_END(pt);
}

// Animation on core 1
static PT_THREAD (protothread_anim1(struct pt *pt))
{
    // Mark Beginning of thread
    PT_BEGIN(pt);

    // Variables for maintaining frame rate
    static int begin_time;
    static int spare_time;

    // Spawn a boid
    spawnBoid(&boid1_x, &boid1_y, &boid1_vx, &boid1_vy, 0);
    
    while(1)
    {
        // Measure time at the start of the thread
        begin_time = time_us_32();
        // Erase boid
        drawRect(fix2int15(boid1_x), fix2int15(boid1_y), 2, 2, BLACK);
        // Update boid position and velocity
        wallsAndEdges(&boid1_x, &boid1_y, &boid1_vx, &boid1_vy);
        // draw boundaries
        drawArena();
        // Delay in accordance with frame rate
        spare_time = FRAME_RATE - (time_us_32() - begin_time);
        // Yield for necessary amount of time
        PT_YIELD_usec(spare_time);
    }
    PT_END(pt);
}



void core1_main()
{
    // Add animation thread
    pt_add_thread(protothread_anim1);
    // Start schedule
    pt_schedule_start;
}

int main()
{
    // Initialize stdio
    stdio_init_all();

    // Initialize VGA
    init_VGA();

    // start core 1
    multicore_reset_core1();
    multicore_launch_core1(&core1_main);

    // add thread
    pt_add_thread(protothread_anim);

    // Start scheduler
    pt_schedule_start;
}