# SA Android Free Clothes

> Unlock the full clothing wardrobe in GTA: San Andreas for Android 2.10 — every item owned, everything free, and all seven stores available from any safehouse.

An [Android Mod Loader (AML)](https://github.com/AndroidModLoader/AndroidModLoader) plugin that removes the story restrictions around the clothing system. Browse the complete wardrobe as if you already bought everything in the story, without altering the game's files.

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](./LICENSE)
[![Version](https://img.shields.io/badge/version-1.2-green.svg)](https://github.com/Jean7z/gta-sa-freeclothes/releases)
[![Platform](https://img.shields.io/badge/platform-Android-blueviolet.svg)]()
[![Game](https://img.shields.io/badge/game-GTA%20SA%202.10-lightgrey.svg)]()

---

## Features

- **All clothes unlocked** — every item is treated as already owned, so the wardrobe shows the full catalog instead of only what you bought in-story.
- **Free clothes** — the buy price is forced to `0`, so buying is always free.
- **All seven shops in the wardrobe** — the safehouse wardrobe normally only lists the shop you purchased from during the story (Binco). This mod unlocks all seven clothing shops — Binco, ProLaps, Sub Urban, ZIP, Victim, Didier Sachs, and the Uniform store — including the ones locked behind story progress.
- **Works in any safehouse** — owned or not, anywhere on the map.
- **Real shops keep their own catalog** — entering Binco shows Binco's items, not a merged list. The merge only happens in the safehouse wardrobe.
- **Fully configurable** — every feature can be toggled independently through the AML config file.
- **Clean uninstall** — delete the `.so` and nothing else is touched.

## How it works

The mod hooks the game's `CShopping` class and works with the game's own clothing script — it doesn't replace the wardrobe UI.

| Concern | Approach |
|---|---|
| **Ownership** | `CShopping::HasPlayerBought` is forced to `true`, and the `ms_bHasBought` bitmap is re-marked as fully owned on init and save load, so the wardrobe's "bought" filter lets every item through. |
| **Price** | `CShopping::GetPrice` returns `0`, so buying is free. |
| **Wardrobe contents** | `CShopping::LoadShop("bought")` — the safehouse wardrobe path — is intercepted: all six clothing shop sections are loaded and merged into the wardrobe buffer, and the seven wardrobe rows are unlocked. |
| **Shop rows** | The clothes script (`scriptv1.img`) only adds a shop row to the wardrobe when its global gate is `1` (normally set by story purchases). The mod forges those gates to `1` the moment the safehouse wardrobe opens — before the script builds its rows — so the script adds the SHOP2–SHOP7 rows itself, preserving its native index dispatch. |

> **Note for developers:** the game's own script still renders and dispatches the wardrobe rows. The mod makes the gates and contents right so the script shows everything — it does not replace the wardrobe UI.

## Requirements

- **GTA: San Andreas** for Android **2.10** (Play Store version).
- **[Android Mod Loader (AML)](https://github.com/AndroidModLoader/AndroidModLoader)** installed and working (the game must load `libAML.so`).
- **arm64-v8a** recommended (full feature set). An `armeabi-v7a` build ships for compatibility.

## Installation

1. Download the latest **`.so`** from the [Releases](https://github.com/Jean7z/gta-sa-freeclothes/releases) page, matching your architecture:

   | File | Architecture |
   |---|---|
   | `libAML_PSDK_FreeClothes64.so` | **arm64-v8a** (64-bit, most devices) |
   | `libAML_PSDK_FreeClothes.so` | **armeabi-v7a** (32-bit) |

2. Move it into the game's mods folder:

   ```
   /Android/data/com.rockstargames.gtasa/mods/
   ```

3. Launch the game. The mod loads automatically.

### Uninstall

Delete the `.so` from the mods folder. No other files are modified.

## Configuration

All keys live under the `[Clothes]` section of the mod's AML config file
(`configs/net.psdk.samod.freeclothes.ini`):

| Key | Default | Description |
|-----|---------|-------------|
| `UnlockAll` | `true` | Mark all clothes as owned |
| `FreePrice` | `true` | Force the buy price to `0` |
| `AllShops` | `true` | Merge all shops in the safehouse wardrobe and unlock the seven shop rows |

---

## Building from source

### Prerequisites

- [Android NDK](https://developer.android.com/ndk/downloads) (r21 or newer; r29 recommended)
- `git`

### Build

```bash
# 1. Clone the repository, including the aml-psdk submodule
git clone --recurse-submodules https://github.com/Jean7z/gta-sa-freeclothes.git
cd gta-sa-freeclothes

# 2. Build with the NDK's ndk-build
$ANDROID_NDK_HOME/ndk-build NDK_PROJECT_PATH=. \
  APP_BUILD_SCRIPT=./Android.mk NDK_APPLICATION_MK=./Application.mk
```

The resulting libraries land in `libs/`:

```
libs/arm64-v8a/libAML_PSDK_FreeClothes64.so
libs/armeabi-v7a/libAML_PSDK_FreeClothes.so
```

> Make sure `ANDROID_NDK_HOME` points at the NDK directory containing `ndk-build`, or call the full path to `ndk-build` directly.

### Reusing an existing aml-psdk checkout

If you already have the SDK checked out, you can use it instead of the submodule:

```bash
mv psdk psdk.bak && ln -s /path/to/aml-psdk psdk
```

## Project layout

```
.
├── Android.mk          # ndk-build makefile (selects module name per ABI)
├── Application.mk      # ABI targets and toolchain settings
├── main.cpp            # the entire mod
├── mod/                # AML mod interface helpers (logger/config), vendored
└── psdk/               # aml-psdk submodule (official SA Android SDK headers)
```

The `mod/` helpers and the `psdk/` SDK headers are the same pieces used by the
official AML mods ([`RusJJ/AndroidModLoader`](https://github.com/AndroidModLoader/AndroidModLoader)
+ [`AndroidModLoader/aml-psdk`](https://github.com/AndroidModLoader/aml-psdk), both MIT).

## Compatibility

- **Game:** GTA San Andreas **2.10** for Android.
- **Tested on arm64** alongside the `net.psdk.samod.unlimitedgym` and `net.psdk.samod.ahead` mods (no conflicts).

## Credits

- [RusJJ](https://github.com/AndroidModLoader) — Android Mod Loader, the mod interface helpers (`mod/`), and the SA Android SDK (`aml-psdk`), all MIT.
- [GTA: San Andreas Reverse Engineering](https://github.com/gta-reversed/gta-reversed-android) — reference documentation of the game's engine and scripts.

## License

MIT — see [LICENSE](./LICENSE).

This project is not affiliated with Rockstar Games or Take-Two Interactive.
GTA: San Andreas and its trademarks belong to their respective owners.
Use at your own risk.