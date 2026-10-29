# Changelog

## v0.2 — performance update / public package

The calculator build is the same v0.2 program tested by the owner; publication adds documentation, source organization, media, verification tooling, a single-transfer bundle, and the Cemetech package.

- Native 24-bit position math replaces software 32-bit work in the frame loop.
- A nearby collision list avoids repeated full tile-pool scans.
- Original sparse graphics are cached in native RLET form in a 48 KiB arena.
- Faster background fill, fewer projections, and shorter skid history.
- Bounded simulation catch-up and less repeated hardware keypad scanning.
- In-game FPS counter; a 30 FPS rendering cap.
- v0.1 graphics banks and `DBSAVE` remain compatible.

The performance update was prepared October 27, 2026; the documented public package was prepared October 29, 2026. Reported hardware performance: about 22 FPS on one TI-84 Plus CE Python / OS 5.8.3 / CEaShell setup.

## v0.1 — first native adaptation

Eight cars, five themes, ten track patterns in both directions, coins, three boosters, garage, save data, and converted original artwork. Native fixed-point physics and orthographic rendering replace the browser runtime.
