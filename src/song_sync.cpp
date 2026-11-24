// ghbandhero - a song's music started with its notes (ghbandhero_song_start_sync; off: the game's own arithmetic). GH Warriors of
// Rock's src/song_sync.cpp (its docs/song-sync.md, local_docs/61-63), on Band Hero's code.
//
// The notes follow the song's clock, a double in seconds at 0x82D81898, and Band Hero's second clock at +8 (src/song_clock.cpp):
// ResetSongTimer (sub_823EB5A8, through the script reset_song_time, which divides its milliseconds by 1000) sets both,
// and the main thread adds each frame's vertical blanks to them. The music is a stream of the song player: the song's
// load script (0x94323F7F) loads it (SongLoad, sub_82164320) and, for a song started later than its beginning (a
// practice section), seeks it there (SongSeek, sub_82164678); the script begin_song_after_intro (0x647F97EE: "GetSongTimeMs,
// if time >= starttimeafterintro break, wait 1 gameframe", then 0x81872036, which calls SongPlay, sub_82163A38) plays
// it in its first pass with the clock at the song's start or past it. From then on clock and music run together, so the
// clock must be before the start when the stream waits: the music then starts within a pass of it.
//
// A song that follows another without the menus (start_gem_scroller with a loading transition) resets the clock, after
// the stream was loaded, to 0 - transition_time - 2 x transition_lag_offset, where transition_lag_offset = 0 -
// video_offset - audio_offset (the calibration's): Band Hero's start_gem_scroller has GH Warriors of Rock's arithmetic word
// for word. There the Rush chapter's parts 2 to 7 (a 100 ms intro) started the clock past 0 with an audio calibration
// more than 50 ms above the video's, and the notes stayed 172 ms (0 / 120) or 333 ms (0 / 200) ahead of the music
// until a pause (PauseAudio seeks the stream to the clock). Any Band Hero transition shorter than the calibration does the
// same; the rules below act only then.
//
// With ghbandhero_song_start_sync (on):
// - A stream "waits" from a SongLoad that loaded it to SongPlay, or to SongStop, SongUnload, SongAudioPlayerDeInit or
//   SongUnPause. Its start is 0, or the time of a SongSeek made while it waits.
// - Rule A: a ResetSongTimer while a stream waits, to a time later than 50 ms before its start (and less than 0.45 s
//   after it: the transitions' arithmetic gives at most +0.4 s), sets both clocks 100 ms before the start instead. The
//   game's poll then starts the music as in any song. A moved reset cannot put the music out of step: the poll starts
//   it at the song's start whatever the clock began at, so the worst a needless move does is a longer wait. The upper
//   bound is for practice, which resets its clock before the load seeks the stream, while the start is still 0.
// - Rule B, for any other path and for a hitch between the reset and the poll: at SongPlay, while a stream waits, a
//   clock more than 30 ms past its start is set to it (both clocks). A start without a hitch is within a frame of it
//   (16.7 ms at 60 FPS), so it is left as the game made it.
// - begin_song_after_intro runs every frame above 60 FPS (src/framerate.cpp), so the music starts within a frame of the
//   song's start (6.25 ms at 160 FPS), not within a 60th of a second.
// Every song's start is logged with or without the option ("[song sync]": the clock against the stream's start at
// SongPlay, and what the rules did), and its end with the clock's report (src/song_clock.cpp). docs/song-sync.md.

#include "framerate.h"
#include "song_clock.h"

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>

REXCVAR_DEFINE_BOOL(ghbandhero_song_start_sync, true, "GHBandHero",
                    "A song's music started with its notes: a song that follows another no longer starts its clock "
                    "past the music's start with an audio calibration larger than the video's (the notes then stayed "
                    "ahead of the music until a pause), and above 60 FPS the music starts within a frame of its time. "
                    "false = the game's own arithmetic");

namespace {

// The song's clocks: doubles, seconds.
constexpr uint32_t kSongClock = 0x82D81898;
constexpr uint32_t kSongClock2 = kSongClock + 8;

// Rule A: a reset later than the start less kResetEarly, and earlier than the start plus kResetFar, is moved to the
// start less kResetBefore.
constexpr double kResetEarly = 0.050;
constexpr double kResetBefore = 0.100;
constexpr double kResetFar = 0.450;
// Rule B: a clock more than this past the start at SongPlay.
constexpr double kPlayLate = 0.030;

struct WaitingStream {
  bool waiting = false;
  double start = 0.0;  // seconds
  bool reset_moved = false;
  double reset_from = 0.0;
};

std::mutex g_mutex;
WaitingStream g_stream;

uint8_t* GuestBase() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

double LoadDouble(const uint8_t* base, uint32_t address) {
  uint64_t bits;
  std::memcpy(&bits, base + address, 8);
  bits = __builtin_bswap64(bits);
  double value;
  std::memcpy(&value, &bits, 8);
  return value;
}

void StoreDouble(uint8_t* base, uint32_t address, double value) {
  uint64_t bits;
  std::memcpy(&bits, &value, 8);
  bits = __builtin_bswap64(bits);
  std::memcpy(base + address, &bits, 8);
}

void EndWaiting() {
  std::lock_guard lock(g_mutex);
  g_stream.waiting = false;
}

// The music no longer plays: the song's reports end.
void SongEnded() {
  EndWaiting();
  ghbandhero::SongClockSongEnded();
  ghbandhero::FrameBlanksSongEnded();
}

}  // namespace

