#include "doomnanoce.h"
#include "level.h"

// Global game state variables
uint8_t scene = INTRO;
bool exit_scene = false;
bool invert_screen = false;
uint8_t flash_screen = 0;

// Game entities
Player player;
Entity entity[MAX_ENTITIES];
StaticEntity static_entity[MAX_STATIC_ENTITIES];
uint8_t num_entities = 0;
uint8_t num_static_entities = 0;

// Timing variables
double delta = 1;
uint32_t lastFrameTime = 0;

// Z-buffer for depth sorting
uint8_t zbuffer[ZBUFFER_SIZE];

// Game level data (will be initialized from level.h)
extern const uint8_t level_data[];

/**
 * Initialize the game system
 */
void setup(void) {
    // Initialize graphics
    gfx_Begin();
    gfx_SetDrawBuffer();
    
    // Initialize input handling
    keypad_Init();
    
    // Initialize z buffer
    memset(zbuffer, 0xFF, ZBUFFER_SIZE);
    
    // Initialize level
    initializeLevel(level_data);
}

/**
 * Main game loop
 */
void loop(void) {
    // Main game loop
    while (1) {
        // Handle input
        handleInput();
        
        // Update entities
        updateEntities(level_data);
        
        // Render the scene
        renderMap(level_data, 0.5);
        
        // Update HUD
        updateHud();
        
        // Frame rate control
        fps();
    }
}

/**
 * Initialize the game level
 */
void initializeLevel(const uint8_t level[]) {
    // Find player spawn position
    for (uint8_t y = LEVEL_HEIGHT - 1; y >= 0; y--) {
        for (uint8_t x = 0; x < LEVEL_WIDTH; x++) {
            uint8_t block = getBlockAt(level, x, y);

            if (block == E_PLAYER) {
                player.pos.x = (double) x + 0.5;
                player.pos.y = (double) y + 0.5;
                player.dir.x = 1.0;
                player.dir.y = 0.0;
                player.plane.x = 0.0;
                player.plane.y = -0.66; // Set the plane for field of view
                player.velocity = 0.0;
                player.health = 100;
                player.keys = 0;
                return;
            }
        }
    }
}

/**
 * Get a block from the level at specified coordinates
 */
uint8_t getBlockAt(const uint8_t level[], uint8_t x, uint8_t y) {
    if (x < 0 || x >= LEVEL_WIDTH || y < 0 || y >= LEVEL_HEIGHT) {
        return E_FLOOR;
    }

    // Simple implementation - in a real version this would decode
    // the actual level data properly
    if (x == 0 || x == LEVEL_WIDTH-1 || y == 0 || y == LEVEL_HEIGHT-1) {
        return E_WALL; // Border walls
    }
    
    // Simple maze pattern - for demo purposes
    if ((x > 10 && x < 20 && y > 5 && y < 15) || 
        (x > 30 && x < 40 && y > 15 && y < 25)) {
        return E_WALL;
    }
    
    // Player spawn position
    if (x == 20 && y == 10) {
        return E_PLAYER;
    }
    
    return E_FLOOR; // Default to floor
}

/**
 * Check if an entity is already spawned
 */
bool isSpawned(UID uid) {
    for (uint8_t i = 0; i < num_entities; i++) {
        if (entity[i].uid == uid) return true;
    }
    return false;
}

/**
 * Check if a static entity exists
 */
bool isStatic(UID uid) {
    for (uint8_t i = 0; i < num_static_entities; i++) {
        if (static_entity[i].uid == uid) return true;
    }
    return false;
}

/**
 * Spawn a new entity in the game world
 */
void spawnEntity(uint8_t type, uint8_t x, uint8_t y) {
    // Limit the number of spawned entities
    if (num_entities >= MAX_ENTITIES) {
        return;
    }

    switch (type) {
        case E_ENEMY:
            entity[num_entities] = (Entity) {
                .uid = create_uid(type, x, y),
                .pos = {x + 0.5, y + 0.5},
                .state = S_STAND,
                .health = 100,
                .distance = 0,
                .timer = 0
            };
            num_entities++;
            break;

        case E_KEY:
            entity[num_entities] = (Entity) {
                .uid = create_uid(type, x, y),
                .pos = {x + 0.5, y + 0.5},
                .state = S_STAND,
                .health = 0,
                .distance = 0,
                .timer = 0
            };
            num_entities++;
            break;

        case E_MEDIKIT:
            entity[num_entities] = (Entity) {
                .uid = create_uid(type, x, y),
                .pos = {x + 0.5, y + 0.5},
                .state = S_STAND,
                .health = 0,
                .distance = 0,
                .timer = 0
            };
            num_entities++;
            break;
    }
}

