#!/usr/bin/env python3
"""Check the fixed point raycaster against the double implementation it replaced.

The raycaster in src/display.c was converted from `double` to fixed point for
speed, because the ez80 has no FPU. This replays both versions of the maths in
Python with C semantics - 24-bit wraparound, truncating integer division,
arithmetic shift - over the real level data from src/level.c, sweeping the
camera through a full turn from several positions.

It reports how far the two disagree and whether any intermediate would have
overflowed 24 or 32 bits. FIX_SHIFT and the delta clamp are read from the
sources, so re-run this after changing either.

Usage: python test/verify-fixed-point.py
"""
import math
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

W, H = 64, 57
DISPLAY_WIDTH = 320
RENDER_HEIGHT = 200
RES_DIVIDER = 2
MAX_RENDER_DEPTH = 12
E_WALL = 0xF
INT24_MIN, INT24_MAX = -8388608, 8388607
INT32_MIN, INT32_MAX = -2147483648, 2147483647


def read_sources():
    level = open(os.path.join(ROOT, 'src', 'level.c'), encoding='utf-8').read()
    body = level[level.index('level_data[LEVEL_SIZE] = {'):level.index('};')]
    data = [int(v, 16) for v in re.findall(r'0x([0-9a-f]{2})', body)]

    fixed_h = open(os.path.join(ROOT, 'src', 'fixed.h'), encoding='utf-8').read()
    shift = int(re.search(r'#define FIX_SHIFT (\d+)', fixed_h).group(1))

    display = open(os.path.join(ROOT, 'src', 'display.c'), encoding='utf-8').read()
    clamp = int(re.search(r'DELTA_MAX = int2fx\((\d+)\)', display).group(1))
    return data, shift, clamp


DATA, FIX_SHIFT, DELTA_CELLS = read_sources()
FIX_ONE = 1 << FIX_SHIFT
assert len(DATA) == W // 2 * H, 'unexpected level size %d' % len(DATA)

overflow24 = 0
overflow32 = 0


def w24(v):
    global overflow24
    if v > INT24_MAX or v < INT24_MIN:
        overflow24 += 1
    return v


def w32(v):
    global overflow32
    if v > INT32_MAX or v < INT32_MIN:
        overflow32 += 1
    return v


def cdiv(a, b):
    """C integer division truncates toward zero; Python floors."""
    q = abs(a) // abs(b)
    return q if (a < 0) == (b < 0) else -q


