#include "display.h"
#include "sprites.h"
#include "fixed.h"

// Color indices into the grayscale palette installed by setupDisplay().
#define COLOR_BLACK 0
#define COLOR_WHITE 255

/**
 * Initialize the display system
 */
void setupDisplay(void) {
    // Initialize graphics
    gfx_Begin();
    gfx_SetDrawBuffer();

    // Install a 256 entry grayscale ramp so a color index doubles as a shading
    // intensity. The raycaster shades walls by distance, which the original
    // 1bpp build had to fake with dithering.
    setFade(255);

    // Set up display parameters
    gfx_SetColor(COLOR_WHITE);
    gfx_SetTextFGColor(COLOR_WHITE);
    gfx_SetTextBGColor(COLOR_BLACK);
    gfx_SetTextTransparentColor(COLOR_BLACK);
    gfx_SetTextScale(1, 1);
}

/**
 * Render the game map using raycasting algorithm.
 * Ported from renderMap() in docs/doom-nano/doom-nano.ino, with the maths
 * moved to 8.8 fixed point: the ez80 has no FPU, so the doubles this used to
 * use turned every DDA step into a pair of software float calls.
 *
 * Shading maps distance straight onto the grayscale palette installed by
 * setupDisplay(), where the 1bpp original had to dither.
 */
