# NEO-Duo

NEO-Duo is a Neo Geo emulator for Android. It is a fork of [NEO.emu](https://github.com/Rakashazi/emu-ex-plus-alpha) (emu-ex-plus-alpha, commit `1c12fac`) rebuilt so **one person can control both Player 1 and Player 2 from a single touchscreen**.

The app id is `com.sevesghost.neoduo` and the launcher name is **NEO-Duo**, so it installs next to the original NEO.emu package.

## What you can do

On a fresh install the on-screen controls start in **two-player** mode, using the **Joint** layout:

- One set of buttons (D-pad, A–D, turbo A–D, A+B, C+D, A+B+C, and the turbo combos) sends each press to **both** players.
- Small per-player Coin and Start buttons sit in the top corners. Joint mode also has a white shared Select and Start at the top center.
- **Separate** mode puts a blue Player 1 set on the left and a mirrored red Player 2 set on the right. An optional row of white buttons at the bottom center can drive both players at once (off by default) and stays at least 12 mm clear of the player buttons.
- A Mode button at the top of the screen switches Joint and Separate when it is held for half a second. While two-player mode is on, the old single-player Virtual Gamepad Player option is greyed out.
- Turning **One Person, Both Players** off restores the original single-player touch layout.
- Each set has its own size and opacity, including the shared buttons. Player 1 and Player 2 start at 50% opacity and the shared row at 40%. Lock P1/P2 Size keeps the two face sizes equal. Changing a size leaves dragged positions where they are. Buttons never shrink below 9 mm; a tight screen packs the gaps instead. **Reset Emulator Button Positions** puts the current mode back on its default anchors.
- Layouts are stored in millimetres from the screen edges under config key `CFGKEY_VCONTROLLER_DEVICE_BUTTONS_V3` (`configId` 0 = separate, 1 = joint), with a separate landscape and portrait arrangement. The turbo row stays inside the bottom 40% of the screen. Wider than 16:10 keeps the controls over the picture. 16:10 or narrower shrinks the game image and leaves a strip along the bottom.
- The game picture can be dragged anywhere on the screen from **Set Video Position**, with Smaller/Larger controls and a reset, or positioned from Video settings (horizontal offset, vertical offset, and placement scale). Landscape and portrait remember their own offsets and scale. The touch pads stay drawn above the picture.
- Launching the app mixes with other audio (YouTube, music) instead of taking exclusive audio focus. That default is not switched back on later.

BIOS and game ROMs are **not** included. You supply your own legally obtained files.

## Save states

The 68000 core is now [Musashi 4.x](https://github.com/kstenerud/Musashi) (MIT) on every ABI, including 32-bit ARM. The Z80 core is [MAME's current Z80](https://github.com/mamedev/mame/tree/master/src/devices/cpu/z80) (BSD-3-Clause), also on every ABI. The old non-commercial Musashi 3.32, Portable Z80 3.9, and GPL-2-only Cyclone cores are gone.

Because of that, **save states written by the previous cores do not load**. The state header uses new core flags (`m68k` 0x6, `z80` 0x9) and rejects older files.

## Credits and licenses

NEO-Duo as a whole is **GPL-3.0-or-later**, the same license as Imagine, EmuFramework, and NEO.emu. See `COPYING.GPL`.

Upstream:

- Imagine and EmuFramework, and the NEO.emu application shell — © Robert Broglia, GPL-3.0-or-later. <https://github.com/Rakashazi/emu-ex-plus-alpha>
- Gngeo — © the Gngeo team, GPL-2.0-or-later. The Neo Geo machine code under `NEO.emu/src/gngeo` (including `video_arm.s`) comes from that project.
- 68000 — Musashi © Karl Stenerud, MIT. Sources are in `NEO.emu/src/musashi`.
- Z80 — MAME's ZiLOG Z80 core, BSD-3-Clause, © Juergen Buchmueller and the MAME team. Sources and the generated `z80.hxx` are in `NEO.emu/src/z80`. Regenerate the execution loop with `python3 z80make.py z80.lst z80.hxx` from that directory if you update `z80.lst`.
- YM2610 (`ym2610.c`) is GPL-2.0-or-later, © Jarek Burczynski and Tatsuyuki Satoh, from MAME's `fm.cpp` and `ymdeltat.cpp`. The interface in `2610intf.c` is BSD-3-Clause © Ernesto Corvi plus the Gngeo GPL-2.0-or-later integration. Neo Geo protection (`neocrypt.c`) stays under MAME's BSD-3-Clause terms. None of these were rewritten onto ymfm.
- The calendar chip in `pd4990a.c` is an original implementation. Its shift register and strobe follow MAME's BSD-3-Clause `upd1990a` device (© Curt Coder, hap, and the MAME team). The old non-commercial MAME `pd4990a` sources are not in this tree.
- The image shaders in `EmuFramework/res/shaders` (direct, scale, and the edge-directed smoother, including the fallback variants) are original GPL-3.0-or-later code. The previous unlicensed hq2x and Scale2x shader text was replaced.
- EmuFramework no longer builds or ships the unused Mednafen `mednafen_common` / `src/shared` sources.
- `memcpy.S` is the NetBSD routine (BSD-style). `crypt.h` is zlib's zip code.

No BIOS, ROMs, keystores, or signing passwords are in this repository.

## Building a signed Android APK

Host tools used for the release build: JDK 21, Android SDK platform 36, Android NDK r29, and the NDK's Clang (21+). CMake is fetched by the project scripts.

```bash
export ANDROID_SDK_ROOT="$HOME/android-sdk"   # must contain platforms/android-36 and ndk/29.*
export ANDROID_HOME="$ANDROID_SDK_ROOT"

# Imagine's third-party libraries (all ABIs the meta build lists: armv7, arm64, x86, x86_64)
( cd imagine/bundle/all && ./makeAll-android.sh install )

# Point the makefiles at this tree
./imagine/android.sh config
./imagine/android.sh installLinks --config Release
./EmuFramework/android.sh config
./EmuFramework/android.sh installLinks --config Release

# Release signing. Keep the keystore and passwords outside the repo.
# ~/.gradle/gradle.properties (mode 600) — do not commit this file:
#   ANDROID_KEY_STORE=/path/to/neoduo-release.keystore
#   ANDROID_KEY_STORE_PASSWORD=...
#   ANDROID_KEY_ALIAS=neoduo
#   ANDROID_KEY_PASSWORD=...

cd NEO.emu
make -f android.mk android-apk CONFIG=Release
```

The signed universal APK is written under `NEO.emu/build/android-release/`. It contains `arm64-v8a`, `armeabi-v7a`, `x86_64`, and `x86`.
