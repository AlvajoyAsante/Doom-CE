#include "doomnanoce.h"

/**
 * Create a coordinate structure
 */
Coords create_coords(double x, double y) {
    Coords coords;
    coords.x = x;
    coords.y = y;
    return coords;
}

/**
 * Calculate distance between two coordinates
 */
uint8_t coords_distance(Coords* a, Coords* b) {
    double dx = a->x - b->x;
    double dy = a->y - b->y;
    double distance = sqrt(dx*dx + dy*dy) * DISTANCE_MULTIPLIER;

    // Clamp before the cast, otherwise the value wraps instead of saturating
    if (distance > 255.0) distance = 255.0;
    return (uint8_t)distance;
}

/**
 * Create a unique identifier for entities
 */
UID create_uid(EType type, uint8_t x, uint8_t y) {
    return (type << 12) | ((uint16_t)x << 6) | y;
}

/**
 * Get the type of an entity from its UID
 */
EType uid_get_type(UID uid) {
    return (uid >> 12) & 0xF;
}