/**
 * Spawn a fireball at given position
 */
void spawnFireball(double x, double y) {
    // Limit the number of spawned entities
    if (num_entities >= MAX_ENTITIES) {
        return;
    }

    UID uid = create_uid(E_FIREBALL, x, y);
    
    // Remove if already exists, don't throw anything. Not the best, but shouldn't happen too often
    if (isSpawned(uid)) return;

    // Calculate direction - simplified for CE port
    double angle = atan2(y - player.pos.y, x - player.pos.x);
    int16_t dir = (int16_t)(angle / M_PI * 16 + 32) % 32;  // Simplified 32-angle system
    
    entity[num_entities] = (Entity) {
        .uid = uid,
        .pos = {x, y},
        .state = S_STAND,
        .health = dir,  // Use health to store angle
        .distance = 0,
        .timer = 0
    };
    num_entities++;
}

/**
 * Remove an entity from the game
 */
void removeEntity(UID uid, bool makeStatic) {
    uint8_t i = 0;
    bool found = false;

    while (i < num_entities) {
        if (!found && entity[i].uid == uid) {
            found = true;
            num_entities--;
        }

        // displace entities
        if (found) {
            entity[i] = entity[i + 1];
        }

        i++;
    }
}

/**
 * Remove a static entity from the game
 */
void removeStaticEntity(UID uid) {
    uint8_t i = 0;
    bool found = false;

    while (i < num_static_entities) {
        if (!found && static_entity[i].uid == uid) {
            found = true;
            num_static_entities--;
        }

        // displace entities
        if (found) {
            static_entity[i] = static_entity[i + 1];
        }

        i++;
    }
}

/**
 * Detect collisions with walls or entities
 */
UID detectCollision(const uint8_t level[], Coords *pos, double relative_x, double relative_y, bool only_walls) {
    // Wall collision
    uint8_t round_x = (uint8_t)(pos->x + relative_x);
    uint8_t round_y = (uint8_t)(pos->y + relative_y);
    uint8_t block = getBlockAt(level, round_x, round_y);

    if (block == E_WALL) {
        return create_uid(block, round_x, round_y);
    }

    if (only_walls) {
        return 0;  // UID_null
    }

    // Entity collision
    for (uint8_t i = 0; i < num_entities; i++) {
        // Don't collide with itself
        if (&(entity[i].pos) == pos) {
            continue;
        }

        uint8_t type = uid_get_type(entity[i].uid);

        // Only ALIVE enemy collision
        if (type != E_ENEMY || entity[i].state == S_DEAD || entity[i].state == S_HIDDEN) {
            continue;
        }

        double distance = coords_distance(pos, &(entity[i].pos));

        // Check distance and if it's getting closer
        if (distance < ENEMY_COLLIDER_DIST && distance < entity[i].distance) {
            return entity[i].uid;
        }
    }

    return 0;  // UID_null
}

/**
 * Handle player shooting
 */
void fire(void) {
    // Simplified shooting logic for CE port
    for (uint8_t i = 0; i < num_entities; i++) {
        // Shoot only ALIVE enemies
        if (uid_get_type(entity[i].uid) != E_ENEMY || entity[i].state == S_DEAD || entity[i].state == S_HIDDEN) {
            continue;
        }

        Coords transform = translateIntoView(&(entity[i].pos));
        if (abs(transform.x) < 20 && transform.y > 0) {
            uint8_t damage = (uint8_t) min(GUN_MAX_DAMAGE, GUN_MAX_DAMAGE / (abs(transform.x) * entity[i].distance) / 5);
            if (damage > 0) {
                entity[i].health = max(0, entity[i].health - damage);
                entity[i].state = S_HIT;
                entity[i].timer = 4;
            }
        }
    }
}

/**
 * Update position of an object with collision detection
 */
