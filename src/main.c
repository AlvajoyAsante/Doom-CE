#include "doomnanoce.h"
#include "level.h"
#include "sprites.h"

// Global game state variables
uint8_t scene = INTRO;
bool exit_scene = false;
bool quit_game = false;
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
clock_t lastFrameTime = 0;

// Z-buffer for depth sorting
uint8_t zbuffer[ZBUFFER_SIZE];

/**
 * Initialize the game system
 */
void setup(void) {
    setupDisplay();
    input_setup();

    // Initialize z buffer
    memset(zbuffer, 0xFF, ZBUFFER_SIZE);

    lastFrameTime = clock();
}

/**
 * Jump to another scene
 */
void jumpTo(uint8_t target_scene) {
    scene = target_scene;
    exit_scene = true;
}

/**
 * Milliseconds since the program started. Stands in for Arduino's millis().
 */
uint32_t millis(void) {
    // Divide first: clock() * 1000 overflows 32 bits after about two minutes
    return (uint32_t)(clock() / (CLOCKS_PER_SEC / 1000));
}

/**
 * Run the active scene, then fade out and hand back to main().
 */
void loop(void) {
    switch (scene) {
        case INTRO:
            loopIntro();
            break;
        case GAME_PLAY:
            loopGamePlay();
            break;
    }

    // fade out effect
    for (uint8_t i = 0; i < FADE_STEPS; i++) {
        setFade(255 - 255 * i / (FADE_STEPS - 1));
        delay(40);
    }

    exit_scene = false;
}

/**
 * Initialize the game level
 */