void renderMap(const uint8_t level[], double view_height) {
    UID last_uid = 0;

    // No clear here: drawColumn() paints ceiling, wall and floor for every
    // column, so the whole viewport is written exactly once per frame.

    // Camera state converted once per frame rather than once per column
    fixed pos_x   = dbl2fx(player.pos.x);
    fixed pos_y   = dbl2fx(player.pos.y);
    fixed dir_x   = dbl2fx(player.dir.x);
    fixed dir_y   = dbl2fx(player.dir.y);
    fixed plane_x = dbl2fx(player.plane.x);
    fixed plane_y = dbl2fx(player.plane.y);
    fixed bob     = dbl2fx(view_height * VIEW_SCALE_Y);

    // A ray running nearly along an axis gives a huge 1/ray. Clamping keeps
    // the running side distances inside 24 bits: the accumulator takes at
    // most MAX_RENDER_DEPTH steps, and 13 * 64 cells still fits comfortably.
    // The clamp must stay well above MAX_RENDER_DEPTH - clamping it down to
    // 16 reorders the DDA and picks the wrong wall on grazing rays.
    const fixed DELTA_MAX = int2fx(64);

    for (int x = 0; x < DISPLAY_WIDTH; x += RES_DIVIDER) {
        // camera_x = 2 * x / width - 1
        fixed camera_x = (fixed)(((long) x << (FIX_SHIFT + 1)) / DISPLAY_WIDTH) - FIX_ONE;
        fixed ray_x = dir_x + fxmul(plane_x, camera_x);
        fixed ray_y = dir_y + fxmul(plane_y, camera_x);

        uint8_t map_x = (uint8_t) fx2int(pos_x);
        uint8_t map_y = (uint8_t) fx2int(pos_y);

        fixed delta_x = fxabs(fxdiv(FIX_ONE, ray_x));
        fixed delta_y = fxabs(fxdiv(FIX_ONE, ray_y));
        if (delta_x > DELTA_MAX) delta_x = DELTA_MAX;
        if (delta_y > DELTA_MAX) delta_y = DELTA_MAX;

        int step_x;
        int step_y;
        fixed side_x;
        fixed side_y;

        if (ray_x < 0) {
            step_x = -1;
            side_x = fxmul(pos_x - int2fx(map_x), delta_x);
        } else {
            step_x = 1;
            side_x = fxmul(int2fx(map_x) + FIX_ONE - pos_x, delta_x);
        }

        if (ray_y < 0) {
            step_y = -1;
            side_y = fxmul(pos_y - int2fx(map_y), delta_y);
        } else {
            step_y = 1;
            side_y = fxmul(int2fx(map_y) + FIX_ONE - pos_y, delta_y);
        }

        // Wall detection (DDA)
        uint8_t depth = 0;
        bool hit = false;
        bool side = false;
        while (!hit && depth < MAX_RENDER_DEPTH) {
            if (side_x < side_y) {
                side_x += delta_x;
                map_x += step_x;
                side = false;
            } else {
                side_y += delta_y;
                map_y += step_y;
                side = true;
            }

            uint8_t block = getBlockAt(level, map_x, map_y);

            if (block == E_WALL) {
                hit = true;
            } else if (block == E_ENEMY || (block & 0x08)) {
                // Spawn entities as soon as they become visible. Same place as
                // the original: scanning for them separately would cost a lot.
                //
                // The original guards this with a distance check against a
                // map_coords it initialises to the player position and never
                // updates, so the check is always true. Reproduced by simply
                // not having it, which also saves a square root per block.
                UID uid = create_uid(block, map_x, map_y);
                if (last_uid != uid && !isSpawned(uid)) {
                    spawnEntity(block, map_x, map_y);
                    last_uid = uid;
                }
            }

            depth++;
        }

        if (hit) {
            fixed distance;

            if (side == false) {
                distance = fxdiv(int2fx(map_x) - pos_x + int2fx((1 - step_x) / 2), ray_x);
            } else {
                distance = fxdiv(int2fx(map_y) - pos_y + int2fx((1 - step_y) / 2), ray_y);
            }

            if (distance < FIX_ONE) distance = FIX_ONE;
            if (distance > int2fx(MAX_RENDER_DEPTH)) distance = int2fx(MAX_RENDER_DEPTH);

            // store zbuffer value for the column
            int z = fx2int(fxmul(distance, int2fx(DISTANCE_MULTIPLIER)));
            zbuffer[x / Z_RES_DIVIDER] = (uint8_t) min(z, 255);

            // rendered line height
            int line_height = fx2int(fxdiv(int2fx(RENDER_HEIGHT), distance));

            // Near walls are bright, far walls fade out. Walls facing along y
            // are darkened a step so corners stay readable, which is what the
            // original achieved by shifting two gradient levels.
            int shade = 255 - (int)(((long) distance * 255) / (MAX_RENDER_DEPTH * FIX_ONE));
            if (side) shade -= 48;
            if (shade < 24) shade = 24;
            if (shade > 255) shade = 255;

            int offset = fx2int(fxdiv(bob, distance));

            drawColumn(
                x,
                offset - line_height / 2 + RENDER_HEIGHT / 2,
                offset + line_height / 2 + RENDER_HEIGHT / 2,
                (uint8_t) shade
            );
        } else {
            // Nothing within render depth: still has to be painted, since
            // there is no separate clear pass any more.
            drawColumn(x, 0, 0, COLOR_BLACK);
            zbuffer[x / Z_RES_DIVIDER] = 255;
        }
    }
}

/**
 * Paint one raycaster column: black ceiling, shaded wall, black floor.
 *
 * Writing the whole column means renderMap no longer has to clear the
 * viewport first, which removes a 64000 byte wipe every frame. Pixels go
 * straight into the draw buffer; at this size the call overhead of the gfx
 * rectangle routines dominated the actual work.
 */
void drawColumn(int x, int start_y, int end_y, uint8_t intensity) {
    if (x < 0 || x >= DISPLAY_WIDTH) {
        return;
    }

    if (start_y < 0) start_y = 0;
    if (start_y > RENDER_HEIGHT) start_y = RENDER_HEIGHT;
    if (end_y > RENDER_HEIGHT) end_y = RENDER_HEIGHT;
    if (end_y < start_y) end_y = start_y;

    int width = RES_DIVIDER;
    if (x + width > DISPLAY_WIDTH) width = DISPLAY_WIDTH - x;

    uint8_t *p = &gfx_vbuffer[0][x];
    int y = 0;

    for (; y < start_y; y++, p += GFX_LCD_WIDTH) {          // ceiling
        for (int i = 0; i < width; i++) p[i] = COLOR_BLACK;
    }
    for (; y < end_y; y++, p += GFX_LCD_WIDTH) {            // wall
        for (int i = 0; i < width; i++) p[i] = intensity;
    }
    for (; y < RENDER_HEIGHT; y++, p += GFX_LCD_WIDTH) {    // floor
        for (int i = 0; i < width; i++) p[i] = COLOR_BLACK;
    }
}

