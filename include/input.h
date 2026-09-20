/*
 * input.h
 * Input system header for DoomNanoCE port
 */

#ifndef INPUT_H
#define INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Key definitions for TI-84+CE
typedef enum {
    KEY_UP = 1,
    KEY_DOWN = 2, 
    KEY_LEFT = 3,
    KEY_RIGHT = 4,
    KEY_ENTER = 5,
    KEY_CLEAR = 6,
    KEY_2ND = 7,
    KEY_MODE = 8,
    KEY_X,TABLET = 9,
    KEY_ZOOM = 10,
    KEY_TRACE = 11,
    KEY_GRAPH = 12,
    KEY_WINDOW = 13,
    KEY_YEQU = 14,
    KEY_STORE = 15,
    KEY_ON = 16
} key_t;

// Input state structure
typedef struct {
    uint8_t keys[17];  // State of each key (0 = not pressed, 1 = pressed)
    int16_t joy_x;     // Joystick X axis (-128 to 127)
    int16_t joy_y;     // Joystick Y axis (-128 to 127)
} input_state_t;

// Function prototypes
void input_init(void);
void input_update(void);
uint8_t input_is_key_pressed(key_t key);
uint8_t input_is_key_released(key_t key);
uint8_t input_is_key_down(key_t key);
int16_t input_get_joystick_x(void);
int16_t input_get_joystick_y(void);

#ifdef __cplusplus
}
#endif

#endif // INPUT_H