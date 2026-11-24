// ghbandhero - the launcher's Mods page: the note highway's size, length, perspective, style and opacity, the whammy bar's
// sound, the instrument's volume on a miss and the Dolby Digital timing. Each is a change of the game's own
// script data (src/script_globals.cpp) or a hook, ported from GH Warriors of Rock (its src/mods.cpp, found and checked
// there in the game with a probe: its local_docs/47-mods-plan.md, 48, 49, 50; docs/mods.md). This game's scripts, data
// and functions were compared with that game's statically (the same engine: every value below is the disc's); nothing
// here has been run in this game yet. They apply when the game starts.
//
// - Highway scale (one player): the highway's size for one player is the first row of highway_scale_chart
//   (scripts\guitar\menu\freeplay_data.qb), the table the game scales the highways by for two to four players (0.97 x
//   0.77 down to 0.74 x 0.75; this game's values are GH Warriors of Rock's to the digit). setup_highway gives that row to
//   highway_scale_manager, which calls update_highway_scale (both compiled, scripts\qtocpp_scriptlist.qb): the highway,
//   its gems, sidebars, rock meter and Star Power meter scaled together, anchored at the strike line. Its
//   extra_offset_factor is left at 0 (the shape is the console's; highway_scale_manager picks its morph targets by
//   comparing it with the other rows', -0.4 and below).
// - The highway's projection (highway_guitar1, scripts\guitar\guitar_tweaks.qb: every highway outside the jam room,
//   get_highway_struct). It is drawn in 2D: from its far end at y = 305 to the strike line at 655 of the game's 720
//   (highway_height 350), 160 wide at the top and 512 at the bottom (highway_top_width x (1 + widthoffsetfactor 2.2)).
//   generate_pos_table's GeneratePosTable (sub_821D8700, the C function table's entry at 0x82B0C698; GH Warriors of
//   Rock's sub_82182F60, the same code) cuts it into rows that are linear in time (a note crosses the first
//   real_highway_lines = 1024 of highway_lines = 1152 in scroll_time - destroy_time: 1.5 s on Expert) and whose heights
//   follow h[0] = 1, h[i] = (h[i-1] * f) ^ e with f, e = perspectivefact 1.00108 and perspectiveexp 1.00125, normalised
//   over the 1024: in closed form ln h[i] = A (e^i - 1) with A = e ln f / (e - 1). The rows near the strike line are 9.4
//   times the far ones, and the width ratio 3.2 is close to the square root of that, as a real camera's would be.
//   Everything else at the far end follows the top width: a gem's scale is gem_start_scale x (1 + widthoffsetfactor x its
//   place from the top) (get_scale_on_guitar_string), and the beat lines (fretbar_start_scale) and sustains
//   (whammy_top_width, whammy_width_offset) are read the same way. This game has no open-note sustain width
//   (GH Warriors of Rock's whammy_top_width_open_note is not in its highway_guitar1).
//   - Highway perspective: the projection's compression times p (A x p, the same e): flatter below 100 %, steeper above;
//     the top width the one a camera gives with that compression (160 x 3.06 ^ (1 - p), 3.06 the console's square
//     root), the far end's sizes with it and the bottom's unchanged (widthoffsetfactor = 512 / top - 1).
//   - Highway length: k times the time on the highway, the same projection continued beyond the console's far end, so
//     the console's part stays where it was and the notes keep their speed at the strike line: the rows of the time
//     k T are those of the console's T extended (e' = e^k, A' = A e^(1024 (1 - k)), exact), the height then what the
//     console's part plus the longer part take (at 100 % perspective, 170 % adds 61 px: the far end at y = 244, 98
//     wide), the top width on the console's side lines, the texture's tiling times k, the sidebars with the height.
//     The time: every difficulty's scroll_time and destroy_time times k (a note crosses in k (scroll_time -
//     destroy_time): gem_scroller's time_offset and set_highway_speed / Set2DHighwaySpeed use that difference, also with
//     the Hyperspeed cheat's factor, difficulty_setup). GH Warriors of Rock's first version scaled the height and
//     scroll_time alone: the highway looked as seen from above and the notes came 18 % slower at 150 % (its
//     local_docs/49).
//   - The notes' speed at the strike line (GH Warriors of Rock's, its local_docs/65, 66): a note crosses the highway in
//     the same time at any perspective, so where the rows near the strike line are shorter (below 100 %) it arrives
//     slower in pixels, and faster above 100 %: s, the strike line's row's share of the highway against the console's
//     (0.444, 0.598, 0.784, 1.243, 1.509 at 25, 50, 75, 125, 150 %; the same for every length, and for Band Hero, whose
//     highway is GH Warriors of Rock's to the digit). With ghbandhero_highway_keep_speed the times on the highway are
//     multiplied by s as well as by the length (scroll_time and destroy_time, so a note crosses in k s (scroll_time -
//     destroy_time) and the Hyperspeed keeps its ratios): the notes reach the strike line at the console's speed and
//     show s times as long. The texture's tiling only follows the length: the highway's picture keeps its look and
//     moves with the notes. The scale is left out (it shrinks the gems with the highway). One factor for every player
//     count (the time is one per difficulty): with two to four players at 75 % their own s is 0.81 to 0.84, so their
//     notes come 4-7 % faster than the console's.
//   - Two to four players: each row of highway_scale_chart has its own projection (perspectivefact, perspectiveexp),
//     changed the same way, and its extra_offset_factor (the bottom's narrowing) divided by the top's change, so each
//     highway's bottom is the console's.
// - Highway style: highway_normal (guitar_tweaks.qb, [255 255 255 255]) is the highway's color, the texture's
//   multiplier (setup_highway; GuitarEvent_StarPowerOff and a miss's highway_pulse_black set it back), and
//   highway_starpower the color highway_pulse_black goes back to in Star Power ([64 255 255 255]). Dark is the
//   community's black highway (GH Warriors of Rock's): [0 0 0 255] and [64 0 0 255].
// - Highway opacity: the alpha of those colors and of highway_pulse (a miss's flash, [0 0 0 255]), and the cut-out of
//   the venue under the highway left out (GHBandHeroHighwayPrepassHook below).
// - Not ported: GH Warriors of Rock's crowd switch. That game hides its crowd with its own soundcheck cheat
//   (cheat_soundcheckmode read as 1 by options_cheats_set_soundcheck, which calls hide_crowd_models); this game has no
//   such cheat and no Crowd3D. Its crowd is impostors: at each song's start start_gem_scroller -> crowd_system_init ->
//   guitar_support_init_crowd -> create_crowd_models (scripts\guitar\guitar_crowd.qb) makes a ped game object for each
//   entry of the venue's <zone>_crowd_models, renders each into a viewport and puts that viewport over the venue's
//   billboard texture (CreateViewportTextureOverride on tex/models/Real_Crowd/crowd_atlas0.dds). Tried on 2026-11-18
//   (ghbandhero_357, Party Play): disable_crowd read as 1 in create_crowd_models (practice mode's state) left no peds, but the
//   billboards then showed crowd_atlas0.dds itself, coloured placeholder rectangles with silhouettes. Left to try:
//   hide_crowd_models {active=true} once the crowd is made (each ped's :hide, what CubeMapScreenshots and
//   practicemode_init do; no global read leads a song's scripts to it), or enable_crowd_models_cfunc (sub_82231870)
//   forced inactive at 0x82231A54, which only runs when a camera cut asks for it (CameraCuts_StartCallback).
// - Whammy sound effect (GH Warriors of Rock's local_docs/50): sub_821E92B0, the whammy bar's work on a held sustain
//   (called by the sustain's sub_821E9FA8 and by sub_821D80F8; GH Warriors of Rock's sub_8218C8A8), gives the song
//   player the bend at 0x821E94D8 (bl sub_82166460, f1 from a fnmsubs of the bar's value, 0.057 at 0x82003C48).
//   sub_82166460 keeps it at +28 of the player's entry and sets the stem stream's pitch ([stem + 300]) to the lowest of
//   its players' ([stem + 324]; from 1.0, 0x82000B58) through sub_823CAE90 -> sub_8232EB08. Off: f1 = 1.0 there, the
//   pitch unbent; in GH Warriors of Rock the bar still wobbled the sustain on the highway and filled Star Power (other
//   code). A remote online player's bend (SongAudioSetWhammy, sub_82163C08 -> sub_82166460) is left alone.
// - Instrument volume on a miss (GH Warriors of Rock's local_docs/47, 50): GuitarEvent_MissedNote and
//   GuitarEvent_UnnecessaryNote (scripts\guitar\guitar_events.qb) call PlayerSetVolume volume = 0 and UpdateGuitarVolume
//   (sub_82237AB0, its per-player body sub_82233B18), which calls sub_82162F10 (through the thunk sub_82166430: r3 +
//   1564, r4 - 1) with the song player's stream state (the static object 0x82CC8440 + 1564), the player - 1 and the flag
//   [player + 532] > 0 (guitar_volume). The player's stream entry is at the stream state + 2948 + player x 40 (+0 its
//   type, 1 guitar or 2 bass; +8 the flag the last call stored; +32 whether it has several streams). Its three ways:
//   one stream (0x82162FF8: bl sub_82168F18 with f1 = 1.0 or 0.0, the time [state + 2936] up or [+2940] down,
//   instrument_missed_note_ramp_up_time 0 and _down_time 0.02 s), several (0x82163234: sub_821618F0's target stored at
//   +12 of [entry + 16]; 1.084 or 0 in GH Warriors of Rock's Quest song), or, when two or more players play the same
//   part (sub_82162B40 sets bit 1, 2 or 4 of [state + 2932]), a crossfade table of their states. This game's function
//   keeps the stream state in r29 where GH Warriors of Rock's kept it in r30, and its offsets are 208 lower (3156, 3140,
//   3144, 3148 there). Other scripts mute with PlayerSetVolume too (Freeplay's players sitting out, practice, the first
//   note's window), so a miss is known by the thread's last "$" reader, the miss's script (its $is_network_game and
//   $single_stream_enabled just before; nothing between reads a global), and by the entry's flag stored on. Then the
//   flag is passed on (the "on" target) and the target times the percentage at the two stores, with the down ramp's
//   time. Two players on the same part: only 100 % (the flag on). The drums mute through SetAllDrumStates: not changed.
//   This game only: with $single_stream_enabled = 1 (the single-stream "mp3" songs, begin_mp3_song) a miss calls
//   musicstudio_update_volume_for_playback instead, and nothing here applies.
// - Dolby Digital timing: setlagoffsets (scripts\guitar\guitar_gems.qb, from call_startup_scripts) adds
//   default_lag_settings, then default_lag_settings_dolbydigital when IsDolbyDigitalEnabled; on xenon -45 / -61 / +53
//   and -30 / -30 / -30 ms (gem, input, drum MIDI), GH Warriors of Rock's values to the digit. The base takes the Dolby
//   offsets and the Dolby set adds nothing: -75 / -91 / +23 whatever the speakers. It matters only with the SDK's
//   audio_speaker_config_from_output = true (src/startup.cpp); with it false (the SDK's default) the game is always told
//   Dolby Digital and already gets these.