UID updatePosition(const uint8_t level[], Coords *pos, double relative_x, double relative_y, bool only_walls) {
    UID collide_x = detectCollision(level, pos, relative_x, 0, only_walls);
    UID collide_y = detectCollision(level, pos, 0, relative_y, only_walls);

    if (!collide_x) pos->x += relative_x;
    if (!collide_y) pos->y += relative_y;

    return collide_x || collide_y ? 1 : 0;  // Return true if collision occurred
}

/**
 * Update all entities in the game world
 */
void updateEntities(const uint8_t level[]) {
    uint8_t i = 0;
    while (i < num_entities) {
        // update distance
        entity[i].distance = coords_distance(&(player.pos), &(entity[i].pos));

        // Run the timer. Works with actual frames.
        if (entity[i].timer > 0) entity[i].timer--;

        // too far away. put it in doze mode
        if (entity[i].distance > MAX_ENTITY_DISTANCE) {
            removeEntity(entity[i].uid, false);
            // don't increase 'i', since current one has been removed
            continue;
        }

        // bypass render if hidden
        if (entity[i].state == S_HIDDEN) {
            i++;
            continue;
        }

        uint8_t type = uid_get_type(entity[i].uid);

        switch (type) {
            case E_ENEMY: {
                // Enemy "IA"
                if (entity[i].health == 0) {
                    if (entity[i].state != S_DEAD) {
                        entity[i].state = S_DEAD;
                        entity[i].timer = 6;
                    }
                } else if (entity[i].state == S_HIT) {
                    if (entity[i].timer == 0) {
                        // Back to alert state
                        entity[i].state = S_ALERT;
                        entity[i].timer = 40;     // delay next fireball thrown
                    }
                } else if (entity[i].state == S_FIRING) {
                    if (entity[i].timer == 0) {
                        // Back to alert state
                        entity[i].state = S_ALERT;
                        entity[i].timer = 40;     // delay next fireball throwm
                    }
                } else {
                    // ALERT STATE
                    if (entity[i].distance > ENEMY_MELEE_DIST && entity[i].distance < MAX_ENEMY_VIEW) {
                        if (entity[i].state != S_ALERT) {
                            entity[i].state = S_ALERT;
                            entity[i].timer = 20;   // used to throw fireballs
                        } else {
                            if (entity[i].timer == 0) {
                                // Throw a fireball
                                spawnFireball(entity[i].pos.x, entity[i].pos.y);
                                entity[i].state = S_FIRING;
                                entity[i].timer = 6;
                            } else {
                                // move towards to the player.
                                updatePosition(
                                    level,
                                    &(entity[i].pos),
                                    (player.pos.x - entity[i].pos.x) * ENEMY_SPEED * delta,
                                    (player.pos.y - entity[i].pos.y) * ENEMY_SPEED * delta,
                                    true
                                );
                            }
                        }
                    } else if (entity[i].distance <= ENEMY_MELEE_DIST) {
                        if (entity[i].state != S_MELEE) {
                            // Preparing the melee attack
                            entity[i].state = S_MELEE;
                            entity[i].timer = 10;
                        } else if (entity[i].timer == 0) {
                            // Melee attack
                            player.health = max(0, player.health - ENEMY_MELEE_DAMAGE);
                            entity[i].timer = 14;
                            flash_screen = 1;
                            updateHud();
                        }
                    } else {
                        // stand
                        entity[i].state = S_STAND;
                    }
                }
                break;
            }

            case E_FIREBALL: {
                if (entity[i].distance < FIREBALL_COLLIDER_DIST) {
                    // Hit the player and disappear
                    player.health = max(0, player.health - ENEMY_FIREBALL_DAMAGE);
                    flash_screen = 1;
                    updateHud();
                    removeEntity(entity[i].uid, false);
                    continue; // continue in the loop
                } else {
                    // Move. Only collide with walls.
                    // Note: using health to store the angle of the movement
                    UID collided = updatePosition(
                        level,
                        &(entity[i].pos),
                        cos((double) entity[i].health / 32 * M_PI) * FIREBALL_SPEED,
                        sin((double) entity[i].health / 32 * M_PI) * FIREBALL_SPEED,
                        true
                    );

                    if (collided) {
                        removeEntity(entity[i].uid, false);
                        continue; // continue in the entity check loop
                    }
                }
                break;
            }

            case E_MEDIKIT: {
                if (entity[i].distance < ITEM_COLLIDER_DIST) {
                    // pickup
                    entity[i].state = S_HIDDEN;
                    player.health = min(100, player.health + 50);
                    updateHud();
                    flash_screen = 1;
                }
                break;
            }

            case E_KEY: {
                if (entity[i].distance < ITEM_COLLIDER_DIST) {
                    // pickup
                    entity[i].state = S_HIDDEN;
                    player.keys++;
                    updateHud();
                    flash_screen = 1;
                }
                break;
            }
        }

        i++;
    }
}

