# The fixes from GH Metallica of 2026-11-11 to 13

**Status: built, not tried in the game yet (2026-11-13).** GH Metallica 1.0.0-beta.1's game code and launcher, without
its online play, as GH5 took them (GH5Recomp `74d690f`, its `docs/fixes-from-gh-metallica.md`): Band Hero is on the
same SDK (`4348a34`), NativeXRenderer (`c8e3e5c`) and launcher (3.21.1). Band Hero is GH5's code base one step on, so
each of GH5's sites was mapped into this game (`tools/port/gh5_bh_map.pkl`, `fmatch.py`), then the instructions
compared one by one (`show.py`, `verify.py`: the same instructions and registers around each hook, but for data
addresses), and the scripts read from this disc's `qb.pab.xen`, named by its `dbg.pak.xen`. Where Band Hero differs
from GH5, the section says how.

## The game's debug menu

**Option, off by default:** the launcher's Settings → Game → "Game debug menu", or `ghbandhero_debug_menu = true`. The
main menu gets a "(debug menu)" item at its end and the pause menu a "Debug Menu" item (not in the tutorials):
Neversoft's debug menu (New Game Modes; In Game: Skip Into Song, Change Venue, Select Slomo, the lag tests, Character
Select; HUD & UI: Show Song Time, Show Measures, Rock Meter Debug; Band Setup; Tools: Play Credits, Model Viewer, Toggle
GPU Time, Toggle CPU Time; Unlock All). Its texts are in English, some items need the developers' tools, and Unlock All
changes the save. **How:** the game adds these items when the script global `enable_button_cheats` is 1; its startup
sets it to 0 on a retail build (`if RetailBuild`, as GH5). `src/debug_menu.cpp` makes it read as 1 only in the two
scripts that build those menus (`create_main_menu_elements`, `ui_create_pausemenu`; both differ from GH5's elsewhere,
not in these tests); everywhere else it stays 0 (`guitar_startup_systems` would otherwise start the PC
viewer). Hooks: the script interpreter's two expression evaluations in `sub_823B35F0` (0x823B37BC / 0x823B37C4,
0x823B3B24 / 0x823B3B28) and the `$name` resolve at the entry of `sub_823AAE40` (GH5's `sub_823B3220` and
`sub_823AAB98`); the running script's name is at +244 of the interpreter's script, as in GH5. The log says
"[debug menu] script ... offers the debug menu" for each menu.

## Ending the intro video once the game has loaded

**Option, off by default:** Settings → Game → "Skip the intro video", or `ghbandhero_skip_intro = true`. Band Hero's
start is GH5's: it plays one video as its loading screen, here `GH5_1_Logo_Movie` (22.1 s; GH5's is
`Intro_FINALanimatic_0209`), loads the game behind it, then waits for the video's end; once loaded, Start or the green
fret ends it (`LoadingEndMovie` sets `end_movie_early`). The video cannot be refused as GH Metallica's are (the loading
screen waits for it to play before the game loads on), so `src/intro.cpp` makes `end_movie_early` read as 1 in
`destroy_loading_screen_spawned` (GH5's script byte for byte): the video ends the moment the game has loaded, as if
Start were pressed then, and Party Play follows. What plays while the game loads still shows. The same hooks as the
debug menu (one hook per address). The log says "[intro] the game has loaded: the intro video ends".

## Drums, five or four lanes

GH Metallica's of 2026-11-11 (SDK change 87, launcher 3.19.1; its `docs/drums.md`): Controls → a controller or the
keyboard → Instrument: Drums, then Drum kit: 5 lanes (Guitar Hero) or 4 lanes (the game's Rock Band highway, as with a
Rock Band drum kit on the console). Band Hero has GH5's code for it: `sub_82489908` makes a drum kit (XInput subtype 8)
the game's type 11 and keeps its capability flags' bit 0 (128 at +216 of the port), `sub_82489C10` turns 11 into 10
(five lanes) at its first state read (0x8248A074) unless that bit was set, and the script function `UseFourLaneHighway`
(its C function table entry at 0x82B0C458: `sub_821D4288` / `sub_821D4190`) plays type 11 on the four-lane highway
(`generate_pos_table`, `setup_button_up_models` with its `4d` materials and `touch_glow_props_rb_drum`: GH5's scripts
byte for byte; here twelve scripts ask `UseFourLaneHighway`, GH5's ten and `autolaunch_spawned_and_wait`,
`select_start_song`). So launcher 3.21.1's `kHasFourLaneDrums` stays true (its changelog lists this engine as five
lanes only: not so). The keyboard's drum keys are its own (D F J K L, four lanes K green; Space the kick, Backspace Star
Power; `launcher/game.h`). Not played through a song yet. **Off:** Instrument: Guitar.

## Other things that came with the move

- The mouse cursor hides over the game after 2 seconds without moving it (SDK change 103): `docs/mouse-cursor.md`.
- Settings → Audio (launcher 3.19, SDK change 85: the output device by name, the channels, the volume, mute while
  another window is in front; low latency audio moved there from Game) and Settings → Graphics → the graphics card
  (SDK change 86, NativeXRenderer D141). Nothing of the game's own.
- NativeXRenderer D142 with SDK change 88 (on by default; GH Metallica's `docs/crowd-stutters.md`): a signal the game
  waits for waits only for the renderer's work submitted before it, not for everything queued since (in GH Metallica
  up to 2.6 ms a frame in a crowded venue). **Off:** `native_x_renderer_precise_gate = false`.

## Not ported

- **Online play** (GH Metallica's experimental online modes through Steam): not for Band Hero now. The SDK's
  `online_play` is off by default and the launcher shows no Online category (`kHasOnline` false).
- **The tap notes' trail at ultrawide** (GH Metallica's `GHMetallicaTapTrailHook`): Band Hero draws none, as GH5. Its
  trail renderer (`sub_82302428`, GH5's `sub_82302288`) reads its control points from arrays at 0x82CCA578 /
  0x82CCA640 that no code writes, and no script or code creates the trail (`createguitartaptrail` is never called).
- **The red border over the whole width at ultrawide**: GH5's fix, already here (`ghbandhero_ultrawide_overlays`,
  `docs/fixes-from-gh5.md`).