void initializeLevel(const uint8_t level[]) {
    // Find player spawn position
    for (int y = LEVEL_HEIGHT - 1; y >= 0; y--) {
        for (int x = 0; x < LEVEL_WIDTH; x++) {
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

    UID uid = create_uid(E_FIREBALL, (uint8_t)x, (uint8_t)y);

    // Remove if already exists, don't throw anything. Not the best, but shouldn't happen too often
    if (isSpawned(uid)) return;

    // Calculate direction. FIREBALL_ANGLES angles per PI
    int16_t dir = FIREBALL_ANGLES + (int16_t)(atan2(y - player.pos.y, x - player.pos.x) / M_PI * FIREBALL_ANGLES);
    if (dir < 0) dir += FIREBALL_ANGLES * 2;

    entity[num_entities] = (Entity) {
        .uid = uid,
        .pos = {x, y},
        .state = S_STAND,
        .health = (uint8_t)dir,  // Use health to store angle
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

            // Park the entity in the sleeping list so it can be respawned
            // when the player comes back into range.
            if (makeStatic && num_static_entities < MAX_STATIC_ENTITIES && !isStatic(uid)) {
                static_entity[num_static_entities] = create_static_entity(
                    uid, (uint8_t)entity[i].pos.x, (uint8_t)entity[i].pos.y, true);
                num_static_entities++;
            }

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
        if (fabs(transform.x) < 20 && transform.y > 0) {
            uint8_t damage = (uint8_t) min(GUN_MAX_DAMAGE, GUN_MAX_DAMAGE / (fabs(transform.x) * entity[i].distance) / 5);
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
            removeEntity(entity[i].uid, true);
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
                        cos((double) entity[i].health / FIREBALL_ANGLES * M_PI) * FIREBALL_SPEED,
                        sin((double) entity[i].health / FIREBALL_ANGLES * M_PI) * FIREBALL_SPEED,
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
    // clock() ticks at CLOCKS_PER_SEC (32768 Hz on the CE)
    const clock_t frame_ticks = (clock_t)(FRAME_TIME * CLOCKS_PER_SEC / 1000);

    clock_t current_time = clock();
    while ((current_time - lastFrameTime) < frame_ticks) {
        current_time = clock();
    }

    delta = (double)(current_time - lastFrameTime) / frame_ticks;
    lastFrameTime = current_time;
}

/**
 * Get actual FPS for debugging
 */
double getActualFps(void) {
    return 1000 / (FRAME_TIME * delta);
}

/**
 * Translate a world position into camera space.
 * transform.y is depth into the screen, transform.x the lateral offset.
 */
Coords translateIntoView(Coords* pos) {
    // translate sprite position to relative to camera
    double sprite_x = pos->x - player.pos.x;
    double sprite_y = pos->y - player.pos.y;

    // required for correct matrix multiplication
    double inv_det = 1.0 / (player.plane.x * player.dir.y - player.dir.x * player.plane.y);
    double transform_x = inv_det * (player.dir.y * sprite_x - player.dir.x * sprite_y);
    double transform_y = inv_det * (-player.plane.y * sprite_x + player.plane.x * sprite_y);

    Coords result = { transform_x, transform_y };
    return result;
}

/**
 * Sort entities far to close so nearer sprites overdraw further ones.
 * Comb sort, as in the original.
 */
void sortEntities(void) {
    uint8_t gap = num_entities;
    bool swapped = false;

    while (gap > 1 || swapped) {
        // shrink factor 1.3
        gap = (gap * 10) / 13;
        if (gap == 9 || gap == 10) gap = 11;
        if (gap < 1) gap = 1;
        swapped = false;

        for (uint8_t i = 0; i + gap < num_entities; i++) {
            uint8_t j = i + gap;
            if (entity[i].distance < entity[j].distance) {
                Entity tmp = entity[i];
                entity[i] = entity[j];
                entity[j] = tmp;
                swapped = true;
            }
        }
    }
}

/**
 * Draw every visible entity as a billboard sprite.
 * Ported from renderEntities() in docs/doom-nano/doom-nano.ino.
 */
void renderEntities(double view_height) {
    sortEntities();

    for (uint8_t i = 0; i < num_entities; i++) {
        if (entity[i].state == S_HIDDEN) continue;

        Coords transform = translateIntoView(&(entity[i].pos));

        // don't render if behind the player or too far away
        if (transform.y <= 0.1 || transform.y > MAX_SPRITE_DEPTH) {
            continue;
        }

        int sprite_screen_x = (int)(HALF_WIDTH * (1.0 + transform.x / transform.y));
        int sprite_screen_y = (int)(RENDER_HEIGHT / 2 + view_height * VIEW_SCALE_Y / transform.y);
        uint8_t type = uid_get_type(entity[i].uid);

        // don't try to render if outside of screen
        if (sprite_screen_x < -HALF_WIDTH || sprite_screen_x > DISPLAY_WIDTH + HALF_WIDTH) {
            continue;
        }

        switch (type) {
            case E_ENEMY: {
                uint8_t sprite;
                if (entity[i].state == S_ALERT) {
                    sprite = (millis() / 500) % 2;          // walking
                } else if (entity[i].state == S_FIRING) {
                    sprite = 2;                             // fireball
                } else if (entity[i].state == S_HIT) {
                    sprite = 3;                             // hit
                } else if (entity[i].state == S_MELEE) {
                    sprite = entity[i].timer > 10 ? 2 : 1;  // melee attack
                } else if (entity[i].state == S_DEAD) {
                    sprite = entity[i].timer > 0 ? 3 : 4;   // dying
                } else {
                    sprite = 0;                             // stand
                }

                drawSprite(
                    sprite_screen_x - (int)(BMP_IMP_WIDTH * .5 * VIEW_SCALE_X / transform.y),
                    sprite_screen_y - (int)(8 * VIEW_SCALE_Y / transform.y),
                    bmp_imp_bits, bmp_imp_mask,
                    BMP_IMP_WIDTH, BMP_IMP_HEIGHT,
                    sprite, transform.y
                );
                break;
            }

            case E_FIREBALL:
                drawSprite(
                    sprite_screen_x - (int)(BMP_FIREBALL_WIDTH / 2 * VIEW_SCALE_X / transform.y),
                    sprite_screen_y - (int)(BMP_FIREBALL_HEIGHT / 2 * VIEW_SCALE_Y / transform.y),
                    bmp_fireball_bits, bmp_fireball_mask,
                    BMP_FIREBALL_WIDTH, BMP_FIREBALL_HEIGHT,
                    0, transform.y
                );
                break;

            case E_MEDIKIT:
                drawSprite(
                    sprite_screen_x - (int)(BMP_ITEMS_WIDTH / 2 * VIEW_SCALE_X / transform.y),
                    sprite_screen_y + (int)(5 * VIEW_SCALE_Y / transform.y),
                    bmp_items_bits, bmp_items_mask,
                    BMP_ITEMS_WIDTH, BMP_ITEMS_HEIGHT,
                    0, transform.y
                );
                break;

            case E_KEY:
                drawSprite(
                    sprite_screen_x - (int)(BMP_ITEMS_WIDTH / 2 * VIEW_SCALE_X / transform.y),
                    sprite_screen_y + (int)(5 * VIEW_SCALE_Y / transform.y),
                    bmp_items_bits, bmp_items_mask,
                    BMP_ITEMS_WIDTH, BMP_ITEMS_HEIGHT,
                    1, transform.y
                );
                break;
        }
    }
}

/**
 * Draw the player's gun, bobbing while walking.
 */
void renderGun(uint8_t gun_pos, double amount_jogging) {
    int gun_w = BMP_GUN_WIDTH * GUN_SCALE;

    // jogging
    int x = (DISPLAY_WIDTH - gun_w) / 2
            + (int)(sin((double) millis() * JOGGING_SPEED) * 10 * amount_jogging * VIEW_SCALE_X);
    int y = RENDER_HEIGHT - (int)(gun_pos * VIEW_SCALE_Y)
            + (int)(fabs(cos((double) millis() * JOGGING_SPEED)) * 8 * amount_jogging * VIEW_SCALE_Y);

    if (gun_pos > GUN_SHOT_POS - 2) {
        // Gun fire
        drawBitmap(x + 6 * GUN_SCALE, y - 11 * GUN_SCALE,
                   bmp_fire_bits, BMP_FIRE_WIDTH, BMP_FIRE_HEIGHT, GUN_SCALE, true);
    }

    // Draw the gun (black mask first, then the actual sprite)
    drawBitmap(x, y, bmp_gun_mask, BMP_GUN_WIDTH, BMP_GUN_HEIGHT, GUN_SCALE, false);
    drawBitmap(x, y, bmp_gun_bits, BMP_GUN_WIDTH, BMP_GUN_HEIGHT, GUN_SCALE, true);
}

/**
 * Draw the static parts of the hud. Only needed once per game.
 */
void renderHud(void) {
    int y = RENDER_HEIGHT + 8;

    drawText(4, y, "{}", 0);                        // Health symbol
    drawText(4 + 40 * TEXT_SCALE, y, "[]", 0);      // Keys symbol
    updateHud();
}

/**
 * Redraw the changing hud values.
 */
void updateHud(void) {
    int y = RENDER_HEIGHT + 8;
    int health_x = 4 + 12 * TEXT_SCALE;
    int keys_x = 4 + 52 * TEXT_SCALE;

    clearRect(health_x, y, 20 * TEXT_SCALE, CHAR_HEIGHT * TEXT_SCALE);
    clearRect(keys_x, y, 8 * TEXT_SCALE, CHAR_HEIGHT * TEXT_SCALE);

    drawTextNum(health_x, y, player.health);
    drawTextNum(keys_x, y, player.keys);
}

/**
 * Debug readout: fps, live entity count and player position.
 * Ported from renderStats() in the original. Built by `make debug`.
 */
void renderStats(void) {
#if DEBUG
    char buffer[40];
    int y = RENDER_HEIGHT + 8;
    int x = DISPLAY_WIDTH / 2;

    clearRect(x, y, DISPLAY_WIDTH - x, CHAR_HEIGHT * TEXT_SCALE);
    sprintf(buffer, "FPS %d E %d", (int) getActualFps(), num_entities);
    drawText(x, y, buffer, 0);
#endif
}

/**
 * Intro screen: logo, then wait for fire.
 */
void loopIntro(void) {
    int logo_scale = 3;
    int logo_w = BMP_LOGO_WIDTH * logo_scale;
    int logo_h = BMP_LOGO_HEIGHT * logo_scale;

    gfx_FillScreen(0);
    drawBitmap((DISPLAY_WIDTH - logo_w) / 2, (DISPLAY_HEIGHT - logo_h) / 3,
               bmp_logo_bits, BMP_LOGO_WIDTH, BMP_LOGO_HEIGHT, logo_scale, true);
    setFade(255);
    gfx_BlitBuffer();

    delay(1000);

    drawText(DISPLAY_WIDTH / 2 - 25 * TEXT_SCALE, (int)(DISPLAY_HEIGHT * .8), "PRESS FIRE", 1);
    gfx_BlitBuffer();

    // wait for fire
    while (!exit_scene) {
        input_setup();
        if (input_quit()) {
            quit_game = true;
            exit_scene = true;
            return;
        }
        if (input_fire()) jumpTo(GAME_PLAY);
    }
}

/**
 * The game itself.
 * Ported from loopGamePlay() in docs/doom-nano/doom-nano.ino.
 */
void loopGamePlay(void) {
    bool gun_fired = false;
    uint8_t gun_pos = 0;
    double rot_speed;
    double old_dir_x;
    double old_plane_x;
    double view_height = 0;
    double jogging = 0;
    uint8_t fade = 0;

    initializeLevel(level_data);

    num_entities = 0;
    num_static_entities = 0;

    // Clear both buffers once; from here renderMap only repaints the viewport
    gfx_FillScreen(0);
    gfx_SwapDraw();
    gfx_FillScreen(0);

    do {
        fps();

        // Read the keypad once; the input_* predicates below share that scan
        input_setup();

        // If the player is alive
        if (player.health > 0) {
            // Player speed
            if (input_up()) {
                player.velocity += (MOV_SPEED - player.velocity) * .4;
                jogging = fabs(player.velocity) * MOV_SPEED_INV;
            } else if (input_down()) {
                player.velocity += (-MOV_SPEED - player.velocity) * .4;
                jogging = fabs(player.velocity) * MOV_SPEED_INV;
            } else {
                player.velocity *= .5;
                jogging = fabs(player.velocity) * MOV_SPEED_INV;
            }

            // Player rotation
            if (input_right()) {
                rot_speed = ROT_SPEED * delta;
                old_dir_x = player.dir.x;
                player.dir.x = player.dir.x * cos(-rot_speed) - player.dir.y * sin(-rot_speed);
                player.dir.y = old_dir_x * sin(-rot_speed) + player.dir.y * cos(-rot_speed);
                old_plane_x = player.plane.x;
                player.plane.x = player.plane.x * cos(-rot_speed) - player.plane.y * sin(-rot_speed);
                player.plane.y = old_plane_x * sin(-rot_speed) + player.plane.y * cos(-rot_speed);
            } else if (input_left()) {
                rot_speed = ROT_SPEED * delta;
                old_dir_x = player.dir.x;
                player.dir.x = player.dir.x * cos(rot_speed) - player.dir.y * sin(rot_speed);
                player.dir.y = old_dir_x * sin(rot_speed) + player.dir.y * cos(rot_speed);
                old_plane_x = player.plane.x;
                player.plane.x = player.plane.x * cos(rot_speed) - player.plane.y * sin(rot_speed);
                player.plane.y = old_plane_x * sin(rot_speed) + player.plane.y * cos(rot_speed);
            }

            view_height = fabs(sin((double) millis() * JOGGING_SPEED)) * 6 * jogging;

            // Update gun
            if (gun_pos > GUN_TARGET_POS) {
                // Right after fire
                gun_pos -= 1;
            } else if (gun_pos < GUN_TARGET_POS) {
                // Showing up
                gun_pos += 2;
            } else if (!gun_fired && input_fire()) {
                // ready to fire and fire pressed
                gun_pos = GUN_SHOT_POS;
                gun_fired = true;
                fire();
            } else if (gun_fired && !input_fire()) {
                // just fired and restored position
                gun_fired = false;
            }
        } else {
            // The player is dead
            if (view_height > -10) view_height--;
            else if (input_fire()) jumpTo(INTRO);

            if (gun_pos > 1) gun_pos -= 2;
        }

        // Player movement
        if (fabs(player.velocity) > 0.003) {
            updatePosition(
                level_data,
                &(player.pos),
                player.dir.x * player.velocity * delta,
                player.dir.y * player.velocity * delta,
                false
            );
        } else {
            player.velocity = 0;
        }

        // Update things
        updateEntities(level_data);

        // Render stuff
        renderMap(level_data, view_height);
        renderEntities(view_height);
        renderGun(gun_pos, jogging);

        // Fade in effect
        if (fade < FADE_STEPS) {
            setFade(255 * fade / (FADE_STEPS - 1));
            fade++;

        } else {
            // Redrawn every frame: the screen is double buffered, so drawing
            // the hud once would only ever reach one of the two buffers.
            renderHud();
            renderStats();
        }

        // flash screen
        if (flash_screen > 0) {
            invert_screen = !invert_screen;
            flash_screen--;
            setInvert(invert_screen);
        } else if (invert_screen) {
            invert_screen = false;
            setInvert(false);
        }

        // Draw the frame
        gfx_SwapDraw();

        // Exit routine
        if (input_quit()) {
            quit_game = true;
            exit_scene = true;
            return;
        }
        if (input_left() && input_right()) {
            jumpTo(INTRO);
        }
    } while (!exit_scene);
}

/**
 * Main program entry point
 */
int main(void) {
    setup();

    // Run scenes until the player quits with [clear]
    while (!quit_game) {
        loop();
    }

    gfx_End();

    return 0;
}
