# The fixes and options from GH Warriors of Rock (2026-11-18)

**In Band Hero (2026-11-24): ported from GH5Recomp's note of the same name, built, not run yet.** Every address was
translated through GH5's and Band Hero's instruction indexes and each hook site compared with GH5's; where Band Hero
differs it says so (the players' info: +116 and +240 here, +108 and +232 in GH5; the note highway's picture,
`DD482D15759B682F`). The statuses and checks below are GH5's runs, not this game's.

**Status: built and checked with the keyboard, a virtual controller and screenshots (Party Play, 160 FPS, both
renderers: 0 errors); not played with a guitar yet.** GH Warriors of Rock (GHWoRRecomp) runs on Band Hero's engine. Its
work of 2026-11-14 to 18 was brought here without its online play, DLSS / FSR 3.1, the resolutions below 720 lines,
the ambient occlusion and the texture detail (those need the game's upscale point, left for later) and its crowd
switch. Each hook was found again in Band Hero's code through the two games' instruction indexes and the scripts' table of C
functions, then compared instruction by instruction; where Band Hero differs, the section says how. The details of each
option are GH Warriors of Rock's notes (its `docs/`).

## The submodules

SDK `6e310b4` (changes 104-122), NativeXRenderer `c1b9b80` (D143-D180; D173 keeps the floor's shadows above the
Original shadow quality, the same surface here), RecompLauncher 3.31.1. NativeXRenderer's record thread is on by
default (`native_x_renderer_record_thread`, D154 / D155; D172 with V-Sync). **Off:** `native_x_renderer_record_thread =
false` in `ghbandhero.toml`.

## Frame rates above 240

300, 360, 480 and Unlimited (1000 Hz) with V-Sync off, experimental; with V-Sync on a rate above the monitor's runs
at the monitor's (`src/options.cpp`). The presses kept for scripts held to 60 a second and the whammy's samples hold a
60th of a second at 1000 FPS (`src/framerate.cpp`). Party Play at 480: about 410 FPS.

## Star Power by Select above 60 FPS

GH WoR's pull request #5 (Panchovix). The script function `check_for_star_power_CFunc` (`sub_822390E8` here) reads
the one-frame pressed bits of Select (and of the D-pad on a controller) outside ControllerMake, so above 60 FPS a press
in a frame its held script skipped was lost. It is given those presses as ControllerMake is (`src/framerate.cpp`,
`GHBandHeroStarPowerPollHook` at its entry, the end hook at its two exits). Not tried with a guitar yet.

## The venue at the full frame rate

**Option, off by default:** Settings → Display → "Venue at the full frame rate", or `ghbandhero_scene_every_frame = true`.
The engine draws its 3D scene over two frames (the split render mode), so the stage moves at half the frame rate.
`src/split_render.cpp` writes the mode 0 in both copies of the render state each frame (`0x82DA7DC8` and
`0x82D9DFB8`, +3928 of Band Hero's render state; GH WoR's +1896). Checked: "the game's mode was 1", SMAA on every frame
(600 in 600), 0 failed draws. About twice the 3D work a frame (GH WoR's `docs/scene-every-frame.md`).

## D3D's fence stamps

`src/fence_stamps.cpp` (`ghbandhero_fence_stamps_unwatched`, on): SetStreamSource's and SetIndices' fence stamps
(`0x8230FB18`, `0x8230FC5C`) written through the physical view, so the renderers' page watches do not see them
(GH WoR's `docs/fence-stamps.md`). Party Play at 160: NativeXRenderer's buffer copies per draw about halved (0.45
against 0.87; different venues on screen). **Off:** `ghbandhero_fence_stamps_unwatched = false`.

## Drum kits on five lanes

`src/drums.cpp` (`ghbandhero_drum_kit_five_lanes`, on): the game gives five lanes only to a kit that holds the left stick's
click (a Guitar Hero kit's sign), so the keyboard, a Rock Band kit or a MIDI adapter played four whatever the
launcher said. A kit without the launcher's four-lane mark ("8:1") is now five-lane at its first read (`0x8248A078`;
Band Hero's controller port keeps the mark at +216 and the reads counter at +244). Checked with the keyboard as a drum kit:
"[drums] slot 0: a drum kit on five lanes". **Off:** `ghbandhero_drum_kit_five_lanes = false`.

## A controller as in Guitar Hero III

The default **Play style** of a controller (`gamepad_guitar_layout = "gh3"`, SDK changes 112 and 113): in a song the
frets on LT, LB, RB, RT and A, the open note on B, no strum, Star Power on the D-pad or Back, the whammy bar on either
stick; its buttons in a song are its own (the launcher's "In a song"). Band Hero kept Guitar Hero III's controller code
(`sub_821E15B8`, chosen when `sub_821D3720` says "not a guitar"); `src/controller_layout.cpp`'s 12 hooks make the
gameplay take such a controller for what it is, and three more let a chord over an extended sustain count (GH WoR's
`docs/controller-gh3-layout.md`). Checked with a virtual Xbox 360 controller: joined Party Play on guitar, LT+RB lit
green and yellow, A orange. **Off:** "Guitar · Xbox 360" in the controller's page, or `gamepad_guitar_layout =
"guitar"`.

## Xbox One guitars

SDK changes 110 and 114: an Xbox One guitar (PDP Riffmaster) is read through Microsoft GameInput's API 3, its tilt
included. The setup offers GameInput when the release ships `GameInputRedist.msi` (`Package-Release -WithGameInput`;
the build copies it when configured with `REXGLUE_GAMEINPUT_REDIST_MSI`), and so does the launcher's Controls tab
(GH WoR's `docs/xbox-one-guitars.md`). Not tried with such a guitar. **Off:** `gamepad_gameinput = false`.

## World Tour's Star Power sound

**Option, off by default:** Mods → Sound → "World Tour's Star Power sound", or `ghbandhero_star_power_reverb = true`
(Panchovix's, GH WoR's pull request #8). Band Hero keeps World Tour's echo, reverb and high pass structs but, as Warriors of
Rock, only raises the venue's reverb on Star Power; `src/star_power_reverb.cpp` builds World Tour's stack on the free
send slot 2 at the boot and sends the player's tracks there. Band Hero's tracks are 112 bytes with 24-byte sends, and its
send code also clears a byte through the same wrong slot as the volume it reads: the fix at `0x82168BD4` makes both the
slot's own. Checked: the stack made at the boot (5 of 64 effects taken). Not heard in a song yet.

## The mods: the note highway, two sounds and the speakers

Mods → Highway (`src/mods.cpp`, `src/script_globals.cpp`, `src/highway_images.cpp`, `src/highway_effects.cpp`): the
highway's scale with one player, its length, perspective, style (dark) and opacity, pictures from a gallery, and the
Star Power, band moment and revival effects at the highway's scale. Band Hero's highway values are GH WoR's to the digit;
the global table is at `0x82D78F44`. Band Hero's highway texture is `Highway_Judy_fm_01`, hash `DD482D15759B682F` (for
`textures\` and the gallery), in `global.pak.xen` (the launcher's preview reads it there, RecompLauncher 3.31.1).
Mods → Sound: the whammy's bend (`0x821E94D8`) and the instrument's volume on a miss (`sub_82162F10`; not in the
single-stream songs, which take another path). Settings → Audio → Channels with 5.1: the game is told its speakers
(`audio_speaker_config_from_output`), and `ghbandhero_dolby_timing` keeps the Dolby Digital timing (-30 ms) whatever it is
told. Checked in Party Play: scale 85, length 150, perspective 75, dark, opacity 60 on screen; the texture log's hash;
the lag settings patched. Not checked: a miss's volume, the whammy's sound, the effects at another scale, a picture in
a song.

**Not ported, the crowd switch:** GH WoR hides its crowd with its own soundcheck cheat, which Band Hero does not have. Read
`disable_crowd` as 1 in `create_crowd_models` (practice mode's state) and the venue's billboards show their
placeholder atlas (coloured rectangles). Hiding each ped (`hide_crowd_models`) is left to try (`src/mods.cpp`).

## GH Warriors of Rock's performance plan, Phases 0-2 (2026-11-23)

**Status: built; Test-Games on both renderers (2026-11-24, `runs/2026-11-24_115142`): the first screen at 160 FPS,
0 errors, the observer 361/361, the D181 and D182 lines in the log; not checked in a song yet.** What GH WoR's plan for weaker PCs changed in its own code and settings (its
`docs/development.md`, `docs/native-x-renderer-ghwor.md`), with NativeXRenderer `70800c6` (D181, D182):
- **`skip_msr`** (`ghbandhero_functions.toml`): the XDK's inline Interlocked operations run as the host's atomics, without
  the SDK's process-wide lock (GH WoR: -7 to -8 % of the process's cores on four cores, its job workers a fifth of
  their time in that lock before). recomp-tools' `tools/codegen/msr_sites.py` checks every site by address: 224 sites
  in 129 functions here. Its first run failed on two sites in `sub_82726320` followed by `mftb r9`, an instruction it
  did not decode and so read as reading `r8`, mfmsr's register; decoded as the write it is, every site passes (the
  tool's fix is recomp-tools', not committed yet).
- **A song's frames counted by the vertical blanks each took** (`hooks/perf_measure.toml`, `src/framerate.cpp`): the
  log's `[frames]` lines, a line every 3,000 of a song's frames and one when its music stops (`SongStop`,
  `SongUnload`, `SongAudioPlayerDeInit`). With NativeXRenderer the summary line also counts the song's frames apart
  (D181, `native_x_renderer::SetMainActivity`).
- **V-Sync in a window** (`src/startup.cpp`): `native_x_renderer_window_present_mode = "mailbox"` (D182), as GH WoR.
- **72, 75, 90 and 100 FPS** in the launcher's frame rate row (`launcher/game.h`).

**Not ported:** GH WoR's lighting job fix (`ghwor_lighting_job_wait`). Band Hero has no such job: its stage 9
(`sub_825D2718`) waits for no job group and no code queues group 365.

## GH Warriors of Rock's work of 2026-11-18 to 24 (2026-11-24)

**Status: built, not run yet.** Submodules: SDK `53665a9` (changes 123-125), NativeXRenderer `e17a68a` (D182's texts
only), RecompLauncher 3.33.1 (GH Warriors of Rock's 3.33.0 with Band Hero's 3.31.1, the highway preview's second pak, merged
in).
- **The notes and the music in step** ([song-sync.md](song-sync.md)): the song's clocks by the vertical blanks' own
  times (`ghbandhero_song_clock_exact`), a song that follows another started with its music (`ghbandhero_song_start_sync`), the
  SDK's two clock and audio fixes. `hooks/song_sync.toml`, `src/song_clock.cpp`, `src/song_sync.cpp`; Band Hero has a second
  song clock (`sub_823EBF10`), which both fixes cover.
- **Notes at the console's speed** with a highway perspective other than 100 % (Mods → Highway, under the
  perspective; `ghbandhero_highway_keep_speed`, off): the times on the highway multiplied by the perspective's speed at the
  strike line (0.784 at 75 %), so the Hyperspeed keeps its feel. The perspective's help no longer says the notes keep
  their speed there (they do not).
- **The keyboard on Guitar Hero III's layout** (SDK change 125, `keyboard_guitar_layout = "gh3"`, the launcher's
  Keyboard page, Play style): in a song the fret keys play without a strum through the same hooks as a controller
  (the open note on 3, Star Power on Space, the whammy bar on the right arrow). **Off:** Play style Guitar · Xbox 360.
- **Two more stamps written unwatched** (the performance plan's Phase 3; `hooks/draw_stamps.toml`,
  `ghbandhero_draw_stamps_unwatched`, on): a render object's last drawn frame (`0x8263C3C8`, object + 128; GH WoR's + 112)
  and SetTexture's fence (`0x82314954`). The render objects share pages with textures, so each stamp faulted on a
  watched texture page; in GH WoR texture loads went from 0.5-1 a frame to none.

## Still to port (2026-11-25, after 0.2.0)

GH Warriors of Rock 1.0.0 has these and Band Hero 0.2.0 does not (Gustavo, 2026-11-25: 0.2.0 without them, noted for
later; GH5 0.2.1 is in the same place). All four need Band Hero's upscale point (WoR's `0x82641558` after the bloom
and the depth of field, D119's point before the HUD), found in Band Hero's code and given to NativeXRenderer as WoR's
`src/native_x_renderer_ghwor.cpp` does:
- **NVIDIA DLSS / DLAA and AMD FSR 3.1** (D119, D156-D161, D164): with the split render's pauses (D159) and the reset
  at camera cuts (WoR's `src/camera_cuts.cpp`, D158), `kHasDlaa` / `kHasDlssModel` in `launcher/game.h`, and
  `Package-Release -WithDlss -WithFsr`.
- **Resolutions below 720 lines** (D174, D175): `native_x_renderer_scene_scale`, `game::kLowResolutions`.
- **Ambient occlusion** (D178, XeGTAO, with the camera from the view's first draw) and **texture detail** (D179): the
  game's own AO left out while ours is on (WoR's flag 0x800).
Also not here: online play (WoR's `docs/online-play.md`) and WoR's crowd switch.
