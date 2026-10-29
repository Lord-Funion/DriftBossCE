DRIFT BOSS CE v0.2
==================
Hold 2nd/Enter to drift right. Release to drift left. Turn early: the car slides.

INSTALL
Send the all-in-one .b84 bundle with TI Connect CE, or send every .8xp/.8xv
file in Calculator/. Keep all eight calculator variables archived. Launch
DRIFTB from CEaShell with your existing working native C/ASM setup.

Files: DRIFTB, DBAS0, DBAS1, DBAS2, graphx, fileioc, keypadc, libload.
Allow roughly 214 KiB archive and at least 60 KiB free user RAM.
Confirmed setup: TI-84 Plus CE Python, OS 5.8.3, CEaShell. Reported speed:
about 22 FPS on that calculator. Rendering is capped at 30 FPS; speed varies.

UPDATING FROM v0.1
Replace only DRIFTB.8xp. Keep the DBAS graphics, libraries, and DBSAVE.

CONTROLS
Driving: hold 2nd/Enter for right; release for left. Mode/Clear pauses.
Garage/boosters: Left/Right browse; Alpha selects, buys, or toggles.
2nd/Enter continues. From pause: 2nd/Enter resumes, Mode opens help,
Clear returns to garage. Mode in garage opens help.

SAVE YOUR PROGRESS
Quit FROM THE GARAGE with Clear to save coins, cars, best score, and booster
stock. Keep DBSAVE when updating. Returning to the garage alone does not save.

FEATURES
Eight cars, five themes, ten track-pattern generators in both directions,
ramps/bumps/valleys, narrow/split roads, coins, and three boosters:
Double Score, Car Insurance, Coin Rush. A fresh save includes one use of each.

CONTENTS
Calculator/: ready-to-transfer game, data, and runtime libraries.
Source/: native C project, generated binary banks, and build instructions.
Screenshots/: real native-renderer PNGs and GIFs, plus four compact GIFs for
Cemetech listing screenshots. GIFs use scripted inputs and are exported at
10 FPS; they are not physical-hardware recordings or FPS measurements.
Cemetech/: title, description, suggested screenshot files, and upload guide.
Docs/: installation, build, testing, media provenance, and change history.

TROUBLESHOOTING
Missing data: send all three DBAS files. Missing library: send the four
runtime files. Cannot launch: check your existing C/ASM setup and shell.
The game is not TI-BASIC or Python and does not update your OS.

SOURCE AND RIGHTS
Source: https://github.com/Lord-Funion/DriftBossCE
Original game/artwork: MarketJS; reference: Math Playground Drift Boss.
Unofficial native adaptation/release: Lord-Funion, with Codex assistance.
Fixed-point physics and orthographic rendering differ from the browser.
No audio, advertising, smoke, or daily/random reward screens are included.
Read LICENSE.md and CREDITS.md for the component-specific rights. Original
assets are not covered by a blanket open-source license. The CE runtimes'
BSD-2-Clause notice is included under Licenses/.
