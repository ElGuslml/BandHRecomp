// ghbandhero - startup (see startup.h).

#include "startup.h"

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/logging.h>
#include <rex/system/flags.h>

#include <algorithm>
#include <cwctype>
#include <array>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "profile_store.h"
#include "project.h"

namespace ghbandhero {
namespace {

// Whether `folder` is inside the repository the build was made from (the parent of its assets\ folder): a
// development build run from out\build\. A copy anywhere else (a release package, an installation) is not, even
// on the machine that built it.
bool InsideRepository(const std::filesystem::path& folder, const std::filesystem::path& repository_assets) {
  std::error_code ec;
  auto normal = [&ec](const std::filesystem::path& p) {
    std::wstring s = std::filesystem::weakly_canonical(p, ec).generic_wstring();
    std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return wchar_t(std::towlower(c)); });
    if (!s.empty() && s.back() != L'/') s += L'/';
    return s;
  };
  std::wstring root = normal(repository_assets.parent_path());
  std::wstring inside = normal(folder);
  return root.size() > 1 && inside.compare(0, root.size(), root) == 0;
}

// CRC32 of the default.xex this code was recompiled from (13,938,688 bytes, title ID 4156085C, version 0.0.0.1,
// media ID 428806E4).
constexpr uint32_t kGameExecutableCrc32 = 0x57900866;

// The GPU settings KameoRecomp, GRAWRecomp and POTCAWERecomp share, none shown to be needed by this game yet:
// when something draws wrong, try the SDK default of each one from the command line first. Each comment says what
// the setting does and what the other value gives back.
constexpr const char kDefaultConfig[] =
    R"(# BandHeroRecomp.exe runtime configuration, loaded at startup. The game recreates this file with these