#include "mods.h"

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

#include "highway_images.h"
#include "script_globals.h"

REXCVAR_DEFINE_BOOL(ghbandhero_dolby_timing, true, "GHBandHero",
                    "The notes', the input's and the drums' timing with the game's Dolby Digital offsets (-30 ms each) "
                    "whatever speakers it is told (audio_speaker_config_from_output): the timing every player had "
                    "before, so no one calibrates again; false = a console's, -30 ms only with 5.1");
REXCVAR_DEFINE_INT32(ghbandhero_highway_scale, 100, "GHBandHero",
                     "The note highway's size with one player, in percent (80-120 in the launcher; it shrinks and grows "
                     "from the strike line); 100 = as on the console");
REXCVAR_DEFINE_INT32(ghbandhero_highway_length, 100, "GHBandHero",
                     "The note highway's length in time, in percent (100, 125, 150 or 170): the notes seen that much "
                     "earlier, the console's highway continued in its own perspective, at the same speed at the strike "
                     "line; 100 = as on the console");
REXCVAR_DEFINE_INT32(ghbandhero_highway_perspective, 100, "GHBandHero",
                     "The note highway's perspective, in percent of the console's (25-150 in the launcher): below 100 "
                     "flatter, as seen from higher, above 100 steeper; 100 = as on the console");
