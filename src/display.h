#ifndef DISPLAY_H
#define DISPLAY_H

#include "doomnanoce.h"

// Display function prototypes
void setupDisplay(void);
void renderMap(const uint8_t level[], double view_height);
void drawPixel(int8_t x, int8_t y, bool color, bool raycasterViewport);
void drawVLine(uint8_t x, int8_t start_y, int8_t end_y, uint8_t intensity);
void drawSprite(int8_t x, int8_t y, const uint8_t bitmap[], const uint8_t mask[], int16_t w, int16_t h, uint8_t sprite, double distance);
void drawText(int8_t x, int8_t y, char *txt, uint8_t space);

#endif // DISPLAY_H