/**
 * Draw a vertical line of a given shade, without touching the rest of the
 * column. Kept for the drawVLine() interface of the original.
 */
void drawVLine(int x, int start_y, int end_y, uint8_t intensity) {
    if (x < 0 || x >= DISPLAY_WIDTH) {
        return;
    }

    if (start_y < 0) start_y = 0;
    if (end_y > RENDER_HEIGHT) end_y = RENDER_HEIGHT;
    if (end_y <= start_y) {
        return;
    }

    int width = RES_DIVIDER;
    if (x + width > DISPLAY_WIDTH) width = DISPLAY_WIDTH - x;

    uint8_t *p = &gfx_vbuffer[start_y][x];
    for (int y = start_y; y < end_y; y++, p += GFX_LCD_WIDTH) {
        for (int i = 0; i < width; i++) p[i] = intensity;
    }
}

/**
 * Rebuild the palette at a given brightness, 0 black and 255 full.
 * The original faded by dithering pixels away; with a palette we can dim the
 * whole frame without touching the framebuffer, which is both cheaper and
 * smoother.
 */
void setFade(uint8_t level) {
    for (unsigned int i = 0; i < 256; i++) {
        unsigned int v = i * level / 255;
        gfx_palette[i] = gfx_RGBTo1555(v, v, v);
    }
}

/**
 * Flip the palette. Used for the damage flash, in place of the original's
 * display.invertDisplay().
 */
void setInvert(bool invert) {
    for (unsigned int i = 0; i < 256; i++) {
        unsigned int v = invert ? 255 - i : i;
        gfx_palette[i] = gfx_RGBTo1555(v, v, v);
    }
}

/**
 * Clear a rectangle to black.
 */
void clearRect(int x, int y, int w, int h) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > DISPLAY_WIDTH)  w = DISPLAY_WIDTH - x;
    if (y + h > DISPLAY_HEIGHT) h = DISPLAY_HEIGHT - y;
    if (w <= 0 || h <= 0) return;

    // Straight into the draw buffer. A full width clear is one contiguous
    // run, which matters because this used to be the per-frame screen wipe.
    if (w == DISPLAY_WIDTH) {
        memset(&gfx_vbuffer[y][0], COLOR_BLACK, (size_t) w * h);
        return;
    }

    for (int row = 0; row < h; row++) {
        memset(&gfx_vbuffer[y + row][x], COLOR_BLACK, (size_t) w);
    }
}

/**
 * Draw a 1bpp bitmap at an integer scale, skipping unset bits.
 * `color` picks whether set bits are drawn white or black, which is how the
 * gun composites its mask before its sprite.
 */