def block_at(x, y):
    if x < 0 or y < 0 or x >= W or y >= H:
        return 0
    return (DATA[((H - 1 - y) * W + x) // 2] >> ((not (x % 2)) * 4)) & 0x0F


def u8(v):
    return v & 0xFF


def cast_double(px, py, dx, dy, plx, ply):
    out = []
    for x in range(0, DISPLAY_WIDTH, RES_DIVIDER):
        camera_x = 2 * x / DISPLAY_WIDTH - 1
        rx, ry = dx + plx * camera_x, dy + ply * camera_x
        mx, my = u8(int(px)), u8(int(py))
        ddx = abs(1 / rx) if rx else float('inf')
        ddy = abs(1 / ry) if ry else float('inf')
        if rx < 0:
            xs, sx = -1, (px - mx) * ddx
        else:
            xs, sx = 1, (mx + 1.0 - px) * ddx
        if ry < 0:
            ys, sy = -1, (py - my) * ddy
        else:
            ys, sy = 1, (my + 1.0 - py) * ddy
        depth, hit, side = 0, False, False
        while not hit and depth < MAX_RENDER_DEPTH:
            if sx < sy:
                sx += ddx; mx = u8(mx + xs); side = False
            else:
                sy += ddy; my = u8(my + ys); side = True
            hit = block_at(mx, my) == E_WALL
            depth += 1
        if not hit:
            out.append(None); continue
        dist = ((mx - px + (1 - xs) / 2) / rx) if not side else ((my - py + (1 - ys) / 2) / ry)
        dist = min(max(1, dist), MAX_RENDER_DEPTH)
        shade = 255 - int(dist / MAX_RENDER_DEPTH * 255) - (48 if side else 0)
        out.append((int(RENDER_HEIGHT / dist), max(24, min(255, shade))))
    return out


def cast_fixed(px, py, dx, dy, plx, ply):
    def fxmul(a, b):
        return w24(w32(a * b) >> FIX_SHIFT)

    def fxdiv(a, b):
        if b == 0:
            return INT24_MAX if a >= 0 else -INT24_MAX
        return w24(cdiv(w32(a << FIX_SHIFT), b))

    pos_x, pos_y = int(px * FIX_ONE), int(py * FIX_ONE)
    dir_x, dir_y = int(dx * FIX_ONE), int(dy * FIX_ONE)
    pl_x, pl_y = int(plx * FIX_ONE), int(ply * FIX_ONE)
    dmax = DELTA_CELLS * FIX_ONE
    out = []
    for x in range(0, DISPLAY_WIDTH, RES_DIVIDER):
        camera_x = cdiv(w32(x << (FIX_SHIFT + 1)), DISPLAY_WIDTH) - FIX_ONE
        rx = dir_x + fxmul(pl_x, camera_x)
        ry = dir_y + fxmul(pl_y, camera_x)
        mx, my = u8(pos_x >> FIX_SHIFT), u8(pos_y >> FIX_SHIFT)
        ddx = min(abs(fxdiv(FIX_ONE, rx)), dmax)
        ddy = min(abs(fxdiv(FIX_ONE, ry)), dmax)
        if rx < 0:
            xs, sx = -1, fxmul(pos_x - (mx << FIX_SHIFT), ddx)
        else:
            xs, sx = 1, fxmul((mx << FIX_SHIFT) + FIX_ONE - pos_x, ddx)
        if ry < 0:
            ys, sy = -1, fxmul(pos_y - (my << FIX_SHIFT), ddy)
        else:
            ys, sy = 1, fxmul((my << FIX_SHIFT) + FIX_ONE - pos_y, ddy)
        depth, hit, side = 0, False, False
        while not hit and depth < MAX_RENDER_DEPTH:
            if sx < sy:
                sx = w24(sx + ddx); mx = u8(mx + xs); side = False
            else:
                sy = w24(sy + ddy); my = u8(my + ys); side = True
            hit = block_at(mx, my) == E_WALL
            depth += 1
        if not hit:
            out.append(None); continue
        if not side:
            dist = fxdiv((mx << FIX_SHIFT) - pos_x + (((1 - xs) // 2) << FIX_SHIFT), rx)
        else:
            dist = fxdiv((my << FIX_SHIFT) - pos_y + (((1 - ys) // 2) << FIX_SHIFT), ry)
        dist = max(FIX_ONE, min(dist, MAX_RENDER_DEPTH << FIX_SHIFT))
        lh = cdiv(w32(RENDER_HEIGHT << FIX_SHIFT), dist)
        shade = 255 - cdiv(w32(dist * 255), MAX_RENDER_DEPTH * FIX_ONE) - (48 if side else 0)
        out.append((lh, max(24, min(255, shade))))
    return out


SPAWNS = [(29.5, 10.5), (10.5, 30.5), (34.5, 24.5), (5.5, 45.5), (48.5, 48.5)]


def closest_tie(px, py, dx, dy, plx, ply, x):
    """Smallest |side_x - side_y| seen while marching this ray, in double.

    The comparison is ambiguous when that gap is smaller than the error the
    fixed point version can accumulate. delta carries up to one quantum of
    rounding error (1/FIX_ONE) and the DDA adds it once per step, so after at
    most MAX_RENDER_DEPTH steps the accumulated bound is
    MAX_RENDER_DEPTH/FIX_ONE. Inside that band the ray is effectively passing
    through a cell corner: which axis steps first is arbitrary, and the two
    implementations are equally right to differ.
    """
    camera_x = 2 * x / DISPLAY_WIDTH - 1
    rx, ry = dx + plx * camera_x, dy + ply * camera_x
    mx, my = int(px), int(py)
    ddx = abs(1 / rx) if rx else float('inf')
    ddy = abs(1 / ry) if ry else float('inf')
    xs, sx = (-1, (px - mx) * ddx) if rx < 0 else (1, (mx + 1.0 - px) * ddx)
    ys, sy = (-1, (py - my) * ddy) if ry < 0 else (1, (my + 1.0 - py) * ddy)
    closest = float('inf')
    for _ in range(MAX_RENDER_DEPTH):
        closest = min(closest, abs(sx - sy))
        if sx < sy:
            sx += ddx; mx += xs
        else:
            sy += ddy; my += ys
        if block_at(u8(mx), u8(my)) == E_WALL:
            break
    return closest


exact = within1 = total = flips = 0
worst_h = worst_s = 0
ties = []
real_errors = []

for px, py in SPAWNS:
    if block_at(int(px), int(py)) == E_WALL:
        continue
    for deg in range(0, 360, 3):
        a = math.radians(deg)
        dx, dy = math.cos(a), math.sin(a)
        plx, ply = -0.66 * math.sin(a), 0.66 * math.cos(a)
        d = cast_double(px, py, dx, dy, plx, ply)
        f = cast_fixed(px, py, dx, dy, plx, ply)
        for i, (cd, cf) in enumerate(zip(d, f)):
            if (cd is None) != (cf is None):
                flips += 1
                continue
            if cd is None:
                continue
            total += 1
            eh, es = abs(cd[0] - cf[0]), abs(cd[1] - cf[1])
            worst_h, worst_s = max(worst_h, eh), max(worst_s, es)
            exact += eh == 0
            within1 += eh <= 1
            if eh > 1:
                gap = closest_tie(px, py, dx, dy, plx, ply, i * RES_DIVIDER)
                ambiguous = float(MAX_RENDER_DEPTH) / FIX_ONE
                (ties if gap < ambiguous else real_errors).append((eh, gap))

print('FIX_SHIFT=%d (1/%d), delta clamp=%d cells' % (FIX_SHIFT, FIX_ONE, DELTA_CELLS))
print('columns compared        : %d' % total)
print('identical wall height   : %.1f%%' % (100.0 * exact / total))
print('within 1px              : %.2f%%' % (100.0 * within1 / total))
print('worst wall height diff  : %dpx of %d' % (worst_h, RENDER_HEIGHT))
print('worst shade diff        : %d of 255' % worst_s)
print('grazing rays hitting a different wall: %d' % flips)
print('24-bit overflows        : %d' % overflow24)
print('32-bit overflows        : %d' % overflow32)
print()
print('columns off by >1px     : %d' % (len(ties) + len(real_errors)))
print('  of those, corner ties : %d  (DDA gap within the accumulated rounding' % len(ties))
print('                             bound %.6f, so the tie-break is arbitrary)'
      % (float(MAX_RENDER_DEPTH) / FIX_ONE))
print('  genuine precision loss: %d' % len(real_errors))
if real_errors:
    for eh, gap in sorted(real_errors, reverse=True)[:5]:
        print('     %dpx error with a DDA gap of %.6f' % (eh, gap))

failed = bool(overflow24 or overflow32 or real_errors)
print()
print('FAIL' if failed else 'OK')
sys.exit(1 if failed else 0)