REXCVAR_DEFINE_BOOL(ghbandhero_highway_keep_speed, false, "GHBandHero",
                    "With a highway perspective other than 100, the notes reach the strike line at the console's speed: "
                    "the times on the highway multiplied by the perspective's speed there (shorter below 100, longer "
                    "above); false = the console's times, the notes slower near the strike line below 100 and faster "
                    "above");
REXCVAR_DEFINE_STRING(ghbandhero_highway_style, "original", "GHBandHero",
                      "The note highway's look: original, or dark (the community's black highway)");
REXCVAR_DEFINE_INT32(ghbandhero_highway_opacity, 100, "GHBandHero",
                     "The note highway's opacity in percent: below 100 the venue shows through it (the game's cut-out of "
                     "the venue under the highway left out); 100 = as on the console");
REXCVAR_DEFINE_BOOL(ghbandhero_whammy_sound, true, "GHBandHero",
                    "The whammy bar bends a held sustain's pitch; false keeps the pitch (the bar still wobbles the note "
                    "and fills Star Power). true = as on the console");
REXCVAR_DEFINE_BOOL(ghbandhero_miss_lower, true, "GHBandHero",
                    "Your instrument's track lowered on a miss or a strum without a note, to ghbandhero_miss_volume; false: "
                    "never lowered. true = as on the console");
