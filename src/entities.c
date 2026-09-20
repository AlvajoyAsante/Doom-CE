#include "doomnanoce.h"

/**
 * Create a new entity with specified parameters
 */
Entity create_entity(uint8_t type, uint8_t x, uint8_t y, uint8_t initialState, uint8_t initialHealth) {
    Entity entity;
    entity.uid = create_uid(type, x, y);
    entity.pos.x = x + 0.5;
    entity.pos.y = y + 0.5;
    entity.state = initialState;
    entity.health = initialHealth;
    entity.distance = 0;
    entity.timer = 0;
    return entity;
}

/**
 * Create a static entity
 */
StaticEntity create_static_entity(UID uid, uint8_t x, uint8_t y, bool active) {
    StaticEntity entity;
    entity.uid = uid;
    entity.x = x;
    entity.y = y;
    entity.active = active;
    return entity;
}