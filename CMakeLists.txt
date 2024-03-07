cmake_minimum_required(VERSION 3.21)

include(pico_sdk_import.cmake)

project(VGA)

pico_sdk_init()

add_executable(animation)

add_compile_options(-0fast)

pico_generate_pio_header(animation ${CMAKE_CURRENT_LIST_DIR}/hsync.pio)
pico_generate_pio_header(animation ${CMAKE_CURRENT_LIST_DIR}/vsync.pio)
pico_generate_pio_header(animation ${CMAKE_CURRENT_LIST_DIR}/rgb.pio)

target_sources(animation PRIVATE animation.c vga_graphics.c)

target_link_libraries(animation PRIVATE pico_stdlib pico_divider pico_multicore pico_bootsel_via_double_reset hardware_pio hardware_dma hardware_adc hardware_irq hardware_clocks hardware_pll)

pico_add_extra_outputs(animation)