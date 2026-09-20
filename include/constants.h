/*
 * constants.h
 * Constants for DoomNanoCE port
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#ifdef __cplusplus
extern "C" {
#endif

// Screen and display constants
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define SCREEN_BPP 1
#define SCREEN_BUFFER_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 8)

// Game constants
#define MAX_ENTITIES 100
#define MAX_SPRITES 50
#define MAX_TEXTURES 20

// Movement constants
#define MOVE_SPEED 4
#define ROTATION_SPEED 4
#define PLAYER_HEIGHT 32

// Timing constants
#define FRAMES_PER_SECOND 30
#define FRAME_TIME_MS (1000 / FRAMES_PER_SECOND)

// Color constants (for TI-84+CE display)
#define COLOR_BLACK 0
#define COLOR_WHITE 1

#ifdef __cplusplus
}
#endif

#endif // CONSTANTS_H