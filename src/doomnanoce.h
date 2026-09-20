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
#include <stdbool.h>
#include <time.h>
#include <sys/timers.h>

// Build/hardware configuration (DISPLAY_WIDTH, DISPLAY_HEIGHT, ...)
#include "config.h"

// Game constants
#define SCREEN_WIDTH  DISPLAY_WIDTH
#define SCREEN_HEIGHT DISPLAY_HEIGHT
#define HALF_WIDTH    (SCREEN_WIDTH / 2)
#define HALF_HEIGHT   (SCREEN_HEIGHT / 2)
#define RENDER_HEIGHT 200                 // raycaster viewport height (the rest is the hud)
#define MAX_ENTITIES 10
#define MAX_STATIC_ENTITIES 28

// The original targets a 128x64 display with a 56px tall viewport. The
// raycaster magnifies that onto this screen, so sprites and hud elements have
// to be magnified by the same factors to line up with the walls.
#define VIEW_SCALE_X  (SCREEN_WIDTH / 128.0)
#define VIEW_SCALE_Y  (RENDER_HEIGHT / 56.0)
#define TEXT_SCALE    2                   // the 4x6 font is drawn at 8x12
#define GUN_SCALE     3

// Key definitions for TI-84+CE (kb_lkey_t values, use with kb_IsDown)
#define K_UP    kb_KeyUp
#define K_DOWN  kb_KeyDown
#define K_LEFT  kb_KeyLeft
#define K_RIGHT kb_KeyRight
#define K_FIRE  kb_Key2nd
#define K_QUIT  kb_KeyClear

// Helpers. The original Arduino sketch relies on the min()/max() macros the
// Arduino core provides; the CE toolchain has no equivalent.
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

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
#define GUN_SHOT_POS          (GUN_TARGET_POS + 4)
#define ROT_SPEED             .12
#define ROT_ACCEL             .3          // How fast turning ramps up to ROT_SPEED.
                                          // A tap turns a fraction of a full step,
                                          // which is what makes fine aiming possible
                                          // when the frame rate is low.
#define FRAME_DELTA_MAX       2.0         // Upper bound on the frame time multiplier
#define MOV_SPEED             .2
#define MOV_SPEED_INV         5           // 1 / MOV_SPEED
#define JOGGING_SPEED         .005
#define ENEMY_SPEED           .02
#define FIREBALL_SPEED        .2
#define FIREBALL_ANGLES       45          // Num of angles per PI
#define FADE_STEPS            16          // Frames in the fade in/out effect

// Collision and damage parameters (distances are * DISTANCE_MULTIPLIER).
// Values match docs/doom-nano/constants.h.
#define MAX_ENTITY_DISTANCE    200
#define MAX_ENEMY_VIEW         80
#define ITEM_COLLIDER_DIST     6
#define ENEMY_COLLIDER_DIST    4
#define FIREBALL_COLLIDER_DIST 2
#define ENEMY_MELEE_DIST       6
#define WALL_COLLIDER_DIST     .2
#define ENEMY_MELEE_DAMAGE     8
#define ENEMY_FIREBALL_DAMAGE  20
#define GUN_MAX_DAMAGE         15

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

// Shared game state (defined in main.c)
extern Player player;
extern Entity entity[MAX_ENTITIES];
extern StaticEntity static_entity[MAX_STATIC_ENTITIES];
extern uint8_t num_entities;
extern uint8_t num_static_entities;
extern uint8_t scene;
extern bool exit_scene;
extern bool quit_game;
extern uint8_t flash_screen;
extern double delta;
extern uint8_t zbuffer[ZBUFFER_SIZE];

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
void setupDisplay(void);
void drawPixel(int x, int y, bool color, bool raycasterViewport);
void drawVLine(int x, int start_y, int end_y, uint8_t intensity);
void drawColumn(int x, int start_y, int end_y, uint8_t intensity);
void drawSprite(int x, int y, const uint8_t bitmap[], const uint8_t mask[], int16_t w, int16_t h, uint8_t sprite, double distance);
void drawBitmap(int x, int y, const uint8_t bitmap[], int16_t w, int16_t h, uint8_t scale, bool color, int clip_bottom);
void drawChar(int x, int y, char ch);
void drawText(int x, int y, const char *txt, uint8_t space);
void drawTextNum(int x, int y, uint8_t num);
void clearRect(int x, int y, int w, int h);
void setFade(uint8_t level);
void setInvert(bool invert);
void renderEntities(double view_height);
void renderGun(uint8_t gun_pos, double amount_jogging);
void renderHud(void);
void renderStats(void);
void updateHud(void);
void sortEntities(void);
void jumpTo(uint8_t target_scene);
void loopIntro(void);
void loopGamePlay(void);
void input_setup(void);
bool input_up(void);
bool input_down(void);
bool input_left(void);
bool input_right(void);
bool input_fire(void);
bool input_quit(void);
Coords translateIntoView(Coords* pos);
Coords create_coords(double x, double y);
uint8_t coords_distance(Coords* a, Coords* b);
UID create_uid(EType type, uint8_t x, uint8_t y);
EType uid_get_type(UID uid);
Entity create_entity(uint8_t type, uint8_t x, uint8_t y, uint8_t initialState, uint8_t initialHealth);
StaticEntity create_static_entity(UID uid, uint8_t x, uint8_t y, bool active);

#endif // _DOOMNANOCE_H