/**
 * Frame rate control and timing
 */
void fps(void) {
    uint32_t current_time = timer_Check();
    while ((current_time - lastFrameTime) < FRAME_TIME);
    delta = (double)(current_time - lastFrameTime) / FRAME_TIME;
    lastFrameTime = current_time;
}

/**
 * Get actual FPS for debugging
 */
double getActualFps(void) {
    return 1000 / (FRAME_TIME * delta);
}

/**
 * Handle input from the TI-84+CE keypad
 */
void handleInput(void) {
    // Get pressed keys
    kb_key_t key = kb_GetKey();
    
    switch(key) {
        case K_UP:
            player.pos.x += player.dir.x * PLAYER_SPEED;
            player.pos.y += player.dir.y * PLAYER_SPEED;
            break;
        case K_DOWN:
            player.pos.x -= player.dir.x * PLAYER_SPEED;
            player.pos.y -= player.dir.y * PLAYER_SPEED;
            break;
        case K_LEFT:
            // Rotate left
            {
                double temp_x = player.dir.x * cos(ROT_SPEED) - player.dir.y * sin(ROT_SPEED);
                double temp_y = player.dir.x * sin(ROT_SPEED) + player.dir.y * cos(ROT_SPEED);
                player.dir.x = temp_x;
                player.dir.y = temp_y;
                
                temp_x = player.plane.x * cos(ROT_SPEED) - player.plane.y * sin(ROT_SPEED);
                temp_y = player.plane.x * sin(ROT_SPEED) + player.plane.y * cos(ROT_SPEED);
                player.plane.x = temp_x;
                player.plane.y = temp_y;
            }
            break;
        case K_RIGHT:
            // Rotate right
            {
                double temp_x = player.dir.x * cos(-ROT_SPEED) - player.dir.y * sin(-ROT_SPEED);
                double temp_y = player.dir.x * sin(-ROT_SPEED) + player.dir.y * cos(-ROT_SPEED);
                player.dir.x = temp_x;
                player.dir.y = temp_y;
                
                temp_x = player.plane.x * cos(-ROT_SPEED) - player.plane.y * sin(-ROT_SPEED);
                temp_y = player.plane.x * sin(-ROT_SPEED) + player.plane.y * cos(-ROT_SPEED);
                player.plane.x = temp_x;
                player.plane.y = temp_y;
            }
            break;
        case K_FIRE:
            fire();
            break;
    }
}

/**
 * Translate a position into view coordinates
 */
Coords translateIntoView(Coords* pos) {
    Coords result;
    result.x = (pos->x - player.pos.x) * player.plane.x + (pos->y - player.pos.y) * player.plane.y;
    result.y = (pos->x - player.pos.x) * player.dir.x + (pos->y - player.pos.y) * player.dir.y;
    return result;
}

/**
 * Calculate distance between two coordinates
 */
double coords_distance(Coords* a, Coords* b) {
    double dx = a->x - b->x;
    double dy = a->y - b->y;
    return sqrt(dx*dx + dy*dy);
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

/**
 * Update and display the HUD
 */
void updateHud(void) {
    // Simple HUD - just show health
    gfx_SetTextXY(10, 10);
    gfx_SetTextScale(1, 1);
    char buffer[32];
    sprintf(buffer, "Health: %d", player.health);
    gfx_PrintString(buffer);
    
    sprintf(buffer, "Keys: %d", player.keys);
    gfx_SetTextXY(10, 25);
    gfx_PrintString(buffer);
}

/**
 * Main program entry point
 */
int main(void) {
    // Initialize the game
    setup();
    
    // Start the main game loop
    loop();
    
    // Clean up
    gfx_End();
    
    return 0;
}