# Tests

Automated tests run the built `.8xp` inside the headless CEmu core
(`cemu-autotester`, shipped with CEdev) and compare CRCs of the calculator's
VRAM against expected values. The test definition lives in
[`../autotest.json`](../autotest.json).

## Running

```bash
make test
```

`test` depends on `build`, so this always tests the current sources. `./build.sh`
also runs the tests after a successful build, and skips them with a notice if no
ROM is present.

## The ROM

`autotest.json` points at `test/ti84pce.rom`. Calculator ROMs are copyrighted and
cannot be committed, so this file is gitignored and you have to supply your own:

1. Dump the ROM from a TI-84 Plus CE you own. The CEmu project documents the
   procedure, which uses a ROM dumper program run on the calculator itself.
2. Save it as `test/ti84pce.rom`.

Without it the autotester stops with `"rom" parameter not given or invalid`.

## Status: the harness cannot launch programs yet

**`make test` currently cannot pass, and a pass would not mean anything.**

With the OS 5.3.0 ROM in this directory, `action|launch` never starts the
program. Everything else in the chain works, which is what makes this
confusing: the emulator boots, the OS is responsive, and the hashing reads the
real screen. Typing `2`, `4` and `1` on the home screen produces three distinct
CRCs, so input and hashing are fine.

What proves the program never runs: a minimal probe that does nothing but
`gfx_FillScreen(c)` produces the *same* CRC for `c = 1`, `2` and `255`. Three
completely different screens cannot hash identically, so those pixels are not
ours - they are the OS home screen, whose blinking cursor also makes the CRC
depend on the delay. A stock CEdev example (`graphx/shapes`) likewise fails all
eight of its own shipped hashes on this ROM.

Ruled out: `isASM` true and false behave the same, transferring `clibs.8xg`
alongside the program changes nothing, and longer boot delays or dismissing the
screen with `[clear]` before launching change nothing.

Most likely cause is a mismatch between this 2020 OS 5.3.0 dump and the launch
mechanism in the current autotester build. The next thing to try is a different
ROM version. Until then the `expected_CRCs` in `autotest.json` are the string
`PLACEHOLDER`, so the tests fail loudly rather than reporting a meaningless
pass; `./build.sh --no-test` skips them.

## What is tested

| Hash | Assertion |
|------|-----------|
| 1 | After launching, a gameplay frame renders: raycast walls plus the health/keys hud |
| 2 | Pressing `[clear]` quits the game and hands back a responsive OS home screen |

Once launching works, hash 2 is the blank home screen, reached by pressing `[clear]` three times: once
to quit the game, then twice more to clear the OS home screen. Those five
expected CRCs are the same ones every CEdev example uses, and they hold across
OS versions — so this test also proves the program exited cleanly rather than
hanging or crashing.

## Refreshing hash 1 after a rendering change

Hash 1 covers actual pixels, so **any** deliberate change to the renderer (and
the raycaster is still a placeholder, so expect several) will fail it. It is
also specific to your ROM's OS version: the expected CRCs shipped with the CEdev
examples do not reproduce on every ROM, and neither will this one.

To re-record it:

```bash
./test/capture-hashes.sh
```

The script runs the autotester and prints the CRC the emulator actually produced
for each failing hash. Confirm the change was intended, then paste the new value
into the matching `expected_CRCs` array in `autotest.json`.

A hash may list several CRCs; the test passes if the emulator matches any one of
them. That is the mechanism used to tolerate several OS versions.
