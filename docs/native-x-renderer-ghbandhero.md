# NativeXRenderer in Band Hero

The game's side of NativeXRenderer (`native_x_renderer/`, the library that hooks a game's Direct3D calls and draws on
the host). The hooks and the profile are in
[`src/native_x_renderer_ghbandhero.cpp`](../src/native_x_renderer_ghbandhero.cpp); the library's checklist is
`native_x_renderer/docs/integrating-a-game.md`.

**Status (2026-11-06):** GH5's side with the addresses translated, plus FXAA and SMAA, which GH5 lacks. Not the default
renderer: `--native_x_renderer=true` or the launcher's NativeXRenderer card.

## The addresses

- The game links the XDK Direct3D library of 2.0.8276, GH5's, at `0x8230AA90`-`0x8232A8EC`. Each of GH5's 36 hooked
  functions has GH5's instructions here but for branch targets and data addresses (the data addresses of 7 of them
  moved), so the device layout, the objects and every offset of the profile are GH5's.
- The device pointer is at `0x82D9D7F0`: the renderer's block `0x82D9D000` + 2032, as GH5's, by its 1411 uses.

## Antialiasing: FXAA and SMAA

`native_x_renderer_antialiasing`, SMAA by default; the launcher's renderer card. GH5's profile has no scene copy
(`scene_resolve_caller = 0`), GH WoR's does, and this game goes GH WoR's way: every resolve goes through the render
targets' helper `sub_82566170`, and `scene_resolve_caller = 0x8256620C` is the return of its color resolve, at the same
place in the function as GH WoR's. The game alternates two kinds of frame, both shown, which copy the scene from different
places into the same 1280x720 texture, so antialiasing one place only would leave every other frame's edges as they were.
NativeXRenderer takes the copies of the output's size only.

Measured: 300 applications in 600 frames in the menus and 899 in 600 in a song (1.5 a frame, as GH WoR), SMAA and FXAA
alike, the HUD and the highway drawn after.

## Checked (2026-11-06, driven by the tools, nobody playing the notes)

| Run | Observer | Failed draws | Cores |
|---|---|---|---|
| 1720x720 ultrawide, 160 FPS, SMAA, main menu | 476 of 476 | 0 | 1.73 |
| The same, Quickplay song | 557 of 557 | 0 | 1.83 |
| 1280x720, 120 FPS, FXAA, four effects off, a song | not run | 0 | 1.44 |

No render state, fetch, constant or program mismatches. The stage fills the window, the highway and the HUD stay at 16:9 in
the center. The main menu's ring pulses white on both renderers, the same curve.

Not tried: DLSS and FSR 3.1, which need `upscale_resolve_caller`, unset here as in GH5 and GH WoR.
