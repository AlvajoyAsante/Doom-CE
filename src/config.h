/*
 * config.h
 * Configuration file for DoomNanoCE port
 */

#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// Build configuration
#define DOOM_NANO_CE_VERSION "0.1.0"
#define DOOM_NANO_CE_AUTHOR "DoomNanoCE Team"

// Hardware configuration
#define HAS_GRAPHICS 1
#define HAS_KEYPAD 1
#define HAS_SOUND 0  // Sound support not implemented yet
#define HAS_FLASH 1

// Display configuration
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define DISPLAY_BPP 8                 // graphx runs the LCD in 8bpp palletized mode
#define DISPLAY_BUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT * DISPLAY_BPP / 8)

// Game configuration
// NOTE: MAX_ENTITIES / MAX_STATIC_ENTITIES live in doomnanoce.h, they are part of
// the game logic rather than the build configuration.
#define MAX_SPRITES 50
#define MAX_TEXTURES 20

// Performance configuration
#define TARGET_FPS 30
#define FRAME_TIME_MS (1000 / TARGET_FPS)

#ifdef __cplusplus
}
#endif

#endif // CONFIG_H