REXCVAR_DEFINE_INT32(ghbandhero_miss_volume, 0, "GHBandHero",
                     "How much of your instrument's track stays when you miss a note or strum without one, in percent "
                     "(0-100 by 10 in the launcher; guitar and bass): 0 = muted until the next hit, as on the console; "
                     "100 = never muted");

namespace ghbandhero::mods {
namespace {

// The console's highway (guitar_tweaks.qb's highway_guitar1, freeplay_data.qb's highway_scale_chart), as floats.
constexpr double kPlayline = 655.0;
constexpr double kHeight = 350.0;
constexpr double kTopWidth = 160.0;
constexpr double kBottomWidth = 512.0;  // 160 x (1 + 2.2)
constexpr int kRows = 1024;             // real_highway_lines
constexpr double kHighestTop = 50.0;    // the far end no higher than this (of 720)

struct Projection {
  double f;  // perspectivefact
  double e;  // perspectiveexp
};

double AsFloat(double v) { return double(float(v)); }

// highway_scale_chart's rows: one to four players' projections and the bottom's narrowing.
const Projection kChart[4] = {{AsFloat(1.00108), AsFloat(1.00125)},
                              {AsFloat(1.00158), AsFloat(1.0005)},
                              {AsFloat(1.00168), AsFloat(1.00027)},
                              {AsFloat(1.00144), AsFloat(1.00028)}};
const double kChartOffset[4] = {0.0, -0.4, -0.5, -0.9};

double Exponent(const Projection& p) { return p.e * std::log(p.f) / (p.e - 1.0); }

// The projection with its compression times `perspective` and its time window times `length`.
Projection Transform(const Projection& in, double perspective, double length) {
  const double a = Exponent(in) * perspective * std::pow(in.e, kRows * (1.0 - length));
  const double e = std::pow(in.e, length);
  return {std::exp(a * (e - 1.0) / e), e};
}

// The place from the far end (0) to the strike line (1) of a fractional row, as GeneratePosTable makes them.
double Place(const Projection& p, double row) {
  std::vector<double> h(kRows + 1);
  h[0] = 1.0;
  for (int i = 1; i <= kRows; ++i) {
    h[i] = std::pow(h[i - 1] * p.f, p.e);
  }
  double sum = 0.0;
  for (int i = 0; i < kRows; ++i) {
    sum += h[i];
  }
  const int whole = std::clamp(int(std::floor(row)), 0, kRows - 1);
  double place = 0.0;
  for (int i = 0; i < whole; ++i) {
    place += h[i];
  }
  place += h[whole] * (row - whole);
  return place / sum;
}

// The notes' speed in pixels at the strike line with the perspective p, against the console's: the last row's share of
// the highway (the same for every length: the length continues the projection beyond the far end).
double StrikeLineSpeed(double p) {
  const auto last_row = [](const Projection& projection) { return 1.0 - Place(projection, kRows - 1); };
  return last_row(Transform(kChart[0], p, 1.0)) / last_row(kChart[0]);
}

struct Shape {
  Projection one_player;  // highway_guitar1's and the chart's first row
  double height;          // highway_height
  double top_width;       // highway_top_width
};

// The highway for a perspective p and a length k (1, 1 = the console's).
Shape Make(double p, double k) {
  Shape out;
  out.one_player = Transform(kChart[0], p, k);
  // The console's far end, at the time T, is the new row 1024 (1 - 1/k); it stays at y = 305.
  out.height = k == 1.0 ? kHeight : kHeight / (1.0 - Place(out.one_player, kRows * (1.0 - 1.0 / k)));
  // The top width a camera gives with this compression, at the console's far end (the width goes with the square root of
  // the rows' heights: the console's near rows are R = exp(A (e^1024 - 1)) = 9.4 times its far ones, its top 160 =
  // 512 / 3.2, sqrt(R) = 3.06); beyond it on the same side lines.
  const double sqrt_r = std::exp(Exponent(kChart[0]) * (std::pow(kChart[0].e, kRows) - 1.0) / 2.0);
  const double top_at_305 = std::min(kBottomWidth, kTopWidth * std::pow(sqrt_r, 1.0 - p));
  out.top_width = top_at_305 + (kBottomWidth - top_at_305) * (kHeight - out.height) / kHeight;
  return out;
}

Highway& TheHighway() {
  static Highway highway;
  return highway;
}

}  // namespace

const Highway& OnePlayerHighway() { return TheHighway(); }

void Configure() {
  static std::once_flag once;
  std::call_once(once, [] {
    std::string summary;
    const int32_t scale = std::clamp(REXCVAR_GET(ghbandhero_highway_scale), 50, 150);
    TheHighway().scale = scale / 100.0;
    if (scale != 100) {
      script::Set("highway_scale_chart[0].hw_x_scale", scale / 100.0);
      script::Set("highway_scale_chart[0].hw_y_scale", scale / 100.0);
      summary += " highway scale " + std::to_string(scale) + " %";
    }

    const int32_t perspective = std::clamp(REXCVAR_GET(ghbandhero_highway_perspective), 10, 200);
    const int32_t asked = std::clamp(REXCVAR_GET(ghbandhero_highway_length), 100, 300);
    // The length that fits: the far end, with the one-player scale, no higher than y = 50.
    int32_t length = asked;
    const double room = (kPlayline - kHighestTop) / std::max(1.0, scale / 100.0);
    while (length > 100 && Make(perspective / 100.0, length / 100.0).height > room) {
      length -= 5;
    }
    if (length != asked) {
      REXLOG_INFO("[mods] highway length {} % with perspective {} % and scale {} %: {} %, the most that fits", asked,
                  perspective, scale, length);
    }
    if (perspective != 100 || length != 100) {
      const double p = perspective / 100.0;
      const double k = length / 100.0;
      const Shape highway = Make(p, k);
      TheHighway().height = std::round(highway.height);
      TheHighway().top_width = highway.top_width;
      const double c = highway.top_width / kTopWidth;  // the far end's sizes
      const double width_offset = kBottomWidth / highway.top_width - 1.0;
      script::Set("highway_guitar1.highway_height", std::round(highway.height));
      script::Set("highway_guitar1.perspectivefact", highway.one_player.f);
      script::Set("highway_guitar1.perspectiveexp", highway.one_player.e);
      script::Set("highway_guitar1.highway_top_width", highway.top_width);
      script::Set("highway_guitar1.widthoffsetfactor", width_offset);
      script::Set("highway_guitar1.whammy_width_offset", width_offset);
      script::Scale("highway_guitar1.gem_start_scale[*]", c);
      script::Scale("highway_guitar1.fretbar_start_scale[0]", c);
      script::Scale("highway_guitar1.fretbar_start_scale[1]", c);
      script::Scale("highway_guitar1.whammy_top_width", c);
      // The sidebars' sprites lie along the slanted sides (their angle generate_pos_table's own): their length with
      // the side's (the console's sqrt(350^2 + 176^2) = 392), not with the height (in GH Warriors of Rock at 25 %
      // perspective the sides were almost upright and the height's ratio left them 60 px past the far end).
      const auto side = [](double height, double top) { return std::hypot(height, (kBottomWidth - top) / 2.0); };
      script::Scale("highway_guitar1.sidebar_y_scale",
                    side(highway.height, highway.top_width) / side(kHeight, kTopWidth));
      script::Scale("highway_guitar1.ghighwaytiling", k);
      for (int row = 0; row < 4; ++row) {
        const Projection projection = Transform(kChart[row], p, k);
        const std::string at = "highway_scale_chart[" + std::to_string(row) + "].";
        script::Set(at + "perspectivefact", projection.f);
        script::Set(at + "perspectiveexp", projection.e);
        if (row > 0) {
          script::Set(at + "extra_offset_factor", kChartOffset[row] / c);
        }
      }
      // The times on the highway: the length's, and with ghbandhero_highway_keep_speed the perspective's speed.
      const double speed = perspective != 100 ? StrikeLineSpeed(p) : 1.0;
      const bool keep_speed = perspective != 100 && REXCVAR_GET(ghbandhero_highway_keep_speed);
      const double time = k * (keep_speed ? speed : 1.0);
      if (time != 1.0) {
        script::Scale("difficulty_list_props.*.scroll_time", time);
        script::Scale("destroy_time", time);
      }
      if (perspective != 100) {
        summary += " highway perspective " + std::to_string(perspective) + " %";
        // Expert's 2.5 s scroll time less the 1 s destroy time, without Hyperspeed.
        const double expert = 1.5 * time;
        if (keep_speed) {
          summary += " (notes at the console's speed)";
          REXLOG_INFO("[mods] highway perspective {} %: the notes reach the strike line at the console's speed "
                      "(ghbandhero_highway_keep_speed: the times on the highway x {:.3f}, {:.2f} s on Expert instead of "
                      "{:.2f})",
                      perspective, speed, expert, 1.5 * k);
        } else {
          REXLOG_INFO("[mods] highway perspective {} %: the notes reach the strike line at {:.2f} of the console's "
                      "speed ({:.2f} s on Expert; ghbandhero_highway_keep_speed off)",
                      perspective, speed, expert);
        }
      }
      if (length != 100) {
        summary += " highway length " + std::to_string(length) + " %";
      }
      REXLOG_INFO("[mods] the highway: {:.0f} high (far end at y = {:.0f}), {:.0f} wide at the top, perspective {:.7f} "
                  "{:.7f}",
                  highway.height, kPlayline - highway.height, highway.top_width, highway.one_player.f,
                  highway.one_player.e);
    }

    const std::string style = REXCVAR_GET(ghbandhero_highway_style);
    if (style == "dark") {
      for (int i = 0; i < 3; ++i) {
        script::Set("highway_normal[" + std::to_string(i) + "]", 0);
      }
      script::Set("highway_starpower[1]", 0);
      script::Set("highway_starpower[2]", 0);
      summary += " highway style dark";
    } else if (style != "original" && !style.empty()) {
      REXLOG_WARN("[mods] ghbandhero_highway_style \"{}\": not a style (original, dark); the original", style);
    }
    // The highway's opacity: the alpha of its colors (the normal one, Star Power's after a miss, a miss's flash), and
    // the game's cut-out of the venue under it left out (GHBandHeroHighwayPrepassHook).
    const int32_t opacity = std::clamp(REXCVAR_GET(ghbandhero_highway_opacity), 0, 100);
    if (opacity < 100) {
      const double alpha = std::round(255.0 * opacity / 100.0);
      script::Set("highway_normal[3]", alpha);
      script::Set("highway_starpower[3]", alpha);
      script::Set("highway_pulse[3]", alpha);
      summary += " highway opacity " + std::to_string(opacity) + " %";
    }
    if (!REXCVAR_GET(ghbandhero_whammy_sound)) {
      summary += " whammy sound off";
    }
    const int32_t miss = REXCVAR_GET(ghbandhero_miss_lower) ? std::clamp(REXCVAR_GET(ghbandhero_miss_volume), 0, 100) : 100;
    if (miss > 0) {
      script::TrackReaders();
      summary += " instrument on a miss " + std::to_string(miss) + " %";
    }
    if (!summary.empty()) {
      REXLOG_INFO("[mods]{}", summary);
    }
    // The game's timing with Dolby Digital whatever speakers it is told (Gustavo, 2026-11-18, for GH Warriors of Rock:
    // "siempre sí lo quiero"). The base takes the Dolby offsets and the Dolby set adds nothing: -75 / -91 / +23 with
    // 5.1, stereo and mono alike, the timing every player had while the SDK always answered Dolby Digital
    // (GH Warriors of Rock's docs/speakers-and-dolby.md).
    if (REXCVAR_GET(ghbandhero_dolby_timing)) {
      script::Set("default_lag_settings.xenon.gem_lag_ms", -45 - 30);
      script::Set("default_lag_settings.xenon.input_lag_ms", -61 - 30);
      script::Set("default_lag_settings.xenon.drum_midi_lag_ms", 53 - 30);
      script::Set("default_lag_settings_dolbydigital.xenon.gem_lag_ms", 0);
      script::Set("default_lag_settings_dolbydigital.xenon.input_lag_ms", 0);
      script::Set("default_lag_settings_dolbydigital.xenon.drum_midi_lag_ms", 0);
      REXLOG_INFO("[speakers] the Dolby Digital timing (-30 ms) whatever the speakers (ghbandhero_dolby_timing)");
    }
    highway::Configure();  // the highway's picture (src/highway_images.cpp)
  });
}

}  // namespace ghbandhero::mods

