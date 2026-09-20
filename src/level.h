#ifndef LEVEL_H
#define LEVEL_H

#include "doomnanoce.h"

// Level data declaration
extern const uint8_t level_data[];

// Function prototypes for level operations
uint8_t getBlockAt(const uint8_t level[], uint8_t x, uint8_t y);

#endif // LEVEL_H