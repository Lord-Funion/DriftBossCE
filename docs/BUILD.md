# Building Drift Boss CE

## Toolchain

Use [CE C/C++ Toolchain v15.0](https://github.com/CE-Programming/toolchain/releases/tag/v15.0). Extract it to a path without spaces and add its `bin` directory to `PATH`. On Windows, launching its `cedev.bat` environment is another option. See the [official getting-started guide](https://ce-programming.github.io/toolchain/static/getting-started.html).

From a checkout of this repository:

```sh
make
```

Output: **`bin/DRIFTB.8xp`**. The committed `makefile`, `icon.png`, and generated C tables are sufficient; the original browser game and an asset-conversion pipeline are not needed to rebuild the native program.

## Recreate the graphics AppVars

With Python 3 and `convbin` on `PATH`:

```sh
python tools/build_assets.py
```

This converts the three supplied `assets/DBAS*.bin` banks into `bin/DBAS*.8xv`. It does not regenerate or alter the artwork. Equivalent commands are:

```sh
convbin -j bin -k 8xv -i assets/DBAS0.bin -o bin/DBAS0.8xv -n DBAS0 -r
convbin -j bin -k 8xv -i assets/DBAS1.bin -o bin/DBAS1.8xv -n DBAS1 -r
convbin -j bin -k 8xv -i assets/DBAS2.bin -o bin/DBAS2.8xv -n DBAS2 -r
```

The four runtime libraries are provided in `dist/` with their license. Transfer the rebuilt program, all three banks, and the libraries to the calculator.

## Verify

```sh
python tools/verify_release.py
python tools/verify_release.py --built bin/DRIFTB.8xp
```

The first validates all eight committed TI transfer files, names, archive flags, checksums, and raw bank payloads. It also checks every sprite’s sparse-row bounds. The second additionally compares the program variable payload against the tested v0.2 release. Comment headers in the TI transfer file are excluded from that comparison.

Run the existing core rules/replay regression with a host C compiler:

```sh
cc -std=c11 -O2 -Isrc tests/test_core.c src/game.c -o core-tests
./core-tests
```

On Windows, the equivalent output may be `core-tests.exe`. The test uses host 32-bit positions; the target uses native 24-bit positions. The replay was also verified on the eZ80 CPU core. No ROM is distributed.

## Project layout

| Path | Role |
|---|---|
| `src/game.c` / `game.h` | 60 Hz fixed-point movement, track generation, contact, garage, boosters, scoring, save checks |
| `src/render.c` / `render.h` | Orthographic projection, sprite caching, native RLET rendering, UI |
| `src/main.c` | Calculator input, file loading, timers, frame pacing, save/exit |
| `src/assets.c` / `assets.h` | Shared palette and offsets for 133 generated sprites |
| `assets/DBAS*.bin` | Ready-to-use converted artwork banks |
| `dist/` | Tested calculator program and runtime files |
| `tests/` | Core rules and deterministic 1,694-tick replay |
| `docs/media/` | Actual native-renderer screenshots and demonstration GIFs |
| `tools/` | Asset packaging, validation, release/media helpers |
| `cemetech/` | Listing text and suggested upload metadata |

## Runtime design

World positions use Q8 fixed point. On the CE, `fx` is the native signed 24-bit `int`; on the host it is `int32_t`. World rebasing keeps positions bounded. A nearby-tile list is shared by wheel/contact substeps. The 60 Hz simulation is separate from rendering, which has a 30 FPS cap and bounded catch-up.

Archived graphics banks begin with `DBCEAS1` and hold width/height plus sparse row runs. A lazy 48 KiB arena converts hot sprites to graphx RLET format and reuses them. RLET copies and screen clipping are handled by graphx. The on-calculator save format remains compatible with v0.1.

The release deliberately uses the conservative **`-Oz`** build. Keep the toolchain and flags fixed when comparing binary payloads. A clean source build was checked against the shipped program before publication.

## Media

See [MEDIA.md](MEDIA.md) for capture provenance and GIF export details. Capture fixtures and a CEmu-core runner are in `tools/capture/`; a calculator ROM is not needed for that synthetic harness. Full OS startup, shell integration, and save I/O still require real-device testing.
