#ifndef _DOOMNANOCE_H
#define _DOOMNANOCE_H

// DoomNanoCE - TI-84 Plus CE Port Header File
// This file contains all the necessary includes and declarations for the port

// Standard C libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

// TI-84+CE specific libraries
#include <graphx.h>
#include <keypadc.h>
#include <sys/timers.h>

// Game constants
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define MAX_ENTITIES 32
#define MAX_STATIC_ENTITIES 16

// Key definitions for TI-84+CE
#define K_UP KEY_2ND
#define K_DOWN KEY_DEL
#define K_LEFT KEY_LEFT
#define K_RIGHT KEY_RIGHT
#define K_FIRE KEY_ENTER

// Game states
#define INTRO 0
#define GAME_PLAY 1

// Entity types (legend applies to level.h)
#define E_FLOOR             0x0   // . (also null)
#define E_WALL              0xF   // #
#define E_PLAYER            0x1   // P
#define E_ENEMY             0x2   // E
#define E_DOOR              0x4   // D
#define E_LOCKEDDOOR        0x5   // L
#define E_EXIT              0x7   // X
// collectable entities >= 0x8
#define E_MEDIKIT           0x8   // M
#define E_KEY               0x9   // K
#define E_FIREBALL          0xA   // not in map

// Entity statuses
#define S_STAND               0
#define S_ALERT               1
#define S_FIRING              2
#define S_MELEE               3
#define S_HIT                 4
#define S_DEAD                5
#define S_HIDDEN              6
#define S_OPEN                7
#define S_CLOSE               8

// Game configuration
#define FRAME_TIME          66.666666   // Desired time per frame in ms (66.666666 is ~15 fps)
#define RES_DIVIDER         2           // Higher values will result in lower horizontal resolution when rasterize and lower process and memory usage
                                        // Lower will require more process and memory, but looks nicer
#define Z_RES_DIVIDER       2           // Zbuffer resolution divider. We sacrifice resolution to save memory
#define DISTANCE_MULTIPLIER 20          // Distances are stored as uint8_t, multiplying the distance we can obtain more precision taking care
                                        // of keep numbers inside the type range. Max is 256 / MAX_RENDER_DEPTH
#define MAX_RENDER_DEPTH    12
#define MAX_SPRITE_DEPTH    8

#define ZBUFFER_SIZE        SCREEN_WIDTH / Z_RES_DIVIDER

// Level configuration  
#define LEVEL_WIDTH_BASE    6
#define LEVEL_WIDTH         (1 << LEVEL_WIDTH_BASE)
#define LEVEL_HEIGHT        57
#define LEVEL_SIZE          LEVEL_WIDTH / 2 * LEVEL_HEIGHT

// Weapon and game parameters
#define GUN_TARGET_POS        18
#define GUN_SHOT_POS          GUN_TARGET_POS + 4
#define ROT_SPEED             .12
#define PLAYER_SPEED          0.05
#define ENEMY_SPEED           0.03
#define FIREBALL_SPEED        0.08

// Collision and damage parameters
#define ENEMY_COLLIDER_DIST   10
#define ITEM_COLLIDER_DIST    15
#define ENEMY_MELEE_DIST      20
#define MAX_ENEMY_VIEW        100
#define ENEMY_MELEE_DAMAGE    20
#define ENEMY_FIREBALL_DAMAGE 30
#define GUN_MAX_DAMAGE        50
#define MAX_ENTITY_DISTANCE   100

// Game structures and types
typedef uint16_t UID;
typedef uint8_t  EType;

// Coordinate structure for 2D positions
typedef struct {
    double x;
    double y;
} Coords;

// Player structure
typedef struct {
    Coords pos;
    Coords dir;
    Coords plane;
    double velocity;
    uint8_t health;
    uint8_t keys;  
} Player;

// Entity structure
typedef struct {
    UID uid;
    Coords pos;
    uint8_t state;
    uint8_t health;     // angle for fireballs
    uint8_t distance;
    uint8_t timer;
} Entity;

// Static entity structure
typedef struct { 
    UID uid;
    uint8_t x;
    uint8_t y;
    bool active;
} StaticEntity;

// Function prototypes
void setup(void);
void loop(void);
void initializeLevel(const uint8_t level[]);
uint8_t getBlockAt(const uint8_t level[], uint8_t x, uint8_t y);
bool isSpawned(UID uid);
bool isStatic(UID uid);
void spawnEntity(uint8_t type, uint8_t x, uint8_t y);
void spawnFireball(double x, double y);
void removeEntity(UID uid, bool makeStatic);
void removeStaticEntity(UID uid);
UID detectCollision(const uint8_t level[], Coords *pos, double relative_x, double relative_y, bool only_walls);
void fire(void);
UID updatePosition(const uint8_t level[], Coords *pos, double relative_x, double relative_y, bool only_walls);
void updateEntities(const uint8_t level[]);
void renderMap(const uint8_t level[], double view_height);
void fps(void);
double getActualFps(void);
void drawPixel(int8_t x, int8_t y, bool color, bool raycasterViewport);
void drawVLine(uint8_t x, int8_t start_y, int8_t end_y, uint8_t intensity);
void drawSprite(int8_t x, int8_t y, const uint8_t bitmap[], const uint8_t mask[], int16_t w, int16_t h, uint8_t sprite, double distance);
void drawText(int8_t x, int8_t y, char *txt, uint8_t space);
void updateHud(void);
void handleInput(void);
Coords translateIntoView(Coords* pos);
double coords_distance(Coords* a, Coords* b);
UID create_uid(EType type, uint8_t x, uint8_t y);
EType uid_get_type(UID uid);

#endif // _DOOMNANOCE_H