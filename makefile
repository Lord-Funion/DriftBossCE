NAME = DRIFTB
ICON = icon.png
DESCRIPTION = "Drift Boss CE"
COMPRESSED = YES
COMPRESSED_MODE = zx7
ARCHIVED = YES
CFLAGS = -Wall -Wextra -Oz
LTO = YES
include $(shell cedev-config --makefile)