void drawBitmap(int x, int y, const uint8_t bitmap[], int16_t w, int16_t h, uint8_t scale, bool color, int clip_bottom) {
    int byte_width = (w + 7) / 8;
    uint8_t c = color ? COLOR_WHITE : COLOR_BLACK;

    if (clip_bottom > DISPLAY_HEIGHT) clip_bottom = DISPLAY_HEIGHT;

    for (int16_t by = 0; by < h; by++) {
        int sy = y + by * scale;
        if (sy + scale <= 0 || sy >= clip_bottom) continue;

        // Clip the scaled pixel's rows once per source row
        int row0 = sy < 0 ? 0 : sy;
        int row1 = sy + scale > clip_bottom ? clip_bottom : sy + scale;

        // Runs of set bits become a single wide fill per screen row
        const uint8_t *bits = bitmap + by * byte_width;
        int16_t bx = 0;
        while (bx < w) {
            if (!(bits[bx >> 3] & (0x80 >> (bx & 7)))) {
                bx++;
                continue;
            }

            int16_t run = bx + 1;
            while (run < w && (bits[run >> 3] & (0x80 >> (run & 7)))) {
                run++;
            }

            int col0 = x + bx * scale;
            int col1 = x + run * scale;
            bx = run;

            if (col1 <= 0 || col0 >= DISPLAY_WIDTH) continue;
            if (col0 < 0) col0 = 0;
            if (col1 > DISPLAY_WIDTH) col1 = DISPLAY_WIDTH;

            for (int row = row0; row < row1; row++) {
                memset(&gfx_vbuffer[row][col0], c, (size_t)(col1 - col0));
            }
        }
    }
}

/**
 * Draw a world sprite, scaled by distance and clipped against the zbuffer.
 * Ported from drawSprite() in docs/doom-nano/display.h. The source art is
 * 1bpp with a mask; a set mask bit means the pixel belongs to the sprite, so
 * black pixels inside the silhouette stay black rather than transparent.
 *
 * This walks the source art rather than the screen. Walking the screen needs
 * a divide per pixel to find the source texel, and a point blank enemy covers
 * the whole viewport - 64000 pixels and twice as many divides, which stalled
 * the frame. The source is at most 32x32 however close the enemy gets, so the
 * per-sprite work is now bounded and each texel becomes one clipped memset.
 */
void drawSprite(int x, int y, const uint8_t bitmap[], const uint8_t mask[], int16_t w, int16_t h, uint8_t sprite, double distance) {
    // Largest source sprite in the game is 32x32; the edge tables are sized
    // for that with room to spare.
    #define SPRITE_MAX_DIM 64
    int xedge[SPRITE_MAX_DIM + 1];
    int yedge[SPRITE_MAX_DIM + 1];

    if (w <= 0 || h <= 0 || w > SPRITE_MAX_DIM || h > SPRITE_MAX_DIM) {
        return;
    }

    // On-screen size. The magnification keeps sprites consistent with the
    // walls, which the raycaster draws at RENDER_HEIGHT / distance.
    int tw = (int)(w * VIEW_SCALE_X / distance);
    int th = (int)(h * VIEW_SCALE_Y / distance);

    if (tw <= 0 || th <= 0) {
        return;
    }

    // Don't draw the whole sprite if the anchor is hidden by the z buffer.
    // Not checked per pixel, for performance reasons.
    int zx = min(max(x, 0), DISPLAY_WIDTH - 1) / Z_RES_DIVIDER;
    if (zbuffer[zx] < distance * DISTANCE_MULTIPLIER) {
        return;
    }

    // Fully off screen?
    if (x >= DISPLAY_WIDTH || x + tw <= 0 || y >= RENDER_HEIGHT || y + th <= 0) {
        return;
    }

    int byte_width = w / 8;
    unsigned int sprite_offset = byte_width * h * sprite;

    // Screen edges of each source column and row. These are the only
    // divisions left, and there are w + h of them rather than one per pixel.
    for (int i = 0; i <= w; i++) {
        xedge[i] = x + (int)((long) i * tw / w);
    }
    for (int j = 0; j <= h; j++) {
        yedge[j] = y + (int)((long) j * th / h);
    }

    for (int sy = 0; sy < h; sy++) {
        int row0 = yedge[sy];
        int row1 = yedge[sy + 1];

        if (row1 <= 0 || row0 >= RENDER_HEIGHT) continue;
        if (row0 < 0) row0 = 0;
        if (row1 > RENDER_HEIGHT) row1 = RENDER_HEIGHT;
        if (row1 <= row0) continue;

        const uint8_t *bmp_row = bitmap + sprite_offset + sy * byte_width;
        const uint8_t *msk_row = mask + sprite_offset + sy * byte_width;

        // Coalesce neighbouring texels that are both visible and the same
        // colour, so a solid band becomes one memset per screen row instead
        // of one per texel.
        int sx = 0;
        while (sx < w) {
            uint8_t bit = 0x80 >> (sx & 7);

            if (!(msk_row[sx >> 3] & bit)) {
                sx++;
                continue;
            }

            uint8_t c = (bmp_row[sx >> 3] & bit) ? COLOR_WHITE : COLOR_BLACK;
            int run = sx + 1;
            while (run < w) {
                uint8_t rbit = 0x80 >> (run & 7);
                if (!(msk_row[run >> 3] & rbit)) break;
                uint8_t rc = (bmp_row[run >> 3] & rbit) ? COLOR_WHITE : COLOR_BLACK;
                if (rc != c) break;
                run++;
            }

            int col0 = xedge[sx];
            int col1 = xedge[run];
            sx = run;

            if (col1 <= 0 || col0 >= DISPLAY_WIDTH) continue;
            if (col0 < 0) col0 = 0;
            if (col1 > DISPLAY_WIDTH) col1 = DISPLAY_WIDTH;
            if (col1 <= col0) continue;

            for (int row = row0; row < row1; row++) {
                memset(&gfx_vbuffer[row][col0], c, (size_t)(col1 - col0));
            }
        }
    }

    #undef SPRITE_MAX_DIM
}

