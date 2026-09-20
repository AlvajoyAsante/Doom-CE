#ifndef FIXED_H
#define FIXED_H

#include <stdint.h>

// Fixed point maths for the raycaster.
//
// The ez80 has no FPU, so every `double` operation in the hot loop is a
// software routine. `int` is 24 bits wide on this toolchain and maps straight
// onto the ez80's native registers, which makes it the right carrier for a
// fixed point type. 11 fractional bits give a range of +/-4095.99 at a
// resolution of 1/2048, which was picked by comparing the raycaster's output
// against the double version it replaced: at 11 bits every column agrees to
// within one pixel, while 12 bits overflows 24 bits in the distance term and
// 8 bits leaves visible stair-stepping on close walls.
//
// Approach follows CodePenguino/TI-84-CE-Wolfenstein, which uses the same
// idea with 8 fractional bits over a 24 bit int.

typedef int fixed;

#define FIX_SHIFT 11
#define FIX_ONE   (1 << FIX_SHIFT)
#define FIX_HALF  (FIX_ONE / 2)
#define FIX_MAX   8388607           // INT_MAX for a 24 bit int

#define int2fx(n)  ((fixed)(n) << FIX_SHIFT)
#define fx2int(f)  ((int)((f) >> FIX_SHIFT))
#define dbl2fx(d)  ((fixed)((d) * FIX_ONE))
#define fx2dbl(f)  ((double)(f) / FIX_ONE)

/**
 * Multiply. The intermediate needs more than 24 bits, so it goes through a
 * 32 bit long - still far cheaper than a software float multiply.
 */
static inline fixed fxmul(fixed a, fixed b) {
    return (fixed)(((long)a * (long)b) >> FIX_SHIFT);
}

/**
 * Divide. Returns a saturated value rather than trapping on a zero divisor,
 * which happens naturally in the raycaster when a ray runs exactly along an
 * axis.
 */
static inline fixed fxdiv(fixed a, fixed b) {
    if (b == 0) {
        return a < 0 ? -FIX_MAX : FIX_MAX;
    }
    return (fixed)(((long)a << FIX_SHIFT) / (long)b);
}

static inline fixed fxabs(fixed a) {
    return a < 0 ? -a : a;
}

#endif // FIXED_H
