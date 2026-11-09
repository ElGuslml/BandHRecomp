# Building and developing BandHRecomp

BandHRecomp is a static recompilation of **Band Hero** (Xbox 360, 2009) to native Windows, built on the
[ReXGlue SDK](https://github.com/rexglue/rexglue-sdk). The game's PowerPC code is translated to C++ and compiled. It is
drawn by [NativeXRenderer](native-x-renderer-ghbandhero.md) (Vulkan), which draws the game's Direct3D calls on the PC's
GPU; ReXGlue's emulation of the Xbox 360 GPU, Xenos (derived from [Xenia](https://xenia.jp)), is the alternative. Band
Hero is Guitar Hero 5's code base one step on, and [GH5Recomp](https://github.com/ElGuslml/GH5Recomp)'s fixes were
ported to it with their addresses translated ([`fixes-from-gh5.md`](fixes-from-gh5.md)); the other notes in
[`docs/`](README.md) describe each problem found and what was done about it.

## Requirements

- Git, [CMake](https://cmake.org/) 3.25 or newer, [Ninja](https://ninja-build.org/),
  [LLVM/Clang](https://github.com/llvm/llvm-project/releases) (MSVC alone does not build this) and the Windows SDK.
- Python 3 (glslang needs it to configure the Vulkan backend).
- Your own copy of the game: extract the content of your disc or ISO with
  [extract-xiso](https://github.com/XboxDev/extract-xiso/releases) and copy all of it into `assets/`, so that
  `assets/default.xex` exists (title ID `4156085C`, version 0.0.0.1, XDK 2.0.8276). The expected executable has
  CRC32 `57900866`; `BandHeroRecomp.exe` warns at startup if yours is a different version. See
  [`assets/README.md`](../assets/README.md).

## Building

```powershell
git clone --recursive https://github.com/ElGuslml/BandHRecomp
cd BandHRecomp
# copy your game files into assets\ first: the build recompiles assets\default.xex
cmake --preset win-amd64-release
cmake --build --preset win-amd64-release
```

- If you cloned without `--recursive`: `git submodule update --init --recursive`.
- The first build translates `default.xex` into C++ (`generated/`) and builds the SDK: it takes a while. If it stops
  with "precompiled header ... has been modified", run the build command again.
- Presets: `win-amd64-release`, `win-amd64-relwithdebinfo`, `win-amd64-debug`.

## Running

```powershell
.\out\build\win-amd64-release\"Band Hero Launcher.exe"   # choose the options, then Play
.\out\build\win-amd64-release\BandHeroRecomp.exe       # or start the game directly with the saved options
```

- Game files are found in `assets\` next to `BandHeroRecomp.exe`, otherwise in the repository's `assets\`.
  `--game_data_root=<folder>` points somewhere else. `"Band Hero Launcher.exe" --launcher_first_run` installs them
  again from an ISO.
- The saves, the achievements and the playtime go to the current profile of the launchers' shared store
  (`%APPDATA%\ElGuslml Recomps`); the shader cache to `userdata\` next to `BandHeroRecomp.exe` (if that folder
  cannot be written, for example under Program Files, to `Documents\ghbandhero`). Logs go to `logs\`.

## Configuration

| File (next to `BandHeroRecomp.exe`) | What it holds |
|---|---|
| `ghbandhero.toml` | Base settings. Created with the defaults if it is missing; a default setting the file lacks is appended at startup. |
| `ghbandhero_options.toml` | What Band Hero Launcher saves. Loaded after `ghbandhero.toml`. |

Command-line options (`--name=value`, for example Steam launch options) win over both files, for example
`--fullscreen=false --window_width=1920 --window_height=1080`, `--native_x_renderer=false` (the Xenos path),
`--ghbandhero_frame_rate=120` (60, 120, 144, 160, 165 or 240), `--ghbandhero_ultrawide=true`
([frame rates above 60 and ultrawide](fixes-from-gh5.md#what-is-gh5s-with-this-games-addresses)),
`--anisotropic_override=0` (off; 5 = 16×), `--user_language=4` (French;
[language](fixes-from-gh5.md#low-latency-audio-and-language)), `--quit_prompt=false` (B in the main menu does not ask
to quit).

## The launcher

Band Hero Launcher is [RecompLauncher](https://github.com/ElGuslml/RecompLauncher) (`recomp_launcher/`, a submodule: the
launcher of every one of these recompilations, SDL3 + Dear ImGui) with this game's profile in `launcher/`:
`launcher/game.h` (names, the disc, the look, the options), `launcher/game.cpp` (the game's own texts),
`launcher/palette.h` (its colors) and `launcher/art/`, and its resources in `res/launcher.rc`.
`installer/ghbandhero.iss` is its Inno Setup installer. The icon is `res/icon.svg` and `res/icon_small.svg`;
`res/make_icon.mjs` builds `res/icon.ico` from them, and `installer/make_images.ps1` the installer's images.

## How the SDK is modified

`thirdparty/rexglue-sdk` is [ElGuslml/rexglue-sdk](https://github.com/ElGuslml/rexglue-sdk), branch `recomp`: the
official ReXGlue SDK's nightly of 2026-10-02 (v0.10.0 and 24 later commits) with the changes this project, GH5Recomp,
GHWoRRecomp, GHVanHRecomp, GHMetalRecomp, KameoRecomp, GRAWRecomp (Ghost Recon Advanced Warfighter), POTCAWERecomp
(Pirates of the Caribbean: At World's End) and [NativeXRenderer](https://github.com/ElGuslml/NativeXRenderer) need, one
commit each. None of the general fixes is specific to one game, and the options they add are off by default (this game
sets the ones it needs in `src/startup.cpp`); the ones for NativeXRenderer change nothing while it is off. The branch's
`RECOMP.md` lists them, and each commit message explains its change.

`native_x_renderer/` is NativeXRenderer, a submodule: a native renderer that hooks the game's Direct3D library. This
game's side of it (its hooks and the map of the game's Direct3D library) is in
`src/native_x_renderer_ghbandhero.cpp` and [`native-x-renderer-ghbandhero.md`](native-x-renderer-ghbandhero.md).
