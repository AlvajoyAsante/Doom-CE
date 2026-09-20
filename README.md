<div align="center">
  <img src="docs/title_image.png" alt="Doom Nano CE" width="30%">
</div>

# Doom Nano CE

A port of [Doom Nano](https://github.com/daveruiz/doom-nano/) to the TI-84 Plus CE.

## Overview

Doom Nano CE is a lightweight 3D raycasting game adapted from the original Doom Nano project, which was designed for Arduino hardware.

> **Note:** This is not the original *Doom* game. It is an original raycasting game inspired by Doom and classic Wolfenstein 3D-style rendering.

## Features

- 3D raycasting engine
- Interactive environments
- Sprite-based enemies
- Collectible items and keys
- Collision detection
- Custom text rendering
- Optimized for the TI-84 Plus CE

## Requirements

### Hardware

- TI-84 Plus CE

### Development

- [CEdev](https://github.com/CE-Programming/toolchain)
- [CEmu](https://github.com/CE-Programming/CEmu)

## Building

```bash
make
```

Debug build:

```bash
make clean
make CC_DEBUG="" LD_DEBUG=""
```

Or:

```bash
./build.sh
```

The build produces:

```text
Doom.8xp
```

## Running

### Calculator

Transfer `Doom.8xp` to your calculator using TI Connect CE and run it from the program menu.

### CEmu

```bash
ceemu -run Doom.8xp
```

## Controls

| Button | Action |
|--------|--------|
| Arrow Keys | Move / strafe |
| Action Button | Interact |
| `2nd` + `MODE` | Exit |

## Project Structure

```text
DoomNanoCE/
├── src/         # Source code
├── include/     # Header files
├── docs/        # Documentation and assets
├── test/        # Tests
├── Makefile
├── build.sh
└── config.h
```

## Development Status

### Completed

- [x] Raycasting engine
- [x] Basic enemy AI
- [x] Item collection
- [x] Collision detection
- [x] Custom text rendering

### In Progress

- [ ] Advanced enemy behavior
- [ ] Doors and locks
- [ ] Game over screen
- [ ] Additional sprites

## Known Limitations

- Simplified enemy AI
- Limited sprite variety
- Limited graphical detail
- Audio and music are not yet fully implemented

## Credits

- **daveruiz** - Original Doom Nano project
- **lodev.org** - Raycasting resources
- **CEdev Team** - TI-84 Plus CE toolchain
- **TI-84 Plus CE community** - Development resources and testing

## License

See [`LICENSE`](LICENSE) for licensing information.

---

<div align="center">

**Doom Nano CE**

*A lightweight 3D raycasting game for the TI-84 Plus CE.*

</div>