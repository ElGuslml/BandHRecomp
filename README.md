# GHBandHRecomp

A static recompilation of **Band Hero** (Xbox 360) to native Windows, built on the
[ReXGlue SDK](https://github.com/rexglue/rexglue-sdk). The game's PowerPC code is translated to C++ and
compiled; the Xbox 360 GPU is emulated by ReXGlue's Xenos backend (derived from [Xenia](https://xenia.jp)).

> [!WARNING]
> Early work in progress: the project has just been set up. There are no releases.

> This project is not affiliated with Microsoft or the game's developer and publisher. It contains no game files
> and no game code: you need your own legally obtained copy of the game.

## Status

- Set up on 2026-11-06 from [recomp-tools](https://github.com/ElGuslml/recomp-tools)' project template, the setup of
  KameoRecomp, GRAWRecomp and POTCAWERecomp.
- `default.xex` (title ID `4156085C`, version 0.0.0.1, XDK 2.0.8276) is recompiled. It boots, and a song plays on both
  renderers, driven by tools: **nobody has played it yet**.
- Band Hero is Guitar Hero 5's code base one step on, so [GH5Recomp](https://github.com/ElGuslml/GH5Recomp)'s fixes were
  ported with every address translated: the threads' waits, the song's clock, the guitar, frame rates above 60,
  ultrawide, the screen effects' switches, the quit prompt, the skinning order, NativeXRenderer (the default renderer,
  with antialiasing and a shadow quality setting).
- [`docs/`](docs/README.md) has a short note on each problem found and what was done about it.

## Requirements

To build:
- Git, [CMake](https://cmake.org/) 3.25 or newer, [Ninja](https://ninja-build.org/),
  [LLVM/Clang](https://github.com/llvm/llvm-project/releases) (MSVC alone does not build this) and the Windows SDK.
- Python 3 (glslang needs it to configure the Vulkan backend).
- Your own copy of the game (see below).

To run:
- Windows 10 or 11, 64-bit, with a GPU that supports Direct3D 12 (or Vulkan).
- A CPU with x86-64-v2 support (SSE4.2 and POPCNT: practically any 64-bit CPU from 2009 on).
- [Microsoft Visual C++ Redistributable 2015–2022 (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe).

## Game files

- **To play:** Band Hero Launcher extracts them from the ISO of your disc on its first run (a full Redump image or a
  trimmed one); it checks that it is this game and this version. `"Band Hero Launcher.exe" --launcher_first_run` installs
  them again.
- **To build:** extract the content of your disc or ISO with
  [extract-xiso](https://github.com/XboxDev/extract-xiso/releases) and copy all of it into `assets/` so that
  `assets/default.xex` exists. The expected executable has CRC32 `57900866`; `GHBandHeroRecomp.exe` warns you at startup if yours
is a different version. See [`assets/README.md`](assets/README.md).

## Building from source

```powershell
git clone --recursive <repository URL> GHBandHRecomp
cd GHBandHRecomp
# copy your game files into assets\ first: the build recompiles assets\default.xex
cmake --preset win-amd64-release
cmake --build --preset win-amd64-release
```

- If you cloned without `--recursive`: `git submodule update --init --recursive`.
- `thirdparty/rexglue-sdk` is the SDK with this project's changes (see below).
- The first build translates `default.xex` into C++ (`generated/`) and builds the SDK: it takes a while.
  If it stops with "precompiled header ... has been modified", run the build command again.

## Running

```powershell
.\out\build\win-amd64-release\"Band Hero Launcher.exe"   # choose the options, then Play; it comes back when you quit
.\out\build\win-amd64-release\GHBandHeroRecomp.exe          # or start the game directly with the saved options
```

- Game files are found in `assets\` next to `GHBandHeroRecomp.exe`, otherwise in the repository's `assets\`.
  Use `--game_data_root=<folder>` to point somewhere else.
- Saves, the profile and the shader cache go to `userdata\` next to `GHBandHeroRecomp.exe` (if that folder cannot be
  written, for example under Program Files, they go to `Documents\ghbandhero`). Logs go to `logs\`.

## Configuration

| File (next to `GHBandHeroRecomp.exe`) | What it holds |
|---|---|
| `ghbandhero.toml` | Base settings. Created with the defaults if it is missing; a default setting the file lacks is appended at startup. |
| `ghbandhero_options.toml` | What Band Hero Launcher saves (monitor, display, window size, render scale, anisotropic filtering, API, keyboard and gamepad controls, and the game's own options). Loaded after `ghbandhero.toml`. |

Command-line options (`--name=value`, for example Steam launch options) win over both files:
`--fullscreen=false --window_width=1920 --window_height=1080`,
`--draw_resolution_scale_x=2 --draw_resolution_scale_y=2`, `--gpu_backend=vulkan`, `--anisotropic_override=0`
(off; 5 = 16×), `--user_language=4` (French).

## How the SDK is modified

`thirdparty/rexglue-sdk` is [ElGuslml/rexglue-sdk](https://github.com/ElGuslml/rexglue-sdk), branch `recomp`: the
official ReXGlue SDK's nightly of 2026-10-02 (v0.10.0 and 24 later commits) with the changes KameoRecomp, GRAWRecomp
(Ghost Recon Advanced Warfighter), POTCAWERecomp (Pirates of the Caribbean: At World's End) and
[NativeXRenderer](native_x_renderer/README.md) need, one commit each. None of the general fixes is specific to one
game, and the options they add are off by default; the ones for NativeXRenderer change nothing while it is off. The
branch's `RECOMP.md` lists them, and each commit message explains its change.

The launcher is [RecompLauncher](https://github.com/ElGuslml/RecompLauncher) (`recomp_launcher/`, a submodule shared by
these recompilations) with this game's profile, texts, colors and art in `launcher/`.

NativeXRenderer (`native_x_renderer/`, a submodule: a native renderer that hooks the game's Direct3D calls) is built
and linked, and draws this game once its side is written (`src/native_x_renderer_ghbandhero.cpp`).

## Credits

- [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk), whose runtime is derived from [Xenia](https://xenia.jp).
- [extract-xiso](https://github.com/XboxDev/extract-xiso), for extracting the game files.
- Developed by Gustavo Garcia with the help of Claude (Anthropic).

## License

MIT for this project's own code (see [`LICENSE`](LICENSE)). The ReXGlue SDK and its components keep their own
licenses (see [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)). Band Hero, its code and its assets belong to their
respective owners.
