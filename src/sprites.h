#ifndef SPRITES_H
#define SPRITES_H

#include <stdint.h>

// Sprite definitions for DoomNanoCE port
// Adapted from original Doom Nano project for TI-84+CE graphics capabilities

// Font bitmap dimensions
#define BMP_FONT_WIDTH   24  // in bytes
#define BMP_FONT_HEIGHT  6
#define CHAR_MAP         " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ.,-_(){}[]#"
#define CHAR_WIDTH       4
#define CHAR_HEIGHT      6

// Font bitmap data (simplified for TI-84+CE)
extern const uint8_t bmp_font[];

// Logo sprite dimensions
#define BMP_LOGO_WIDTH  72
#define BMP_LOGO_HEIGHT 47

// Logo sprite data (simplified for calculator display)
extern const uint8_t bmp_logo_bits[];

// Gun sprite dimensions  
#define BMP_GUN_WIDTH   32
#define BMP_GUN_HEIGHT  32

// Gun sprite data (simplified for calculator display)
extern const uint8_t bmp_gun_bits[];

// Enemy sprite data (basic placeholder)
#define BMP_ENEMY_WIDTH   16
#define BMP_ENEMY_HEIGHT  16
extern const uint8_t bmp_enemy_bits[];

// Item sprite data (basic placeholder)
#define BMP_ITEM_WIDTH   16
#define BMP_ITEM_HEIGHT  16
extern const uint8_t bmp_item_bits[];

#endif // SPRITES_H