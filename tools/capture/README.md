# Native media capture fixture

These are developer tools, not calculator files. Obtain the [CEmu core source](https://github.com/CE-Programming/CEmu) and CE toolchain v15.0 separately; no calculator ROM is needed for this synthetic harness.

1. Create a temporary CE project using this `makefile`. Copy the repository's `game.c`, `game.h`, `render.c`, `render.h`, `assets.c`, and `assets.h` into its `src/` directory. Add this fixture's `main.c` and `inputs.h`.
2. Build with `make` to produce `bin/BENCH.obj` and `.map`.
3. Run `python tools/capture/patch_graphx.py path/to/BENCH.obj patches.bin`. The script reads the committed banks and graphx runtime. It locates `graphx.lib` using `CEDEV` or `cedev-config` on PATH.
4. Compile `runner.c` against the CEmu core as a host executable. Include the core headers/C sources, using only `os-win32.c` on Windows or `os-linux.c` on Linux. Source selection for debug/ARM components depends on the CEmu version. CEmu is not vendored here.
5. Read the `_main` address from the `.map`, create an output directory, then run `runner BENCH.obj ENTRY_HEX patches.bin OUTPUT_DIRECTORY`.

The runner outputs 435 raw 320×240 indexed framebuffers, `frame-times.csv`, and a 19-field `final-state.bin`. Convert pixels using `game_palette` in `src/assets.c`. A capture/acknowledgment handshake prevents rendering the next frame before the current frame is copied. The volatile replay index is reloaded across native library calls.

The driving fixture replays 1,694 physics inputs. Final fields should match the expected array in `tests/test_core.c` as little-endian 32-bit words. `render_fps` stays zero, so the unmodified renderer omits its unmeasured FPS HUD. GIFs are sampled by simulation tick at 10 FPS; capture cycle times are not physical-calculator FPS.

The fixture bypasses TI OS/CEaShell startup, save/file I/O, keypad hardware and the display-swap main loop. Do not present its output as a physical-device recording. CEmu is a separate project with its own license; no CEmu or TI ROM binary is included.