/**
 * Draw a single character from the original 4x6 font sheet.
 * Two characters share each byte, hence the nibble select.
 */
void drawChar(int x, int y, char ch) {
    uint8_t c = 0;

    // Find the character
    while (CHAR_MAP[c] != ch && CHAR_MAP[c] != '\0') c++;
    if (CHAR_MAP[c] == '\0') return;

    uint8_t bOffset = c / 2;

    for (uint8_t line = 0; line < CHAR_HEIGHT; line++) {
        uint8_t b = bmp_font[line * BMP_FONT_WIDTH + bOffset];
        int row0 = y + line * TEXT_SCALE;
        int row1 = row0 + TEXT_SCALE;

        if (row1 <= 0 || row0 >= DISPLAY_HEIGHT) continue;
        if (row0 < 0) row0 = 0;
        if (row1 > DISPLAY_HEIGHT) row1 = DISPLAY_HEIGHT;

        for (uint8_t n = 0; n < CHAR_WIDTH; n++) {
            if (!(b & (0x80 >> ((c % 2 == 0 ? 0 : 4) + n)))) continue;

            int col0 = x + n * TEXT_SCALE;
            int col1 = col0 + TEXT_SCALE;

            if (col1 <= 0 || col0 >= DISPLAY_WIDTH) continue;
            if (col0 < 0) col0 = 0;
            if (col1 > DISPLAY_WIDTH) col1 = DISPLAY_WIDTH;

            for (int row = row0; row < row1; row++) {
                memset(&gfx_vbuffer[row][col0], COLOR_WHITE, (size_t)(col1 - col0));
            }
        }
    }
}

/**
 * Draw a string using the original font, with `space` extra pixels between
 * characters (in font pixels, so it scales with the text).
 */
void drawText(int x, int y, const char *txt, uint8_t space) {
    int pos = x;

    for (const char *c = txt; *c != '\0'; c++) {
        drawChar(pos, y, *c);
        pos += (CHAR_WIDTH + space) * TEXT_SCALE;

        // shortcut on end of screen
        if (pos > DISPLAY_WIDTH) return;
    }
}

/**
 * Draw an integer (3 digits max).
 */
void drawTextNum(int x, int y, uint8_t num) {
    char buf[4];
    sprintf(buf, "%u", (unsigned) num);
    drawText(x, y, buf, 1);
}