# defaults if it is missing, and adds the settings an existing file lacks. Band Hero Launcher saves its choices to
# ghbandhero_options.toml, which is loaded after this file; command line options (--name=value) win over both.
log_level = "info"
# Do not wait at the end of the frame for new D3D12 pipelines (a stutter the first time an effect
# appears): the object appears a few frames later. false = wait.
gpu_async_pipelines_no_frame_wait = true
# Translate the shaders' control flow with if blocks instead of a switch. In POTCAWERecomp the
# switch (the SDK's default) made the pixel shader body re-execute under GPU load and lit objects
# turned into flat saturated colors. true = the SDK default.
dxbc_switch = false
# Clears drawn through a surface with another MSAA mode or pitch than the scene: without these
# settings the SDK copies the render target into that alias and back around each clear. Run the
# vertex shader of unclipped draws on the CPU to know how much EDRAM they use (the Xenia default;
# the SDK default is false)...
execute_unclipped_draw_vs_on_cpu = true
# ...and skip the copy into a render target when the draw replaces every sample of it.
render_target_skip_transfers_on_overwrite = true
# Write constant full-coverage draws (clears) straight into the render targets that hold that
# memory. Off: in KameoRecomp it removed all the geometry from a cinematic (a bug not found yet).
render_target_draw_clears = false
# Keep the GPU copies of memory the CPU uploaded valid across frames. With the SDK default (true)
# every page uploaded by the CPU is re-uploaded the next frame even if it did not change. false is
# the original Xenia behavior (true is a hack for one game).
clear_memory_page_state = false
# Anisotropic filtering for textures with mipmaps, sharper at oblique angles: 5 = 16x, 4 = 8x,
# 3 = 4x (the SDK default), 2 = 2x, 0 = off, -1 = only what the game asks. Band Hero Launcher's
# "Anisotropic filtering" row sets it.
anisotropic_override = 5
# Start the game's audio callback at least 3 ms after its last call started (the console calls it every
# 5.3 ms), not in the bursts of two the host device frees frames in. FMOD's mixer polls how far its
# two-block ring has been read and missed most blocks: from the title screen on, the music played as
# noise. Counted from the start, the callback's own work is part of the 3 ms, so a slower CPU keeps up
# (SDK change 82; docs/audio-crackling.md). 0 = the SDK default (no spacing).
audio_callback_min_interval_us = 3000
# Audio frames queued for the sound device (5.3 ms each): 4 is the least the SDK allows, 21 ms less
# delay between a strum and its sound than the SDK's 8. With the calls spaced the queue stays full
# (no gap in a whole song, measured); the game's calibration covers what is left. 8 = the SDK default.
audio_maxqframes = 4
# The SDK's audio threads (its audio worker runs the game's sound callback) in Windows' multimedia class
# "Pro Audio", as the sound device's thread: a busy PC holds them up less.
# false = normal priority (the SDK default).
audio_thread_priority = true
# When the game's sound still comes late (its audio thread held up longer than the 21 ms queue), the silence played
# meanwhile is made up by dropping as many of its next frames: a short skip, and the music stays with the notes (the
# SDK default, false, plays the rest of the song that much late, until a pause).
audio_keep_time = true
# Band Hero Launcher's "Low latency audio" (audio_wasapi_exclusive, off by default) takes the output device in
# exclusive mode, past Windows' audio engine. With it, 10 ms kept in the device's buffer and 2 of the game's frames
# queued instead of 4: on a Focusrite Scarlett 2i2, 20 ms from the game's frame to the driver playing it, where the
# shared output takes 77 ms (measured in GH Metallica). Without it these do nothing.
audio_wasapi_exclusive_buffer_ms = 10
audio_wasapi_exclusive_queue = 2
# The game is told the speakers it plays on (the SDK's XAudioGetSpeakerConfig, its change 121): Dolby Digital 5.1 only
# with audio_channels = "surround", or "auto" on a device of more than two channels; two speakers with "stereo", "mono"
# or "auto" on a stereo device, as a console without Dolby. GH Warriors of Rock, the same engine, mixes the same six
# channels either way (measured there: its centre and LFE empty, its rears a third of its fronts, whatever it is told),
# so the sound does not change; Dolby Digital adds -30 ms to the notes' and the input's timing (the scripts'
# default_lag_settings_dolbydigital, the same as GH WoR's), which ghbandhero_dolby_timing (src/mods.cpp, on) keeps whatever it
# is told: no one calibrates again. Band Hero Launcher's Channels row (Automatic, 5.1, Stereo, Mono) writes audio_channels.
# false = always Dolby Digital, as before.
audio_speaker_config_from_output = true
# V-Sync (Band Hero Launcher's row writes both): NativeXRenderer presents with FIFO, each frame shown once and never
# torn, and the game's vertical blank follows the monitor's where the frame rate is its refresh rate or a whole
# fraction of it, so the game never runs ahead of the display. Measured in GH Metallica at 160 FPS on a 160 Hz display:
# 4 ms from present to display, no frame dropped (MAILBOX with the SDK's timer: 16-19 ms, NVIDIA queuing three
# frames). "immediate" and false = V-Sync off (tearing, or a G-Sync / FreeSync display following the game).
native_x_renderer_present_mode = "vsync"
vblank_follows_display = true
# V-Sync in a window that does not cover its display (NativeXRenderer D182; GH Warriors of Rock's local_docs/74, 2.1b):
# MAILBOX there, the compositor or a display plane showing one whole picture a refresh. Measured in GH WoR at 160 FPS
# in a 1280x720 window on a display plane: every frame shown, the shown frames' 1 % low 158.3-158.7 FPS (FIFO
# 151.6-158.6) and 2.6-3.1 ms from present to display (FIFO 2.4-6.2, by its phase). A composed window (Wallpaper Engine
# animating the desktop behind it) had FIFO's second phase: frames held a refresh longer, a 1 % low of 56 FPS.
# Borderless and full screen keep FIFO (MAILBOX there: 10.4 ms to the display against 4.0). "vsync" = FIFO in a window
# too, as before.
native_x_renderer_window_present_mode = "mailbox"
# NativeXRenderer records its DiligentCore calls on a thread of their own while its renderer's thread prepares the next
# commands (D154, D155), on as in GH Warriors of Rock (its Quest song with the venue every frame at 3440x1440: at 240
# FPS every stretch at 230 or more, 225.7 without it; the renderer's thread at about half of the frame). With V-Sync,
# D172 drops one frame when the display's queue stays a frame too deep. false = one thread, as before.
native_x_renderer_record_thread = true
# Where the vertical blank runs on the SDK's timer (a G-Sync / FreeSync display, V-Sync off): its ticks within a few
# microseconds of their time (700 us of spinning each; without, up to half a millisecond late), as the display shows
# each frame when it comes.
vblank_timer_spin_us = 700
# A thread entering a game lock another thread holds watches it for up to 20 us before it sleeps (the console's
# kernel puts it to sleep at once when the game asks for no spin, as it does for its job queue's lock). In GH
# Metallica's songs the main thread and the three job workers took that lock by turns and slept and woke each other
# ~92,000 times a second; with this ~31,000 (its docs/cpu-usage.md). 0 = the SDK default.
critical_section_spin_us = 20
# Report a controller (and the keyboard's emulated one) as a guitar: the game gives a plain gamepad
# the vocals, which need a microphone. Frets on A (green), B (red), Y (yellow), X (blue) and LB
# (orange), strumming on the D-pad, the whammy bar on the right stick, Star Power on Back. A real
# guitar or drum kit keeps its own kind. 8 = drum kit, 0 = the SDK default (a gamepad).
gamepad_subtype = 6
# A controller plays a song as in Guitar Hero III, which had a layout for one (the menus stay as they are: A, B, the
# D-pad): the frets on LT (green), LB (red), RB (yellow), RT (blue) and A (orange), no strum (a change of the frets
# held plays, while one is held: a fret pressed, or one released while another is held), Star Power on the D-pad or
# Back, the whammy bar on either stick (src/controller_layout.cpp). "guitar": an Xbox 360 guitar's controls, frets on
# A, B, Y, X and LB, strumming on the D-pad, the whammy bar on the right stick, the tilt on its Y, Star Power on Back.
gamepad_guitar_layout = "gh3"
# The keyboard playing guitar plays a song on the same layout (SDK change 125, keyboard_guitar_layout; Band Hero Launcher's
# Keyboard page, Play style): the game's controller code plays its fret keys (green, red, yellow, blue, orange on the
# keys of A, B, Y, X and LB below; the open note on RB's), a fret key pressed plays, no strum: in a song the arrows (the
# D-pad) do nothing and the strum switch (mnk_tap_target) does not act. Star Power on Back's keys, the whammy bar on
# the right stick's key. The menus keep the keys as they are. "guitar": an Xbox 360 guitar's, as before (the arrows
# strum).
keyboard_guitar_layout = "gh3"
# A PS3 Guitar Hero guitar (a CRKD guitar on its P3 setting too): yellow and blue, swapped on that protocol, put right,
# and its whammy bar, which rests at the middle of its axis, at rest there.
guitar_ps3_protocol = true
# An Xbox 360 guitar reads its whammy bar half pressed until the bar first moves: at rest until then.
guitar_whammy_rest = true
# A controller or the keyboard playing guitar: the whammy bar at rest while the right stick is centred (or its keys
# are up), and pressed as far as the stick goes from the centre, on either side. The stick's own value, 0 at rest,
# is the bar held about half way for the game (GH Metallica's engine: its bar rests at -0.76 of the axis).
gamepad_whammy_rest = true
# A PC microphone 12 dB quieter than Windows records it (usb_microphone_1, off until Band Hero Launcher's
# Microphone chooses one): the game turns its own USB microphone down to the bottom of its
# range for a song; a PC microphone at full level also hears the room and the speakers (GH Metallica's
# first test, a condenser microphone at 0 dB: the pitch line jumped). The launcher's Gain row changes it.
usb_microphone_1_gain = -12
usb_microphone_2_gain = -12
# Start pressed on a second controller: the game asks that controller to sign in, and only the first
# has a profile. The sign-in UI closes as when the player cancels it, and the game shows its "not
# signed in" message (sign in / continue without saving); the SDK default reported a sign-in change
# and the game waited for that controller's profile forever on an empty screen.
signin_ui_cancel = true
# The other controllers are the profile's guests, as on the console (Name (1)): signed in, so the game
# does not ask them to sign in, with no saves or settings of their own. The controller that presses a
# button at the title screen is the profile's (ghbandhero_profile_follows_press, src/players.cpp).
signin_guests = true
# The keyboard as a guitar (with mnk_mode; Band Hero Launcher's Keyboard page changes them): the frets
# on D, F, J, K and L (green, red, yellow, blue, orange: A, B, Y, X, LB), strumming on the up and down
# arrows (the D-pad), Star Power on Space or Backspace (Back), Start on Enter or X, the whammy bar on the
# right arrow (the right stick). The sticks' other keys are off: W A S D, F and K would move them with the
# frets.
keybind_a = "D"
keybind_b = "F"
keybind_y = "J"
keybind_x = "K"
keybind_left_shoulder = "L"
keybind_dpad_up = "Up"
keybind_dpad_down = "Down"
keybind_back = "Space,Backspace"
keybind_start = "Return,X"
keybind_rstick_right = "Right"
keybind_rstick_up = ""
keybind_rstick_down = ""
keybind_rstick_left = ""
keybind_rstick_press = ""
keybind_lstick_up = ""
keybind_lstick_down = ""
keybind_lstick_left = ""
keybind_lstick_right = ""
keybind_lstick_press = ""
# The keyboard's frets strum as they are pressed with mnk_tap_target = "dpad_down" (the launcher's switch),
# frets pressed within mnk_tap_window_ms of each other making one chord: one strum. Empty: off, the
# arrows strum.
mnk_tap_sources = "a,b,y,x,lb"
mnk_tap_target = ""
# The SDK's developer keys (F3 the debug overlay, F4 its settings window, ` the console) do nothing, so that
# the keyboard can play with them (a player's request in GH Metallica, 2026-11-02); Band Hero Launcher's
# Settings > Game > Debug keys turns them on. A key on one of the keyboard's controls plays only that control either
# way, F7 (the achievements) included.
debug_hotkeys = false
# The same for a controller playing guitar (its frets A, B, Y, X and LB) with gamepad_tap_target = "dpad_down" (the
# launcher's switch on the controller's page), only in a song as the keyboard's (src/keyboard.cpp's gate).
gamepad_tap_sources = "a,b,y,x,lb"
gamepad_tap_target = ""
# The game's screen effects over the 3D image (src/screen_fx.cpp; the note highway and the HUD are drawn after
# them): false turns one off. Bloom: the glow around lights. Depth of field: the background blurred in some camera
# shots and behind the menus and the pause menu. Grain: the film grain. Color: the color correction (each venue's
# look, fades to black, the image greyed while the rock meter is red); with it off the grain is off too (in GH Van
# Halen the grain was far stronger without the color pass). Vignette: darkened corners, where a venue turns it on.
# true = as on the console.
ghbandhero_fx_bloom = true
ghbandhero_fx_depth_of_field = true
ghbandhero_fx_grain = true
ghbandhero_fx_color = true
ghbandhero_fx_vignette = true
# The 3D scene drawn in every frame (src/split_render.cpp): the game draws the venue and the crowd in one frame and the
# band and the screen effects in the next, so its 3D image changes at half the frame rate (30 times a second on the
# console; the note highway and the HUD every frame). true turns that split off: the whole scene in every frame, for
# about twice the 3D work a frame (CPU and GPU). false = as on the console.
ghbandhero_scene_every_frame = false
# The game's own debug menu (Neversoft's, src/debug_menu.cpp): true adds an item named (debug menu) to the main menu
# and a Debug Menu item to the pause menu (Unlock All, Skip Into Song, Change Venue, Select Slomo, Show Song Time, Toggle GPU
# Time, ...). Some items need Neversoft's development tools and do nothing. false = as on the console.
ghbandhero_debug_menu = false
# The intro video at the start (GH5_1_Logo_Movie, 22 s, played while the game loads) ended as soon as the game has
# loaded, as if Start were pressed then: Party Play right after the loading (src/intro.cpp). false = as on the
# console.
ghbandhero_skip_intro = false
# Guitar Hero World Tour's Star Power sound (src/star_power_reverb.cpp, from GH Warriors of Rock): while a player's
# Star Power is on, their instrument goes through World Tour's echo timed to the song, reverb and high pass, with World
# Tour's settings for each instrument and for one or more players, instead of the venue's reverb. The band, the crowd
# and the vocals do not change. false = as on the console. With it on, ghbandhero_star_power_reverb_send_db (dB added to
# World Tour's send, 0) and ghbandhero_star_power_reverb_multi_preset (true: World Tour's settings for several players for
# one player too) can be added here to tune it.
ghbandhero_star_power_reverb = false
# The launcher's Mods (src/mods.cpp), changes of the game's own script data when it starts. The note highway's size
# with one player in percent (the launcher offers 80-120; from the strike line, the gems, sidebars and meters with it);
# its length in time in percent (100, 125, 150, 170: the notes seen that much earlier, the console's highway continued
# in its own perspective, at the same speed at the strike line, for every player); its perspective in percent of the
# console's (25-150: flatter or steeper), and with ghbandhero_highway_keep_speed the notes at the console's speed at the
# strike line whatever the perspective (the times on the highway shorter below 100, longer above); its look, "original"
# or "dark" (the community's black highway). 100, 100, 100, false and "original" = as on the console.
ghbandhero_highway_scale = 100
ghbandhero_highway_length = 100
ghbandhero_highway_perspective = 100
ghbandhero_highway_keep_speed = false
ghbandhero_highway_style = "original"
# The whammy bar's bend of a held sustain (true, as on the console; false keeps the pitch, the bar still wobbles the
# note and fills Star Power), and how much of your instrument's track stays on a miss or a strum without a note, in
# percent (0, as on the console: muted until the next hit; 100 = never muted; guitar and bass; not in the
# single-stream songs). src/mods.cpp.
ghbandhero_whammy_sound = true
ghbandhero_miss_lower = true
ghbandhero_miss_volume = 0
# The note highway's opacity in percent: below 100 the venue shows through it (src/mods.cpp). 100 = as on the console.
ghbandhero_highway_opacity = 100
# Images that replace the game's textures (NativeXRenderer's D176): a folder next to the executable, each image named
# by the texture's hash (16 hexadecimal digits, then anything; .png, .jpg or .jpeg; subfolders too); the note highway's
# is DD482D15759B682F (computed from the disc; native_x_renderer_texture_log = true logs every texture's hash as it
# loads).
native_x_renderer_texture_replacements = "textures"
# The note highway's picture from a folder (src/highway_images.cpp; the launcher's Mods > Highway gallery writes them):
# the folder, the picture (a file of it, "<random>" for a random one at each song, empty for the game's own), where a
# random one comes from ("all" or "favorites"), the favorites (names separated by |), and how bright pictures are
# darkened toward black for all ("off", "auto" or a percent) and for some (name:value separated by |).
ghbandhero_highway_folder = ""
ghbandhero_highway_image = ""
ghbandhero_highway_random_from = "all"
ghbandhero_highway_favorites = ""
ghbandhero_highway_dim = "off"
ghbandhero_highway_image_dims = ""
# The mouse cursor hidden over the game after cursor_auto_hide_ms without moving it (shown again when it moves, while
# another window has the focus, and over the SDK's overlays). false = always shown.
cursor_auto_hide = true
cursor_auto_hide_ms = 2000
)";

