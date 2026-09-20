/*
 * entities.h
 * Entity system header for DoomNanoCE port
 */

#ifndef ENTITIES_H
#define ENTITIES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Entity types
typedef enum {
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_ENEMY,
    ENTITY_TYPE_ITEM,
    ENTITY_TYPE_PROJECTILE,
    ENTITY_TYPE_WALL,
    ENTITY_TYPE_DOOR
} entity_type_t;

// Basic entity structure
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
    uint8_t type;
    uint8_t active;
    uint8_t health;
    uint8_t sprite_index;
} entity_t;

// Entity manager structure
typedef struct {
    entity_t entities[100];  // Maximum 100 entities
    uint16_t entity_count;
} entity_manager_t;

// Function prototypes
void entities_init(void);
void entities_update(void);
void entities_render(void);
void entities_add_entity(entity_t* entity);
void entities_remove_entity(uint16_t index);
entity_t* entities_get_entity(uint16_t index);

#ifdef __cplusplus
}
#endif

#endif // ENTITIES_H