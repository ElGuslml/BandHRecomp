// ghbandhero - the boot's intro video ended as soon as the game has loaded: Party Play right after the loading.
//
// GH Metallica's, GH Van Halen's and GH Smash Hits' boot is a chain of UI states (ui_boot.qb: a legal notice, then a
// video per state, PlayMovieAndWait each), and their fix makes the script function PlayMovie find none of those
// videos. Band Hero's boot is GH5's (read from the decoded scripts): guitar_startup_spawned (scripts\guitar\guitar.qb)
// plays one video, GH5_1_Logo_Movie (data\movies\gh5_1_logo_movie.bik.xen: 663 frames at 29.97 fps, 22.1 s, the same
// video as moviestreams\GH5_1_Logo_Movie.bik; GH5's is Intro_FINALanimatic_0209), as its loading screen
// (create_loading_screen {is_bink ...}), loads the game behind it, then calls destroy_loading_screen {wait_for_unload,
// wait_for_end}. With wait_for_end (passed by the boot only) destroy_loading_screen_spawned
// (scripts\guitar\menu\loading_screen.qb, GH5's byte for byte) shows a button helper and waits while the video plays:
//   begin  if NOT IsMoviePlaying ... break  if ($end_movie_early = 1) break  wait 1 gameframe  repeat
// the helper's handler (LoadingEndMovie, on Start or the green fret) setting end_movie_early. Then Party Play
// (UIstate_freeplay). There is no legal notice to wait for (no script plays another video at the boot: PlayMovie is
// called by PlayMovieAndWait, for the career's and the bonus videos, create_loading_screen_spawned and
// goal_presentation_movie_onstart only).
//
// The video cannot be refused as in those games: create_loading_screen_spawned starts it and then waits until it is
// playing (begin if IsMoviePlaying break ... repeat) before the startup goes on, so a video not found would stop the
// boot. Instead end_movie_early reads as 1 in destroy_loading_screen_spawned, as if the player pressed Start the
// moment the game had loaded: the video ends there and then, and Party Play follows. The global itself keeps its
// value. The read goes through src/debug_menu.cpp's hooks on the script interpreter (the "$name" resolve, one hook per
// address); after the first such read they are left alone again (unless the debug menu is on). If the video ends
// before the game has loaded (a slow disk), the loop ends on IsMoviePlaying and nothing changes. GH5's fix
// (GH5Recomp's src/intro.cpp), the same scripts and keys here.

#include "intro.h"

#include <rex/cvar.h>
#include <rex/logging.h>

#include <atomic>
#include <cstdint>

REXCVAR_DEFINE_BOOL(ghbandhero_skip_intro, false, "GHBandHero",
                    "End the intro video at the game's start (played while the game loads) as soon as the game has "
                    "loaded, as if Start were pressed then: Party Play right after the loading; false = as on the "
                    "console");

namespace {

constexpr uint32_t kDestroyLoadingScreenSpawned = 0x9F597079;  // destroy_loading_screen_spawned
constexpr uint32_t kEndMovieEarly = 0x283DCD57;                // the script global end_movie_early

std::atomic<bool> g_done{false};

bool Enabled() {
  static const bool enabled = REXCVAR_GET(ghbandhero_skip_intro);
  return enabled;
}

}  // namespace

namespace ghbandhero {

bool IntroSkipPending() { return Enabled() && !g_done.load(std::memory_order_relaxed); }

bool IntroReadsAsOne(uint32_t script_name, uint32_t global) {
  if (global != kEndMovieEarly || script_name != kDestroyLoadingScreenSpawned || !Enabled()) {
    return false;
  }
  if (!g_done.exchange(true)) {
    REXLOG_INFO("[intro] the game has loaded: the intro video ends (end_movie_early read as 1)");
  }
  return true;
}

}  // namespace ghbandhero