uint32_t Crc32(const std::vector<char>& data) {
  static const std::array<uint32_t, 256> table = [] {
    std::array<uint32_t, 256> t{};
    for (uint32_t i = 0; i < 256; ++i) {
      uint32_t c = i;
      for (int k = 0; k < 8; ++k) {
        c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
      }
      t[i] = c;
    }
    return t;
  }();
  uint32_t crc = 0xFFFFFFFFu;
  for (char byte : data) {
    crc = table[(crc ^ uint8_t(byte)) & 0xFF] ^ (crc >> 8);
  }
  return crc ^ 0xFFFFFFFFu;
}

#ifdef _WIN32
// MB_* flags; returns the button pressed.
int ShowMessage(const std::wstring& text, UINT flags) {
  return MessageBoxW(nullptr, text.c_str(), L"" GAME_TITLE, flags);
}

std::wstring Hex32(uint32_t value) {
  wchar_t buffer[16];
  swprintf(buffer, 16, L"%08X", value);
  return buffer;
}
#endif

}  // namespace

std::filesystem::path DefaultGameRoot() {
  std::filesystem::path next_to_exe = rex::filesystem::GetExecutableFolder() / "assets";
  if (std::filesystem::exists(next_to_exe / GAME_XEX)) {
    return next_to_exe;
  }
  // The repository's assets\, for a development build run from the repository only: a package or an installation
  // uses its own (the launcher extracts the game there), even on the machine that built it.
  std::filesystem::path repository = GHBANDHERO_DEFAULT_GAME_ROOT;
  if (InsideRepository(rex::filesystem::GetExecutableFolder(), repository) &&
      std::filesystem::exists(repository / GAME_XEX)) {
    return repository;
  }
  // Neither has the game: report the folder next to the executable (the release layout), not a path
  // of the machine the build was made on.
  return next_to_exe;
}

