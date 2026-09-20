#ifndef SPRITES_H
#define SPRITES_H

#include <stdint.h>

// Sprite definitions for DoomNanoCE, ported from docs/doom-nano/sprites.h.
// All data is 1bpp: each row is packed 8 pixels per byte, most significant bit
// leftmost, rows top to bottom. Sheets with several frames store them one
// after another; drawSprite() selects one with its `sprite` argument.
// Sprites that can be occluded come with a separate mask of the same size: a
// set mask bit means "draw this pixel", so black pixels inside the silhouette
// stay black instead of being transparent.

// Font sheet: 4x6 characters, two per byte, indexed through CHAR_MAP
#define BMP_FONT_WIDTH   24  // in bytes
#define BMP_FONT_HEIGHT  6
#define CHAR_MAP         " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ.,-_(){}[]#"
#define CHAR_WIDTH       4
#define CHAR_HEIGHT      6
extern const uint8_t bmp_font[144];

// Title logo
#define BMP_LOGO_WIDTH   72
#define BMP_LOGO_HEIGHT  47
extern const uint8_t bmp_logo_bits[423];

// Player gun, drawn as a black mask followed by the sprite
#define BMP_GUN_WIDTH    32
#define BMP_GUN_HEIGHT   32
extern const uint8_t bmp_gun_bits[128];
extern const uint8_t bmp_gun_mask[128];

// Muzzle flash
#define BMP_FIRE_WIDTH   24
#define BMP_FIRE_HEIGHT  20
extern const uint8_t bmp_fire_bits[60];

// Enemy: stand, walk, fire, hit, dead
#define BMP_IMP_WIDTH    32
#define BMP_IMP_HEIGHT   32
#define BMP_IMP_COUNT    5
extern const uint8_t bmp_imp_bits[640];
extern const uint8_t bmp_imp_mask[640];

// Enemy projectile
#define BMP_FIREBALL_WIDTH  16
#define BMP_FIREBALL_HEIGHT 16
extern const uint8_t bmp_fireball_bits[32];
extern const uint8_t bmp_fireball_mask[32];

// Door (present in the original art, not rendered by the original game either)
#define BMP_DOOR_WIDTH   32
#define BMP_DOOR_HEIGHT  32
extern const uint8_t bmp_door_bits[128];

// Pickups: medikit, key
#define BMP_ITEMS_WIDTH  16
#define BMP_ITEMS_HEIGHT 16
#define BMP_ITEMS_COUNT  2
extern const uint8_t bmp_items_bits[64];
extern const uint8_t bmp_items_mask[64];

#endif // SPRITES_H