namespace {

bool WhammySound() {
  static const bool on = REXCVAR_GET(ghbandhero_whammy_sound);
  return on;
}

// The launcher's switch off: never lowered, as at 100 %.
int32_t MissVolume() {
  static const int32_t volume =
      REXCVAR_GET(ghbandhero_miss_lower) ? std::clamp(REXCVAR_GET(ghbandhero_miss_volume), 0, 100) : 100;
  return volume;
}

uint8_t* GuestBase() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

uint32_t Load32(const uint8_t* base, uint32_t address) {
  uint32_t value;
  std::memcpy(&value, base + address, 4);
  return __builtin_bswap32(value);
}

float LoadFloat(const uint8_t* base, uint32_t address) {
  const uint32_t bits = Load32(base, address);
  float value;
  std::memcpy(&value, &bits, 4);
  return value;
}

constexpr uint32_t kMissedNote = 0x8D14D919;       // GuitarEvent_MissedNote
constexpr uint32_t kUnnecessaryNote = 0x1D649F39;  // GuitarEvent_UnnecessaryNote
constexpr uint32_t kStreamEntries = 2948;          // the players' stream entries in the stream state, 40 bytes each
constexpr uint32_t kSharedParts = 2932;            // bit 1 / 2 / 4: two or more players on that part
constexpr uint32_t kRampDownTime = 2940;           // instrument_missed_note_ramp_down_time

// The part of the "on" target a miss leaves in this call (1 = no change), set at the ramp's entry for its stores.
thread_local double t_miss_part = 1.0;

std::atomic<int> g_whammy_logged{0};
std::atomic<int> g_miss_logged{0};

// The target times the part a miss leaves, ramped down in the console's time (the "on" target would ramp up in 0 s).
// `state` = the song player's stream state (r29 at both stores).
void MissVolumeStore(PPCRegister& state, PPCRegister& f1, PPCRegister& f31) {
  if (t_miss_part >= 1.0) {
    return;
  }
  if (uint8_t* base = GuestBase()) {
    f31.f64 = LoadFloat(base, state.u32 + kRampDownTime);
  }
  if (g_miss_logged.load() <= 4) {
    REXLOG_INFO("[mods] a miss's target {:.3f} -> {:.3f} in {:.3f} s", f1.f64, f1.f64 * t_miss_part, f31.f64);
  }
  f1.f64 *= t_miss_part;
}

}  // namespace