void ConfigureUserData(std::filesystem::path& user_data_root, std::filesystem::path& cache_root) {
  if (!rex::cvar::Query<std::string>("user_data_root").empty()) {
    return;  // chosen on the command line
  }
  std::filesystem::path portable = rex::filesystem::GetExecutableFolder() / "userdata";
  std::error_code ec;
  std::filesystem::create_directories(portable, ec);
  // Writable? Logging is not set up yet: the folder in use is logged later
  // (GhbandheroApp::SetupEnvironment).
  std::filesystem::path probe = portable / ".write_test";
  {
    std::ofstream f(probe, std::ios::binary);
    if (!f) {
      return;
    }
  }
  std::filesystem::remove(probe, ec);
  user_data_root = portable;
  if (rex::cvar::Query<std::string>("cache_root").empty()) {
    cache_root = portable / "cache";
  }
}

void ConfigureProfile(const std::filesystem::path& user_data_root) {
  if (!REXCVAR_GET(profile_root).empty()) {
    return;  // chosen on the command line
  }
  if (std::optional<launcher::profiles::GameProfile> profile = launcher::profiles::ForGame(user_data_root)) {
    std::u8string folder = profile->folder.u8string();
    REXCVAR_SET(profile_root, std::string(folder.begin(), folder.end()));
    if (REXCVAR_GET(profile_name).empty()) {
      REXCVAR_SET(profile_name, profile->name);
    }
  }
}

