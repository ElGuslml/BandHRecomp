# The fixes ported from Guitar Hero 5

**Status: in testing (2026-11-06).** Band Hero is Guitar Hero 5's code base one step on: its executable's original
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
  and fade, the frets falling back after a hit (`button_checker`, `0x716BADDC`).
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

## Warriors of Rock's additions

GH5 lacks these two; they are GH WoR's.

- **FXAA and SMAA**: [native-x-renderer-ghbandhero.md](native-x-renderer-ghbandhero.md).
- **Six more scripts run every frame above 60 Hz.** `0xEC23DF7E`, `0xE66D1DA1`, `0x4E850682`, `0xE20DF41E`, `0xCFDDF140`
  and `0x3E4FDDAF` add GetDeltaTime to a timer each pass and wait a game frame: held to 60 passes a second, their timers
  would run at 60 / 160 of their speed at 160 FPS. GH5's port left them held, GH WoR's did not. A scan of all 9005 scripts
  finds the same ones as GH5's 8795: none of the 326 scripts only this game has does it.

## Known

- In ultrawide a one pixel column of the image stays at the left end of the 16:9 area in the menus. GH5 has the same one.
- Not tried by a player yet: a real guitar, drums and singing, the sound by ear and its sync, star power, the whammy above
  60 FPS, the career, a second controller, the other languages, DLC.