// [[midasm_hook]] 0x8259B4C4 (sub_8259B408, a window element made: after it read viewportrenderpre into +604; GH
// Warriors of Rock's 0x82581C7C, the same function instruction for instruction): r30 = the element. Only
// create_highway_prepass's windows set it (the one script that names it): a highway-shaped sprite drawn into
// bg_viewport before the venue, so the venue is never drawn under the highway (black behind a see-through one). The flag
// is passed on to the window's render node (sub_8259B5D0 writes it, sub_824CA2E8 and sub_824C44A8 read it). With the
// highway's opacity below 100 the flag is cleared: the window is an ordinary one of bg_viewport.
void GHBandHeroHighwayPrepassHook(PPCRegister& r30) {
  static const bool see_through = std::clamp(REXCVAR_GET(ghbandhero_highway_opacity), 0, 100) < 100;
  if (!see_through) {
    return;
  }
  if (uint8_t* base = GuestBase(); base && base[r30.u32 + 604]) {
    base[r30.u32 + 604] = 0;
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) {
      REXLOG_INFO("[mods] the highway's cut-out of the venue left out (ghbandhero_highway_opacity)");
    }
  }
}

// [[midasm_hook]] 0x821E94D8 (sub_821E92B0, bl sub_82166460: the local player's whammy bend): f1 = the pitch.
void GHBandHeroWhammySoundHook(PPCRegister& f1) {
  if (WhammySound()) {
    return;
  }
  if (f1.f64 != 1.0 && g_whammy_logged.fetch_add(1) < 2) {
    REXLOG_INFO("[mods] the whammy's bend {:.3f} held at 1 (ghbandhero_whammy_sound off)", f1.f64);
  }
  f1.f64 = 1.0;
}

