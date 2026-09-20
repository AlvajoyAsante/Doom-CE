# ----------------------------
# Makefile Options
# ----------------------------

NAME = DOOM
ICON = icon.png
DESCRIPTION = "Doom Nano port for the TI-84 Plus CE"
COMPRESSED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

# The toolchain's default goal is named "build". Because this repo also has a
# build.sh, make's built-in "%: %.sh" rule would otherwise copy it to an
# executable named "build" on every run.
.PHONY: build

# "test" runs the CEmu autotester on autotest.json. It has to be phony too,
# otherwise make sees the test/ directory and decides there is nothing to do.
.PHONY: test

include $(shell cedev-config --makefile)

# Always build before handing the .8xp to the emulator. The recipe itself comes
# from the toolchain makefile included above; this only adds the prerequisite.
test: build
