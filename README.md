# SA Android Free Clothes — Unlock All DLC & Wardrobe

A mod for GTA: San Andreas for Android **2.10** that unlocks the full clothing
wardrobe: every item is marked as owned, everything in the safehouse wardrobe
costs nothing, and all seven clothing shops (including the ones normally locked
behind story progress) appear in the wardrobe of any safehouse on the map.

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](./LICENSE)
[![Version](https://img.shields.io/badge/version-1.2-green.svg)](https://github.com/Jean7z/gta-sa-freeclothes/releases)
[![Platform](https://img.shields.io/badge/platform-Android-blueviolet.svg)]()

> A mod for [Android Mod Loader (AML)](https://github.com/AndroidModLoader/AndroidModLoader)
> by RusJJ. The official SA Android plugin SDK
> ([aml-psdk](https://github.com/AndroidModLoader/aml-psdk)) is used and provided
> as a git submodule.

---

## Features

- **Unlock all clothes** — every item is treated as already owned, so the
  wardrobe shows the full catalog instead of only what you bought in-story.
- **Free clothes** — the buy price is forced to `0` in the shops.
- **All shops in the safehouse wardrobe** — normally only the shop you bought
  clothes from in the story shows up there (Binco). The mod unlocks all seven
  clothing shops as wardrobe rows, including shops that are story-locked, and
  it works in **any** safehouse, bought or from the map.
- **Real shops keep their own catalog** — entering Binco shows Binco's items,
  not a merged list; the merge only happens in the wardrobe.
- **Every feature togglable** via the AML config file.
- **Clean uninstall** — just delete the `.so`, nothing else touched.

## How it works

The mod hooks the game's `CShopping` class:

- **Ownership** — `CShopping::HasPlayerBought` is forced to `true`, and the
  `ms_bHasBought` bitmap is re-marked as fully owned on init / save load, so the
  wardrobe's "bought" filter lets every item through.
- **Price** — `CShopping::GetPrice` returns `0`, so buying is free.
- **Wardrobe contents** — `CShopping::LoadShop("bought")` (the safehouse
  wardrobe path) is intercepted: the mod loads all six clothing shop sections,
  merges them into the wardrobe buffer, and unlocks the seven wardrobe rows.
- **SCM shop gates** — the clothes script (`scriptv1.img`) only adds a shop row
  to the wardrobe when its global gate is `1` (set by story purchases). The mod
  forges those gates to `1` when the safehouse wardrobe is opened — *before*
  the script builds its rows — so the script adds the SHOP2–SHOP7 rows itself
  with its native index dispatch.

> Honest note: the game's own script still renders and dispatches the wardrobe
> rows. The mod makes the gates and contents right so the script shows
> everything — it does not replace the wardrobe UI.

## Requirements

- **GTA: San Andreas** for Android **2.10** (play store version).
- **[Android Mod Loader (AML)](https://github.com/AndroidModLoader/AndroidModLoader)**
  installed and working (the game must load `libAML.so`).
- **arm64-v8a** recommended (all features). The `armeabi-v7a` build ships for
  completeness.

## Installation

1. Grab the latest **`.so`** from the [Releases](https://github.com/Jean7z/gta-sa-freeclothes/releases)
   page. Pick the one that matches your architecture:
   - `libAML_PSDK_FreeClothes64.so` → **arm64-v8a** (64-bit, most devices)
   - `libAML_PSDK_FreeClothes.so`   → **armeabi-v7a** (32-bit)
2. Push it into the game's mods folder:
   `/Android/data/com.rockstargames.gtasa/mods/` — or the unprotected path on
   your device.
3. Start the game. The mod is active automatically.

### Uninstall

Delete the `.so` from the mods folder. Nothing else is changed.

## Configuration

All keys live under the `[Clothes]` section of the AML config file:

| Key | Default | Description |
|-----|---------|-------------|
| `UnlockAll` | `true` | Mark all clothes as owned |
| `FreePrice` | `true` | Force the buy price to 0 |
| `AllShops` | `true` | Merge all shops in the safehouse wardrobe + unlock the 7 shop rows |

## Building from source

### Prerequisites

- [Android NDK](https://developer.android.com/ndk/downloads) (r21 or newer; r29 recommended)
- `git`

### Steps

```bash
# 1. Clone the repository including the aml-psdk submodule
git clone --recurse-submodules https://github.com/Jean7z/gta-sa-freeclothes.git
cd gta-sa-freeclothes

# 2. Build with the NDK's ndk-build
$ANDROID_NDK_HOME/ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=./Android.mk NDK_APPLICATION_MK=./Application.mk
```

The resulting libraries land in `libs/`:

```
libs/arm64-v8a/libAML_PSDK_FreeClothes64.so
libs/armeabi-v7a/libAML_PSDK_FreeClothes.so
```

> Environment tip: make sure `ANDROID_NDK_HOME` points at your NDK directory
> (the one containing `ndk-build`), or call the full path to `ndk-build`
> directly.

### Reusing an existing aml-psdk checkout (no submodule needed)

If you already have the SDK checked out somewhere, you can use it instead of
the submodule:

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

The `mod/` helpers and the `psdk/` SDK headers are the same pieces the official
AML mods use (`RusJJ/AndroidModLoader` + `AndroidModLoader/aml-psdk`, both MIT).

## Compatibility

- Game: GTA San Andreas **2.10** for Android.
- Tested on **arm64** alongside the `net.psdk.samod.unlimitedgym` and
  `net.psdk.samod.ahead` mods (no conflicts).

## Credits

- [RusJJ](https://github.com/AndroidModLoader) — Android Mod Loader, the mod
  interface helpers (`mod/`) and the SA Android SDK (`aml-psdk`), all MIT.
- [GTA: San Andreas Reverse Engineering](https://github.com/gta-reversed/gta-reversed-android)
  — reference documentation of the game's engine and scripts.

## License

MIT — see [LICENSE](./LICENSE). This project is not affiliated with Rockstar
Games or Take-Two Interactive. GTA: San Andreas and its trademarks belong to
their respective owners. Use at your own risk.