// [[midasm_hook]] 0x821643C8 (SongLoad, sub_82164320: after "bl 0x82166038", the song player's load): r3 = loaded.
void GHBandHeroSongLoadHook(PPCRegister& r3) {
  std::lock_guard lock(g_mutex);
  g_stream = {};
  g_stream.waiting = (r3.u32 & 0xFF) != 0;
}

// [[midasm_hook]] 0x82164724 (SongSeek, sub_82164678: "bl 0x821673D0", the song player's seek): r4 = the time, ms.
void GHBandHeroSongSeekHook(PPCRegister& r4) {
  std::lock_guard lock(g_mutex);
  if (g_stream.waiting) {
    g_stream.start = double(r4.u32) / 1000.0;
  }
}

// [[midasm_hook]] 0x823EB5E4 (ResetSongTimer, sub_823EB5A8: "addi r31,r10,6296", before f0, the new time in seconds,
// is stored to both clocks). Rule A.
void GHBandHeroSongResetHook(PPCRegister& f0) {
  if (!REXCVAR_GET(ghbandhero_song_start_sync)) {
    return;
  }
  std::lock_guard lock(g_mutex);
  const double time = f0.f64;
  if (!g_stream.waiting || !(time > g_stream.start - kResetEarly) || !(time < g_stream.start + kResetFar)) {
    return;
  }
  g_stream.reset_moved = true;
  g_stream.reset_from = time;
  f0.f64 = g_stream.start - kResetBefore;
  REXLOG_INFO("[song sync] the song's clock reset to {:+.1f} ms, past the music's start at {:.3f} s while it waits: "
              "set to {:+.1f} ms (ghbandhero_song_start_sync)",
              (time - g_stream.start) * 1000.0, g_stream.start, -kResetBefore * 1000.0);
}

// [[midasm_hook]] 0x82163A38 (the entry of SongPlay, sub_82163A38: the song's stream starts). Rule B and the song's log
// line.
void GHBandHeroSongPlayHook() {
  WaitingStream stream;
  {
    std::lock_guard lock(g_mutex);
    stream = g_stream;
    g_stream.waiting = false;
  }
  if (!stream.waiting) {
    return;
  }
  ghbandhero::SongClockSongStarted();
  uint8_t* base = GuestBase();
  if (!base) {
    return;
  }
  const bool on = REXCVAR_GET(ghbandhero_song_start_sync);
  const double late = LoadDouble(base, kSongClock) - stream.start;
  const bool set_back = on && late > kPlayLate;
  if (set_back) {
    StoreDouble(base, kSongClock, stream.start);
    StoreDouble(base, kSongClock2, stream.start);
  }
  REXLOG_INFO("[song sync] the music starts from {:.3f} s with the notes' clock {:+.1f} ms from it{}{}{}",
              stream.start, late * 1000.0,
              stream.reset_moved ? fmt::format("; its reset to {:+.1f} ms was moved (rule A)",
                                               (stream.reset_from - stream.start) * 1000.0)
                                 : std::string(),
              set_back ? "; the clock set back to the start (rule B)" : "",
              on ? "" : " (ghbandhero_song_start_sync off)");
}

// The stream no longer waits: stopped, unloaded, the song player shut down, or played by unpausing it. The music's
// stop also ends the song's clock report and the count of its frames by the blanks they took (src/framerate.cpp).
// [[midasm_hook]] 0x82163A80 (the entry of SongStop, sub_82163A80).
void GHBandHeroSongStopHook() { SongEnded(); }

// [[midasm_hook]] 0x82164408 (the entry of SongUnload, sub_82164408).
void GHBandHeroSongUnloadHook() { SongEnded(); }

// [[midasm_hook]] 0x821645D8 (the entry of SongAudioPlayerDeInit, sub_821645D8).
void GHBandHeroSongDeInitHook() { SongEnded(); }

// [[midasm_hook]] 0x82163B94 (SongUnPause, sub_82163B20: "bl 0x821672B0", the stream unpaused).
void GHBandHeroSongUnPauseHook() { EndWaiting(); }
