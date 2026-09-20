# DoomNanoCE

A Doom port of the [https://github.com/daveruiz/doom-nano/](https://github.com/daveruiz/doom-nano/) to the TI-84+CE calculator using the CE Dev Chain.

## Project Status

This is an ongoing port of the Doom Nano 3D raycasting engine to the TI-84+CE calculator. The project is currently in early development stages with basic structure and functionality implemented.

## Features Implemented

- Basic raycasting engine structure
- Entity system (player, enemies, items)
- Input handling for TI-84+CE keypad
- Simple level rendering placeholder
- Basic collision detection
- Game loop and timing control

## Build Instructions

1. Make sure you have the CE Dev Chain installed
2. Run `make` to build the project
3. Use `make install` to install to your calculator
4. Use `make emu` to run in CEmu emulator

## Project Structure

```
.
├── src/              # Source code files
│   ├── main.c        # Main program entry point
│   ├── doomnanoce.h  # Main header file
│   ├── types.c       # Utility functions for coordinates and UIDs
│   ├── entities.c    # Entity creation and management
│   ├── input.c       # Input handling
│   ├── level.c       # Level data and rendering
│   ├── display.c     # Display and rendering functions
│   └── config.h      # Configuration settings
├── include/          # Include files (empty for now)
├── docs/             # Documentation
├── test/             # Test files (empty for now)
└── Makefile          # Build configuration
```

## Development Notes

This project is based on the original Arduino Doom Nano implementation but adapted for TI-84+CE hardware constraints and the eZ80 processor architecture. Key differences include:

- 24-bit addressing mode (ADL) instead of 8-bit AVR
- Different graphics subsystem using graphx.h
- Input handling via keypadc.h 
- Memory management optimized for calculator's limited resources

## Next Steps

1. Implement full raycasting engine
2. Add proper sprite rendering
3. Implement sound system
4. Create more complex levels
5. Add enemy AI improvements
6. Implement item collection and health systems
7. Add proper collision detection
8. Optimize performance for calculator hardware

## License

This project is licensed under the MIT License - see the LICENSE file for details.