// Creates ghbandhero.toml with the defaults, or appends to an existing one the settings (with their
// comments) it does not mention, so that a setting added in a newer build takes effect without
// deleting the user's file. A setting the file has, with any value, is left alone.
void EnsureDefaultConfig(const std::filesystem::path& config_path) {
  // Logging is not initialized yet (the config sets it up): nothing to log here.
  std::string existing;
  if (std::filesystem::exists(config_path)) {
    std::ifstream in(config_path, std::ios::binary);
    existing.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  } else {
    std::ofstream f(config_path, std::ios::binary);
    f << kDefaultConfig;
    return;
  }
  auto has_key = [&existing](const std::string& key) {
    size_t pos = 0;
    while ((pos = existing.find(key, pos)) != std::string::npos) {
      size_t line_start = existing.rfind('\n', pos);
      line_start = line_start == std::string::npos ? 0 : line_start + 1;
      size_t after = pos + key.size();
      bool only_spaces_before = existing.find_first_not_of(" \t", line_start) == pos;
      size_t next = existing.find_first_not_of(" \t", after);
      if (only_spaces_before && next != std::string::npos && existing[next] == '=') {
        return true;
      }
      pos = after;
    }
    return false;
  };
  std::string missing;
  std::string comments;
  std::istringstream defaults(kDefaultConfig);
  std::string line;
  while (std::getline(defaults, line)) {
    if (line.empty()) {
      comments.clear();
      continue;
    }
    if (line[0] == '#') {
      comments += line + '\n';
      continue;
    }
    size_t eq = line.find('=');
    if (eq == std::string::npos) {
      comments.clear();
      continue;
    }
    std::string key = line.substr(0, eq);
    key.erase(key.find_last_not_of(" \t") + 1);
    if (!has_key(key)) {
      missing += comments + line + '\n';
    }
    comments.clear();
  }
  if (missing.empty()) {
    return;
  }
  std::ofstream f(config_path, std::ios::binary | std::ios::app);
  if (!existing.empty() && existing.back() != '\n') {
    f << '\n';
  }
  f << missing;
}

