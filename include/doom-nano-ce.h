/*
 * doom-nano-ce.h
 * Main header file for DoomNanoCE port
 */

#ifndef DOOM_NANO_CE_H
#define DOOM_NANO_CE_H

#ifdef __cplusplus
extern "C" {
#endif

// Include TI-84+CE libraries
#include <graphx.h>
#include <keypadc.h>
#include <ti/vars.h>

// Project-specific includes
#include "doom-nano/constants.h"
#include "doom-nano/display.h"
#include "doom-nano/input.h"
#include "doom-nano/entities.h"
#include "doom-nano/level.h"

// Configuration constants
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define SCREEN_BUFFER_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 8)

// Game states
typedef enum {
    GAME_STATE_MENU,
    GAME_STATE_PLAYING,
    GAME_STATE_PAUSED,
    GAME_STATE_GAME_OVER
} game_state_t;

// Game structure
typedef struct {
    game_state_t state;
    uint32_t frame_count;
    uint8_t running;
} doom_game_t;

// Function prototypes
void doom_init(void);
void doom_update(void);
void doom_render(void);
void doom_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // DOOM_NANO_CE_H