// [[midasm_hook]] 0x82162F10 (the entry of sub_82162F10, the instrument's volume ramp): r3 = the song player's stream
// state (0x82CC8440 + 1564), r4 = the player - 1, r5 = the flag (the player's guitar_volume > 0).
void GHBandHeroMissVolumeHook(PPCRegister& r3, PPCRegister& r4, PPCRegister& r5) {
  t_miss_part = 1.0;
  const int32_t volume = MissVolume();
  if (volume <= 0 || (r5.u32 & 0xFF) != 0 || r4.u32 >= 4) {
    return;
  }
  const uint32_t reader = ghbandhero::script::LastReader();
  if (reader != kMissedNote && reader != kUnnecessaryNote) {
    return;
  }
  uint8_t* base = GuestBase();
  if (!base) {
    return;
  }
  const uint32_t entry = r3.u32 + kStreamEntries + r4.u32 * 40;
  const uint32_t type = Load32(base, entry);
  // Guitar or bass (the function's own test), and on until now: the game's own mute (a Freeplay player sitting out) is
  // not a miss.
  if ((type != 1 && type != 2) || base[entry + 8] == 0) {
    return;
  }
  const bool shared = base[entry + 32] != 0 && (Load32(base, r3.u32 + kSharedParts) & type) != 0;
  if (shared && volume < 100) {
    return;
  }
  r5.u64 = 1;
  t_miss_part = volume / 100.0;
  if (g_miss_logged.fetch_add(1) < 4) {
    REXLOG_INFO("[mods] a miss (script {:08X}, player {}): the instrument kept at {} %", reader, r4.u32 + 1, volume);
  }
}

// [[midasm_hook]] 0x82162FF8 (sub_82162F10's one stream: mr r6,r30 before bl sub_82168F18; a branch target): r29 = the
// stream state (r30 there is the time now), f1 = the target, f31 = the ramp's time.
void GHBandHeroMissVolumeSingleHook(PPCRegister& r29, PPCRegister& f1, PPCRegister& f31) { MissVolumeStore(r29, f1, f31); }

// [[midasm_hook]] 0x82163234 (sub_82162F10's several streams: before the target is stored at +12 of [entry + 16]; a
// branch target): the same registers.
void GHBandHeroMissVolumeMultiHook(PPCRegister& r29, PPCRegister& f1, PPCRegister& f31) { MissVolumeStore(r29, f1, f31); }
