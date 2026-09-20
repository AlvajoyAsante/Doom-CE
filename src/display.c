#include "display.h"

/**
 * Initialize the display system
 */
void setupDisplay(void) {
    // Initialize graphics
    gfx_Begin();
    gfx_SetDrawBuffer();
    
    // Set up display parameters
    gfx_SetColor(1);
}

/**
 * Render the game map using raycasting algorithm
 */
void renderMap(const uint8_t level[], double view_height) {
    // This is a placeholder for the raycasting implementation
    // A full implementation would go here
    
    // Clear screen
    gfx_FillScreen(0);
    
    // Draw a simple grid pattern as a placeholder
    for (int x = 0; x < DISPLAY_WIDTH; x += 20) {
        for (int y = 0; y < DISPLAY_HEIGHT; y += 20) {
            if ((x + y) % 40 == 0) {
                gfx_SetColor(1);
            } else {
                gfx_SetColor(0);
            }
            gfx_Rectangle(x, y, 20, 20);
        }
    }
    
    // Draw player position
    gfx_SetColor(1);
    gfx_FillCircle(160, 120, 5);  // Center of screen
    
    // Swap buffers to display
    gfx_SwapDraw();
}

/**
 * Draw a single pixel on the screen
 */
void drawPixel(int8_t x, int8_t y, bool color, bool raycasterViewport) {
    if (x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT) {
        gfx_SetPixel(x, y);
    }
}

/**
 * Draw a vertical line
 */
void drawVLine(uint8_t x, int8_t start_y, int8_t end_y, uint8_t intensity) {
    if (x >= 0 && x < DISPLAY_WIDTH) {
        gfx_VLine(x, start_y, end_y - start_y);
    }
}

/**
 * Draw a sprite
 */
void drawSprite(int8_t x, int8_t y, const uint8_t bitmap[], const uint8_t mask[], int16_t w, int16_t h, uint8_t sprite, double distance) {
    // Placeholder for sprite rendering
    gfx_SetColor(1);
    gfx_Rectangle(x, y, w, h);
}

/**
 * Draw text to the screen
 */
void drawText(int8_t x, int8_t y, char *txt, uint8_t space) {
    gfx_SetTextXY(x, y);
    gfx_PrintString(txt);
}