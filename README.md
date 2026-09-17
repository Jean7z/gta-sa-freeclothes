# SA Android Free Clothes

> [!IMPORTANT]
> Unlock the complete clothing wardrobe in **GTA: San Andreas Android 2.10**.
> Access every clothing item and all seven stores directly from any safehouse.

Your character's full clothing catalog — every shirt, jacket, pant, shoe, hat,
chain, and watch — without playing through the story's shop progression.

[![Version: 1.3](https://img.shields.io/badge/version-1.3-green.svg)](https://github.com/Jean7z/gta-sa-freeclothes/releases)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](./LICENSE)
[![Game: GTA SA 2.10 Android](https://img.shields.io/badge/game-GTA%20SA%202.10%20Android-blueviolet.svg)]()
[![Platform: Android](https://img.shields.io/badge/platform-Android-lightgrey.svg)]()
[![Loader: AML](https://img.shields.io/badge/loader-Android%20Mod%20Loader-orange.svg)](https://github.com/AndroidModLoader/AndroidModLoader)

An [Android Mod Loader (AML)](https://github.com/AndroidModLoader/AndroidModLoader) plugin.
Requires the official SA Android plugin SDK ([aml-psdk](https://github.com/AndroidModLoader/aml-psdk), included as a submodule).

---

## Table of contents

- [Features](#features)
- [How it works](#how-it-works)
- [Installation](#installation)
- [Configuration](#configuration)
- [Building from source](#building-from-source)
- [Project layout](#project-layout)
- [Compatibility](#compatibility)
- [Credits](#credits)
- [License](#license)

---

## Features

### 🎁 Full clothing wardrobe
Every clothing item is treated as already owned, so the wardrobe shows the **complete catalog** instead of only what you bought in-story.

### 🏬 All seven stores
All seven clothing shops — Binco, ProLaps, Sub Urban, ZIP, Victim, Didier Sachs, and the Uniform store — appear in the safehouse wardrobe, **including the story-locked ones**.

### 🏠 Any safehouse
Works in **any** safehouse on the map — owned or not, from the very start.

### 🔍 Original store catalogs
Real stores keep their **own items**: entering Binco shows Binco's catalog, not a merged list. The merge only happens in the safehouse wardrobe.

### ⚙️ Fully configurable
Every feature toggles independently via the AML config file.

### 🧹 Lightweight & reversible
A single `.so` plugin. No game files are modified — **delete the plugin and everything is reverted**.

---

## How it works

The mod hooks the game's `CShopping` class and lets the game's own scripts do the rest:

- **Ownership** — `CShopping::HasPlayerBought` is forced to `true`, and the `ms_bHasBought` bitmap is re-marked as fully owned on init and save load, so the wardrobe's "bought" filter lets every item through.
- **Price** — `CShopping::GetPrice` is forced to return `0` (relevant when `UnlockAll` is off, so the items you do see in real shops cost nothing).
- **Wardrobe contents** — the `"bought"` shop path handled by `CShopping::LoadShop` is intercepted: the six clothing shop sections are merged into the wardrobe buffer **only for the safehouse wardrobe**.
- **Shop rows** — the clothing script (`scriptv1.img`) adds a shop row only when its SCM gate flag is `1` (normally set by story purchases). The mod **enables those gates** so the script adds the SHOP2–SHOP7 rows itself with its native index dispatch: once the moment the safehouse wardrobe opens (via `LoadShop`), and again on startup/save load (via `CShopping::Load`) — a savegame stores the vanilla gate values, so after loading, walking into a safehouse wardrobe would otherwise show just Binco until a shop opened.

> [!NOTE]
> The game's original wardrobe UI is still used throughout. The mod adjusts gates and contents; the script renders and dispatches the wardrobe.

---

## Installation

| Step | What to do |
|---|---|
| **1. Requirements** | GTA SA 2.10 for Android + [AML](https://github.com/AndroidModLoader/AndroidModLoader) installed. |
| **2. Download** | Grab your `.so` from the [Releases](https://github.com/Jean7z/gta-sa-freeclothes/releases) page: <br> `libAML_PSDK_FreeClothes64.so` → **arm64-v8a** (64-bit, most devices) <br> `libAML_PSDK_FreeClothes.so` → **armeabi-v7a** (32-bit) |
| **3. Place the `.so`** | Into the game's mods folder: <br> `/Android/data/com.rockstargames.gtasa/mods/` |
| **4. Launch** | Start the game. The mod loads automatically. |

**Uninstall:** delete the `.so` from the mods folder. Nothing else is changed.

---

## Configuration

Edit `configs/net.psdk.samod.freeclothes.ini` under the `[Clothes]` section:

| Key | Default | Description |
|---|---|---|
| `UnlockAll` | `true` | Show the complete wardrobe |
| `FreePrice` | `true` | Force buy prices to 0 (only matters when `UnlockAll` is off — with everything owned there is nothing left to buy) |
| `AllShops` | `true` | Unlock all seven stores in every safehouse |

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

```text
libs/arm64-v8a/libAML_PSDK_FreeClothes64.so
libs/armeabi-v7a/libAML_PSDK_FreeClothes.so
```

> [!TIP]
> Make sure `ANDROID_NDK_HOME` points at the NDK directory containing `ndk-build`, or call the full path to `ndk-build` directly.

### Reusing an existing aml-psdk checkout

```bash
mv psdk psdk.bak && ln -s /path/to/aml-psdk psdk
```

---

## Project layout

```text
.
├── Android.mk          # ndk-build makefile (selects the module name per ABI)
├── Application.mk      # ABI targets and toolchain settings
├── main.cpp            # the entire mod
├── mod/                # AML mod interface helpers (logger/config), vendored
└── psdk/               # aml-psdk submodule (official SA Android SDK headers)
```

---

## Compatibility

| | |
|---|---|
| **Game** | GTA: San Andreas **2.10** for Android (Play Store version) |
| **Architectures** | **arm64-v8a** (full features), **armeabi-v7a** |
| **Mod loader** | [Android Mod Loader (AML)](https://github.com/AndroidModLoader/AndroidModLoader) |
| **Tested alongside** | `net.psdk.samod.unlimitedgym` and `net.psdk.samod.ahead` (no conflicts) |

---

## Credits

- [RusJJ](https://github.com/AndroidModLoader) — Android Mod Loader, the mod interface helpers (`mod/`), and the SA Android SDK (`aml-psdk`), all MIT.
- [GTA: San Andreas Reverse Engineering](https://github.com/gta-reversed/gta-reversed-android) — reference documentation of the game's engine and scripts.

## License

MIT — see [LICENSE](./LICENSE).

*Not affiliated with Rockstar Games or Take-Two Interactive. GTA: San Andreas and its trademarks belong to their respective owners. Use at your own risk.*