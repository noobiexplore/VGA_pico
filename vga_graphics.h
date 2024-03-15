/*  Hardware connection
 *  - GPIO 16 ---> VGA Hsync
 *  - GPIO 17 ---> VGA Vsync
 *  - GPIO 18 ---> 330 ohm resistor ---> VGA Red
 *  - GPIO 19 ---> 330 ohm resistor ---> VGA Green
 *  - GPIO 20 ---> 330 ohm resistor ---> VGA Blue
 *  - RP2040 GND ---> VGA GND
*/

// Give the I/O pins that we're using some names that make sense - usable in main()
enum vga_pins {HSYNC=16, VSYNC, RED_PIN, GREEN_PIN, BLUE_PIN};

// We can only produce 8 (3-bit) colors, so let's give them usable names - usable in main()
enum colors {BLACK, RED, GREEN, BLUE, YELLOW, MAGENTA, CYAN, WHITE};

// VGA primitives - usable in main()
void init_VGA(void);
void drawPixel(short x, short y, char color);
void drawVLine(short x, short y, short h, char color);
void drawHLine(short x, short y, short w, char color);
void drawRect(short x, short y, short w, short h, char color);