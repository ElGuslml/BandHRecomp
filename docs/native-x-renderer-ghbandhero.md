# NativeXRenderer in Band Hero

The game's side of NativeXRenderer (`native_x_renderer/`, the library that hooks a game's Direct3D calls and draws on
the host). The hooks and the profile are in
[`src/native_x_renderer_ghbandhero.cpp`](../src/native_x_renderer_ghbandhero.cpp); the library's checklist is
`native_x_renderer/docs/integrating-a-game.md`.

**Status (2026-11-08):** GH5's side with the addresses translated: FXAA and SMAA on the scene's copy, the shadow quality
row. The default renderer, as in GH5 and GH Warriors of Rock (`kNativeXRendererDefault`: a first run draws with
NativeXRenderer and SMAA); the launcher's Xenos card or `--native_x_renderer=false` turns it off.

## The addresses

- The game links the XDK Direct3D library of 2.0.8276, GH5's, at `0x8230AA90`-`0x8232A8EC`. Each of GH5's 36 hooked
  functions has GH5's instructions here but for branch targets and data addresses (the data addresses of 7 of them
  moved), so the device layout, the objects and every offset of the profile are GH5's.
- The device pointer is at `0x82D9D7F0`: the renderer's block `0x82D9D000` + 2032, as GH5's, by its 1411 uses.

## Antialiasing: FXAA and SMAA

`native_x_renderer_antialiasing`, SMAA by default; the launcher's renderer card. GH5's choice (its `575136e`, with the
addresses translated): `scene_resolve_caller = 0x8256620C`, `scene_resolve_outer_caller = 0x8256694C`,
`scene_resolve_once = true`. The engine draws its scene over two frames and resolves through the render targets' helpers
(`sub_82566918` -> `sub_82566170`):

| Frame | 1280x720 copies, in order |
|---|---|
| Stage and crowd | the scene without the band, from `0x82566A78`; shows the last frame's image with the 2D layer over it |
| Band and composite | **the whole scene, from `0x8256694C` (the first)**; the composite's first pass, the same callers and texture; the finished image, `0x8255F17C < 0x825659CC`, read only by the next frame |

The first copy of the second kind is antialiased, once: before the bloom and the depth of field, so they read the smoothed
scene, and both frames show it. The HUD and the note highway are drawn after it and stay sharp. Until 2026-11-08 this
profile went GH WoR's way, every 1280x720 copy through the helper (`scene_resolve_caller` alone): 899 times in 600 frames
in a song, so the stage was smoothed twice and the composite's pass once more. Now 300 times in 600 frames in Party Play
at 160 FPS, once for each pair of frames, as GH5. The menus were not measured here (GH5's were, the same functions).

## Checked (2026-11-06, driven by the tools, nobody playing the notes)

| Run | Observer | Failed draws | Cores |
|---|---|---|---|
| 1720x720 ultrawide, 160 FPS, SMAA, main menu | 476 of 476 | 0 | 1.73 |
| The same, Quickplay song | 557 of 557 | 0 | 1.83 |
| 1280x720, 120 FPS, FXAA, four effects off, a song | not run | 0 | 1.44 |

No render state, fetch, constant or program mismatches. The stage fills the window, the highway and the HUD stay at 16:9 in
the center. The main menu's ring pulses white on both renderers, the same curve.

Not tried: DLSS and FSR 3.1, which need `upscale_resolve_caller`, unset here as in GH5 and GH WoR.

2026-11-08, Party Play at 160 FPS (no keys), 1280x720 and 2400x1350, SMAA, shadows at 4x: 0 errors, 156-157 FPS, no
frame without a scene in 12,367; on Xenos at 60 FPS, 0 errors.

## Shadow quality: the filter follows the map

The launcher's **Shadow quality** row (Original, High, Ultra; NativeXRenderer's D92), as in GH5 (`cdec72f`). With
`native_x_renderer_shadow_scale = 4` the log names one shadow map in Party Play, GH5's: the 400x400 depth surface at EDRAM
base 1709, 1600x1600 on the host. Nothing else is taken for one (GH Warriors of Rock's engine also has a 640x360 depth pass
there, which keeps that game's row off).

The characters' shadows are read in 4, 9 or 16 samples (the count at `0x82B27048`), each compared with the pixel's depth,
their offsets one texel of the 400x400 map apart (`sub_825C8558`, GH5's `sub_825CCE98` grown by 295 instructions, the same
switch on the count). A larger map kept those offsets, and in GH5 its samples fell four texels apart: a lattice of
diamonds on skin. `GHBandHeroShadowFilterHook` (`0x825C8758`, after the texel size is loaded into `f31` and `f0`) divides
them by the shadow scale. Seen at 2400x1350 with the hook off: GH5's stepped edges on a singer's arm. With it on, the
shadows' edges are soft (Party Play picks its song and band anew each run, so not the same moment).

**Off:** `ghbandhero_shadow_filter_fix = false` in `ghbandhero.toml`; the row's Original draws the map at the console's
size.

## Stalls on a first play (NativeXRenderer D126)

A graphics pipeline new to the driver can take most of a second to compile, and D101 made draws into offscreen targets
wait for theirs: in GH5 the first play of a song stopped for up to 787 ms. D126 skips instead of waiting when the target
has been drawn into on more than 16 frames in a row (it keeps its last frame and mends itself); other offscreen draws still
wait (GH5's `docs/first-play-stalls.md`). **Off:** `native_x_renderer_skip_steady_passes = false` in `ghbandhero.toml`.

## The blurs' lattice (NativeXRenderer D127, 2026-11-08)

In GH Metallica (a player's issue there, its `docs/bloom-lattice.md`) and then in GH Warriors of Rock, this engine,
above 720 lines the screen effects' blurred copies of the scene showed a grid of fine lines and doubled edges: their blurs
step by constants for 320x180 texels, so at the render scale the taps skip host texels. In GH WoR it was the background
its depth of field blurs, at 3440x1440 in the rocker creator's character screen. NativeXRenderer's
`unscaled_screen_fractions` keeps those reduced copies of the screen (640x360, 320x180, 160x90) at the game's resolution,
as on the console; it is set in this profile as in GH5 and GH WoR (the submodule moved from `bf965e9` to `655cd0b` for
it). The bloom, the depth of field and the other screen effects' passes are at the console's resolution; the scene, the
characters and the HUD stay at the output's.

**Off:** `native_x_renderer_unscaled_screen_fractions = false` in `ghbandhero.toml`.

## The render scale in the shaders' constants (NativeXRenderer D128, 2026-11-08)

With the SDK's recomp change 73, the translated shaders read the render scale from the draw's system constants instead
of having it written into them. Before, a window size, internal resolution or FSR 1 / NIS setting not used before gave
the driver new shaders to compile, and its pipeline cache kept a copy of each pipeline per scale; now one translation
serves every scale. The first start after the change compiles every pipeline once. A draw computes what it computed
(NativeXRenderer's decisions.md D128). **Off:** `--native_x_renderer_scale_in_constants=false` writes the scale into the
shaders as before.
