#include "doomnanoce.h"

/**
 * Initialize input handling
 */
void input_setup(void) {
    keypad_Init();
}

/**
 * Handle game input from TI-84+CE keypad
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