# Validation and performance

## Physical calculator report

The project’s calculator owner confirmed that v0.2 runs on a **TI-84 Plus CE Python, OS 5.8.3, using CEaShell**, reporting approximately **22 FPS**. This is one reported setup, not a guaranteed minimum for all devices/scenes. Save-file I/O was not independently demonstrated by the CPU harness.

## CPU component benchmarks

The existing 48 MHz, rev-M, CPU-only CEmu-core test used actual graphx code in emulated RAM and asset banks in emulated flash:

| Component | v0.1 | v0.2 |
|---|---:|---:|
| 60 physics ticks | 3,725.6 ms | 191.2 ms |
| Warm rendered frame | 114.5 ms | 26.7 ms |

These are isolated component times. They do not include the full TI OS, shell startup, display presentation, or all scene/cache combinations. The v0.2 FPS cap is 30. See [the original benchmark record](validation-v0.2.txt).

## Regression evidence

- Existing rule fixtures cover save validation, purchases, booster use/refunds, pause/help/resume, falling, insurance, and road bounds.
- All ten track-pattern generators were checked in both directions against the reference version.
- The native sprite conversion/clipping checks covered **931 cases** across all 133 sprites, including cache eviction.
- A deterministic **1,694-tick** driving replay finished with **all 19 checked gameplay fields matching** between the host simulation and compiled eZ80 code: coordinates, velocities, shadow/camera positions, score timing, coins, best score, mode, difficulty, serial, and RNG state.
- Fresh publication captures replayed that same input sequence through the unmodified v0.2 game core and renderer; all 19 final fields still matched. The run finishes at score **271**, with **29 coins**, at difficulty **7**.
- A clean rebuild of the published source was compared with the tested release’s TI variable payload.
- Transfer headers, variable names/types, archive flags, bank bytes, and TI checksums were validated. Release ZIP and bundle members were byte-checked and CRC-tested.

`tests/test_core.c` reuses the core rules and replay regression. `tools/verify_release.py` checks the packaged files and sprite-bank bounds. Historical sprite/pattern comparisons are recorded above; the host test is not a substitute for hardware testing.

## What the capture harness does

It loads the compiled eZ80 program into RAM, maps the three graphics banks into flash, and calls the real graphx library. It bypasses TI OS/CEaShell and substitutes the small boot trampoline/initial ClearVRAM behavior required by that environment. A capture/acknowledgment handshake freezes each completed frame while it is copied, avoiding torn screenshots.

The scripted driver is a test fixture, not a bot shipped in the game. GIFs are exported at 10 FPS according to simulation time. The unmodified renderer hides its FPS counter when `render_fps` is zero, as it is in the capture harness. No FPS was invented for the images.
