#include "doomnanoce.h"

// Ported from docs/doom-nano/input.cpp. The original polls each button
// separately so several can be held at once; kb_Data gives us the whole
// keypad in one scan, so the game loop scans once per frame and then asks
// these predicates as often as it likes.

/**
 * Read the keypad. Call once per frame, before the input_* predicates.
 */
void input_setup(void) {
    kb_Scan();
}

bool input_up(void) {
    return kb_IsDown(K_UP) != 0;
}

bool input_down(void) {
    return kb_IsDown(K_DOWN) != 0;
}

bool input_left(void) {
    return kb_IsDown(K_LEFT) != 0;
}

bool input_right(void) {
    return kb_IsDown(K_RIGHT) != 0;
}

bool input_fire(void) {
    return kb_IsDown(K_FIRE) != 0;
}

/**
 * Not in the original, which runs until the calculator is switched off.
 * [clear] leaves the program and hands the calculator back to the OS.
 */
bool input_quit(void) {
    return kb_IsDown(K_QUIT) != 0;
}
