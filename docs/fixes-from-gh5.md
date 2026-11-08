# The fixes ported from Guitar Hero 5

**Status: in testing (2026-11-06; GH5's later fixes ported 2026-11-08).** Band Hero is Guitar Hero 5's code base one step on: its executable's original
name is `gh5_1_burn_Final.exe` (GH5's `gh5_burn_Final.exe`), built with the same XDK (2.0.8276), and 99.3 % of its
functions have the same instructions as GH5's but for branch targets and data addresses (Warriors of Rock 30.9 %, GH
Van Halen 25.4 %). Its first boot had GH5's problems (5.35 cores busy at the first screen, among others), so GH5Recomp's
fixes were copied with their names changed and every address translated. Warriors of Rock's two additions to them are
in too: below.

Each hook was found with a function matcher over both executables' instruction indexes and checked against GH5's: the
same instruction, the six around it each side and the same registers. Each of the 174 functions the patches name was
compared with structure offsets on, because some of the game's structures grew (the renderer's block by 32 to 48 bytes
from about +12600, a player's input object by 8 from +128): none of the offsets `src/` reads moved.

Checked here (2026-11-06, by the tools, not yet by a player): the first screen, the main menu, the quit prompt, the band
lobby, Quickplay and a song on both renderers, the CPU, the timer fix, ultrawide at 1720x720, 120 and 160 FPS, four
screen effects turned off. NativeXRenderer: [native-x-renderer-ghbandhero.md](native-x-renderer-ghbandhero.md).

## What is GH5's, with this game's addresses

- **The threads' waits** (`src/thread_waits.cpp`): **5.35 -> 0.80 cores** at the first screen, 0.87 in the main menu,
  0.88 in a song. **Off:** `ghbandhero_wait_smart = false` (slices only), `ghbandhero_wait_spin_us` /
  `ghbandhero_wait_sleep_us`.
- **The registers as local variables**, with setjmp `sub_829117A0` and longjmp `sub_82911480`, used only by libjpeg and
  libpng; non-volatile vector registers stay in the context. `uninit_cfg.py`: 173 pairs, as GH5's.
- **The song's clock** (`src/timer.cpp`, `ghbandhero_timer_fix`): the engine's timer keeps the frequency it is told, so
  the notes do not drift from the audio.
- **The audio** (`ghbandhero.toml`): `audio_maxqframes = 4`, `audio_callback_min_interval_us = 3000` (FMOD mixes on its
  own thread here, as in GH5 and GH WoR), `audio_keep_time`, `audio_thread_priority`; the launcher's Low latency audio.
  GH5's `src/fmod_mix.cpp` comes along, off as there (`ghbandhero_fmod_on_audio_thread`).
- **The guitar and the keyboard**: a gamepad reported as a guitar, the whammy bar at rest, the keyboard as a guitar
  (frets D F J K L, strum the arrows, Star Power Space, Start Enter), PS3 and CRKD guitars (`guitar_ps3_protocol`).
- **The first controller to press takes the profile** (`src/players.cpp`): the game starts in Party Play, as GH5. The
  window opens there and closes for good with the first press. **Off:** `ghbandhero_profile_follows_press = false`.
- **Each slot's kind of controller follows the one played** (`src/players.cpp`).
- **Frame rates above 60** (`src/framerate.cpp`, the launcher's frame rate row): the vblank clocks, the scripts' game-frame
  waits, the button presses scripts poll, the whammy on a sustain, the loading screen's animations, the strings' vibration
  and fade, the frets falling back after a hit (`button_checker`, `0x716BADDC`), the camera cuts and the drummer (below),
  each counted by the frames' measured length.
- **Clothes shooting out for a frame**: [clothes-stretching.md](clothes-stretching.md). **Off:**
  `ghbandhero_skinning_order_fix = false`.
- **Ultrawide** (`src/widescreen.cpp`, 9 hooks): the 3D views widened, the 2D layer narrowed to 16:9 in the center, black
  bars outside a song, the highway's depth mask narrowed with the board. **Off:** 16:9 in the launcher
  (`ghbandhero_ultrawide = false`), `ghbandhero_ultrawide_hud`, `ghbandhero_menu_bars`, `ghbandhero_ultrawide_overlays`.
- **Quitting from the main menu** (`src/quit_prompt.cpp`): B asks. The main menu's element and the dialog's are GH5's.
  **Off:** `quit_prompt = false`.
- **V-Sync with G-Sync or FreeSync**, Alt+Tab from fullscreen, the keyboard's controls: GH Metallica's, through GH5.

The scripts these depend on are the same bytes as GH5's: star power's drain, `button_checker`, the band's strings
(`0x02B91F04`); the quit prompt's and Party Play's elements work as in GH5. Of Band Hero's 9005 scripts, 96.4 % exist in GH5
and 90 % of those are byte for byte the same.

## Where this game differs

**The screen effects' composite** (`sub_8263E130`, GH5's `sub_82643598`) has one more fragment in Pass0, `_projection1`,
bit `0x200` of the flags; Pass1 no longer chooses `_visualizer1`. It is the one function the patches touch that grew (17
instructions) and the flags' base moved from `r18` to `r19`, so its three hooks were placed by hand, each after the
same read of the flags (`0x8263E508`, `0x8263E658`, `0x8263E71C`). The bits of the five switches are GH5's, checked in the
code that builds the variants' numbers. Nothing turns `_projection1` off. **Off:** `ghbandhero_fx_bloom`,
`_depth_of_field`, `_grain`, `_color`, `_vignette`, all on by default (`ghbandhero_fx_color = false` takes the grain too).

**The presentation** tests a new setting (`+584` of the game's configuration, in `sub_8254FB40` and `sub_824285D0`). It
changes which of two ways a frame is presented; both reach the same waits, where the hooks are.

## GH5's fixes of 2026-11-06/07 (ported 2026-11-08)

GH5Recomp `722e5c0`, `575136e`, `e0c0e0c`, `cdec72f` and `c0cb7b0`, with GH Warriors of Rock's port of them (`3690a15`)
for what this game shares with it:

- **Camera cuts above 60.** The engine draws the stage and the crowd in one frame and the band in the next
  (IsPrimaryFrame), and some scripts align to that with game-frame waits. Held to 60 passes a second, in GH5 each camera
  cut showed 1 to 5 frames with no scene. Here, as in GH WoR, two scripts align (`0xCAD261DF` and `0x6506F6C8`); they and
  their eight callers run every frame (`kFrameExactScripts`). The cut itself calls neither, as in GH WoR, where the
  symptom was not seen either way. Checked: Party Play at 160 FPS, 12,367 frames, none without a scene.
- **The drummer in slow motion** (GH5: his hits off the song at 160 FPS). Script `0x473164C7`, GH5's bytes, measures its
  step across a timed "Wait 1 frame": above 60 Hz that wait is a real frame (`GHBandHeroDrummerWaitHook`).
- **The scripts that time themselves:** five scripts that add GetDeltaTime to a timer each pass and wait a game frame run
  every frame (`0xEC23DF7E`, `0xE66D1DA1`, `0x4E850682`, `0xE20DF41E`, `0x3E4FDDAF`). The jam studio's arrows (`0xCFDDF140`)
  stay held, as in GH5 and GH WoR: they also step by the pass, and every frame they would speed up 2.67 times as soon.
- **The frames' real length.** What is counted in frames above 60 Hz (the whammy's 5 frames, the strings' fade, the loading
  screen's steps, the margins of the script waits and of the presses given to scripts) uses the frames' measured length,
  not 1 / the chosen rate, so it keeps the console's time when the game falls below the chosen rate
  (`GHBandHeroMainFrameHook` at the end of the main thread's frame).
- **The skinning order**: [clothes-stretching.md](clothes-stretching.md).
- **NativeXRenderer the default, its antialiasing once a frame, Shadow quality**:
  [native-x-renderer-ghbandhero.md](native-x-renderer-ghbandhero.md).

None of these has been played here yet: checked by the tools in Party Play (the first screen) at 160 and 60 FPS, on both
renderers, 0 errors.

## Warriors of Rock's additions

- **FXAA and SMAA** (GH5 has them since too): [native-x-renderer-ghbandhero.md](native-x-renderer-ghbandhero.md).
- A scan of all 9005 scripts for those that add GetDeltaTime finds the same ones as GH5's 8795: none of the 326 scripts
  only this game has does it.

## Known

- In ultrawide a one pixel column of the image stays at the left end of the 16:9 area in the menus. GH5 has the same one.
- Not tried by a player yet: a real guitar, drums and singing, the sound by ear and its sync, star power, the whammy above
  60 FPS, the career, a second controller, the other languages, DLC.
