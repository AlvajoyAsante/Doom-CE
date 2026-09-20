#include "display.h"
#include "sprites.h"

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
 * Ported from renderMap() in docs/doom-nano/doom-nano.ino. The original
 * dithered a 1bpp display to fake shading; here distance maps straight onto
 * the grayscale palette installed by setupDisplay().
 */
void renderMap(const uint8_t level[], double view_height) {
    UID last_uid = 0;

    // No clear here: drawColumn() paints ceiling, wall and floor for every
    // column, so the whole viewport is written exactly once per frame.

    for (int x = 0; x < DISPLAY_WIDTH; x += RES_DIVIDER) {
        double camera_x = 2 * (double) x / DISPLAY_WIDTH - 1;
        double ray_x = player.dir.x + player.plane.x * camera_x;
        double ray_y = player.dir.y + player.plane.y * camera_x;
        uint8_t map_x = (uint8_t) player.pos.x;
        uint8_t map_y = (uint8_t) player.pos.y;
        Coords map_coords = { player.pos.x, player.pos.y };
        double delta_x = fabs(1 / ray_x);
        double delta_y = fabs(1 / ray_y);

        int8_t step_x;
        int8_t step_y;
        double side_x;
        double side_y;

        if (ray_x < 0) {
            step_x = -1;
            side_x = (player.pos.x - map_x) * delta_x;
        } else {
            step_x = 1;
            side_x = (map_x + 1.0 - player.pos.x) * delta_x;
        }

        if (ray_y < 0) {
            step_y = -1;
            side_y = (player.pos.y - map_y) * delta_y;
        } else {
            step_y = 1;
            side_y = (map_y + 1.0 - player.pos.y) * delta_y;
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
                if (coords_distance(&(player.pos), &map_coords) < MAX_ENTITY_DISTANCE) {
                    UID uid = create_uid(block, map_x, map_y);
                    if (last_uid != uid && !isSpawned(uid)) {
                        spawnEntity(block, map_x, map_y);
                        last_uid = uid;
                    }
                }
            }

            depth++;
        }

        if (hit) {
            double distance;

            if (side == false) {
                distance = max(1, (map_x - player.pos.x + (1 - step_x) / 2) / ray_x);
            } else {
                distance = max(1, (map_y - player.pos.y + (1 - step_y) / 2) / ray_y);
            }

            // store zbuffer value for the column
            zbuffer[x / Z_RES_DIVIDER] = (uint8_t) min(distance * DISTANCE_MULTIPLIER, 255);

            // rendered line height
            int line_height = (int)(RENDER_HEIGHT / distance);

            // Near walls are bright, far walls fade out. Walls facing along y
            // are darkened a step so corners stay readable, which is what the
            // original achieved by shifting two gradient levels.
            int shade = 255 - (int)(distance / MAX_RENDER_DEPTH * 255.0);
            if (side) shade -= 48;
            if (shade < 24) shade = 24;
            if (shade > 255) shade = 255;

            // view_height is in the original's 56px viewport units
            int bob = (int)(view_height * VIEW_SCALE_Y / distance);

            drawColumn(
                x,
                bob - line_height / 2 + RENDER_HEIGHT / 2,
                bob + line_height / 2 + RENDER_HEIGHT / 2,
                (uint8_t) shade
            );
        } else {
            // Nothing within render depth: still has to be painted, since
            // there is no separate clear pass any more.
            drawColumn(x, 0, 0, COLOR_BLACK);
        }
    }
}

/**
 * Draw a single pixel on the screen
 */
void drawPixel(int x, int y, bool color, bool raycasterViewport) {
    // The raycaster may only draw inside its viewport; the hud owns the rest
    int max_y = raycasterViewport ? RENDER_HEIGHT : DISPLAY_HEIGHT;

    if (x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < max_y) {
        gfx_vbuffer[y][x] = color ? COLOR_WHITE : COLOR_BLACK;
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
void drawBitmap(int x, int y, const uint8_t bitmap[], int16_t w, int16_t h, uint8_t scale, bool color) {
    int byte_width = (w + 7) / 8;
    uint8_t c = color ? COLOR_WHITE : COLOR_BLACK;

    for (int16_t by = 0; by < h; by++) {
        int sy = y + by * scale;
        if (sy + scale <= 0 || sy >= DISPLAY_HEIGHT) continue;

        // Clip the scaled pixel's rows once per source row
        int row0 = sy < 0 ? 0 : sy;
        int row1 = sy + scale > DISPLAY_HEIGHT ? DISPLAY_HEIGHT : sy + scale;

        for (int16_t bx = 0; bx < w; bx++) {
            if (!(bitmap[by * byte_width + bx / 8] & (0x80 >> (bx % 8)))) continue;

            int sx = x + bx * scale;
            if (sx + scale <= 0 || sx >= DISPLAY_WIDTH) continue;

            int col0 = sx < 0 ? 0 : sx;
            int col1 = sx + scale > DISPLAY_WIDTH ? DISPLAY_WIDTH : sx + scale;

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
 */
void drawSprite(int x, int y, const uint8_t bitmap[], const uint8_t mask[], int16_t w, int16_t h, uint8_t sprite, double distance) {
    // On-screen size. The magnification keeps sprites consistent with the
    // walls, which the raycaster draws at RENDER_HEIGHT / distance.
    int tw = (int)(w * VIEW_SCALE_X / distance);
    int th = (int)(h * VIEW_SCALE_Y / distance);
    int byte_width = w / 8;
    unsigned int sprite_offset = byte_width * h * sprite;

    if (tw <= 0 || th <= 0) {
        return;
    }

    // Don't draw the whole sprite if the anchor is hidden by the z buffer.
    // Not checked per pixel, for performance reasons.
    int zx = min(max(x, 0), DISPLAY_WIDTH - 1) / Z_RES_DIVIDER;
    if (zbuffer[zx] < distance * DISTANCE_MULTIPLIER) {
        return;
    }

    // Clip once up front instead of testing every pixel
    int ty0 = y < 0 ? -y : 0;
    int ty1 = y + th > RENDER_HEIGHT ? RENDER_HEIGHT - y : th;
    int tx0 = x < 0 ? -x : 0;
    int tx1 = x + tw > DISPLAY_WIDTH ? DISPLAY_WIDTH - x : tw;

    for (int ty = ty0; ty < ty1; ty++) {
        int sy = ty * h / th;   // the y from the sprite
        const uint8_t *bmp_row = bitmap + sprite_offset + sy * byte_width;
        const uint8_t *msk_row = mask + sprite_offset + sy * byte_width;
        uint8_t *dst = &gfx_vbuffer[y + ty][x];

        for (int tx = tx0; tx < tx1; tx++) {
            int sx = tx * w / tw;   // the x from the sprite
            uint8_t bit = 0x80 >> (sx & 7);

            if (msk_row[sx / 8] & bit) {
                dst[tx] = (bmp_row[sx / 8] & bit) ? COLOR_WHITE : COLOR_BLACK;
            }
        }
    }
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

    gfx_SetColor(COLOR_WHITE);
    for (uint8_t line = 0; line < CHAR_HEIGHT; line++) {
        uint8_t b = bmp_font[line * BMP_FONT_WIDTH + bOffset];

        for (uint8_t n = 0; n < CHAR_WIDTH; n++) {
            if (b & (0x80 >> ((c % 2 == 0 ? 0 : 4) + n))) {
                gfx_FillRectangle(x + n * TEXT_SCALE, y + line * TEXT_SCALE,
                                  TEXT_SCALE, TEXT_SCALE);
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
