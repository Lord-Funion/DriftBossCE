# Installation and controls

## Requirements

- A TI-84 Plus CE family calculator with a working **native C/ASM execution setup**.
- **CEaShell** is the launch method tested by the owner. Other shells have not been verified for this release.
- **TI Connect CE** on your computer and a USB cable.
- About **214 KiB archive** for the full set and at least **60 KiB free user RAM**.

The confirmed device is a TI-84 Plus CE Python with OS **5.8.3** and CEaShell. This game does not change the OS or provide an ASM-enabling setup. Consult the [official toolchain setup documentation](https://ce-programming.github.io/toolchain/static/getting-started.html) for native-execution requirements on your OS.

## Choose one transfer method

**Single bundle:** send `DriftBossCE-v0.2-all-in-one.b84` using TI Connect CE. A bundle is one transfer file that installs separate calculator variables.

**Individual files:** send every file below from `Calculator/` in the release ZIP:

| File | Purpose |
|---|---|
| `DRIFTB.8xp` | Native game program |
| `DBAS0.8xv`, `DBAS1.8xv`, `DBAS2.8xv` | Required graphics data |
| `graphx.8xv` | Graphics runtime |
| `fileioc.8xv` | File/save runtime |
| `keypadc.8xv` | Input runtime |
| `libload.8xv` | Runtime loader |

Keep all eight variables archived. Run **DRIFTB** from CEaShell.

## Updating from v0.1

Only `DRIFTB.8xp` changed in v0.2. Replace it and retain your existing DBAS variables, libraries, and **DBSAVE**. You may also transfer the full bundle if you prefer.

## Driving

Hold **2nd** or **Enter** to drift right; release to drift left. Momentum persists after changing direction, so turn before a corner. **Mode** or **Clear** pauses. From pause, **2nd / Enter** resumes, **Mode** opens help, and **Clear** returns to the garage.

## Garage and boosters

Use **Left / Right** to browse. In the garage, **Alpha** selects an owned car or buys the displayed car if you have enough coins. **2nd / Enter** opens the booster selection screen. **Mode** opens help.

In the booster screen, **Alpha** toggles the highlighted booster, using stock first and purchasing a use if needed. **Clear** returns to the garage and refunds selected boosters to stock. **2nd / Enter** starts driving.

A new save starts with one use of each booster. Double Score costs 25 coins, Car Insurance 50, and Coin Rush 75. Car unlock prices are 0, 200, 250, 300, 350, 400, 450, and 500 coins.

## Saving

**Quit from the garage with Clear.** This writes `DBSAVE`, containing coins, best score, owned/selected cars, and booster stock. Returning to the garage alone does not write the save. Keep `DBSAVE` when reinstalling or upgrading.

## Troubleshooting

- **Missing data:** transfer all three DBAS files from this release. They are needed every run.
- **Missing library:** transfer the four supplied runtime libraries.
- **Transfer fails:** check free archive space and archive the large DBAS variables.
- **Cannot launch:** verify your existing C/ASM setup and shell; the game is not a TI-BASIC or Python program.
- **Slow gameplay:** the lower-right counter reports rendered FPS after about half a second. Report the calculator model, OS, shell, selected car, and the scene where the slowdown occurs.

For a reproducible bug, include the screen/action that triggered it, expected behavior, actual behavior, and whether you were using a booster. Do not include your calculator ROM in a bug report.