bool CheckGameFiles(const std::filesystem::path& game_root) {
  std::filesystem::path exe = game_root / GAME_XEX;
  std::ifstream f(exe, std::ios::binary);
  if (!f) {
    REXLOG_ERROR("[startup] Game files not found: {} does not exist", exe.string());
#ifdef _WIN32
    ShowMessage(L"Game files not found.\n\n"
                L"Copy the files of your " GAME_TITLE " (Xbox 360) disc into:\n" +
                    game_root.wstring() + L"\n\nso that this file exists:\n" + exe.wstring(),
                MB_OK | MB_ICONERROR);
#endif
    return false;
  }
  std::vector<char> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  uint32_t crc = Crc32(data);
  if (crc == kGameExecutableCrc32) {
    REXLOG_INFO("[startup] {}: CRC32 {:08X} (expected version)", exe.string(), crc);
    return true;
  }
  REXLOG_WARN("[startup] {}: CRC32 {:08X}, expected {:08X}: a different version of the game",
              exe.string(), crc, kGameExecutableCrc32);
#ifdef _WIN32
  int answer = ShowMessage(L"" GAME_XEX " is not the version this build was recompiled from "
                           L"(CRC32 " + Hex32(crc) + L", expected " + Hex32(kGameExecutableCrc32) +
                               L").\nThe game will most likely crash.\n\nStart anyway?",
                           MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
  return answer == IDYES;
#else
  return true;
#endif
}

}  // namespace ghbandhero
