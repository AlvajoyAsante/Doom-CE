/*
 * display.h
 * Display system header for DoomNanoCE port
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Screen dimensions for TI-84+CE (320x240)
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define DISPLAY_BUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT / 8)

// Display buffer
extern uint8_t display_buffer[DISPLAY_BUFFER_SIZE];

// Function prototypes
void display_init(void);
void display_clear(void);
void display_update(void);
void display_put_pixel(int x, int y, uint8_t color);
uint8_t display_get_pixel(int x, int y);
void display_fill_rect(int x, int y, int width, int height, uint8_t color);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H