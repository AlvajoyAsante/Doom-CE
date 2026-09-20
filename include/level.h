/*
 * level.h
 * Level system header for DoomNanoCE port
 */

#ifndef LEVEL_H
#define LEVEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Map dimensions
#define MAP_WIDTH 64
#define MAP_HEIGHT 64
#define MAP_SIZE (MAP_WIDTH * MAP_HEIGHT)

// Wall types
typedef enum {
    WALL_TYPE_NORMAL,
    WALL_TYPE_DOOR,
    WALL_TYPE_WINDOW,
    WALL_TYPE_SECRET
} wall_type_t;

// Level structure
typedef struct {
    uint8_t map_data[MAP_SIZE];
    uint8_t wall_types[MAP_SIZE];
    uint8_t floor_height[MAP_SIZE];
    uint8_t ceiling_height[MAP_SIZE];
    uint8_t textures[MAP_SIZE];
} level_t;

// Function prototypes
void level_init(void);
void level_update(void);
void level_render(void);
void level_load_map(const char* filename);
void level_save_map(const char* filename);
uint8_t level_get_tile(int x, int y);
void level_set_tile(int x, int y, uint8_t tile);

#ifdef __cplusplus
}
#endif

#endif // LEVEL_H