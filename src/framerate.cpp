// ghbandhero - frame rates above 60: the emulated vertical blank at the chosen rate.
//
// No patch for a higher frame rate exists for any Guitar Hero (Xenia Canary's game patches, RPCS3's patches,
// 2026-10-25): they run at 60 on the consoles. The game's own pacing (sub_8254FB40, the render thread presenting a
// frame) is all vertical blanks:
// - one frame in flight: it waits until the swap counter 0x82D7DFE0 (sub_823B09C8; the swap callback sub_823B07C0
//   adds one when a frame is shown) passes the previous frame's;
// - then the present interval, from the global 0x82B34354 (1 in the image, written by no code): 1 = every vertical
//   blank (the vblank callback sub_823B0748 counts them at 0x82D7DFD0), 2 = every second one, 0 = immediate.
// The engine's clock is real time: the main thread measures each frame with the time base (sub_82414058) and the
// game scales by the frame's duration in seconds. So with the SDK's emulated vblank at a higher rate
// (video_mode_refresh_rate, which src/options.cpp sets from ghbandhero_frame_rate) the game presents at that rate
// and keeps its speed (GH Metallica, the same engine: at 120 Hz it ran at 120 FPS, 8.33 ms per frame in a song, and the
// career's first song took as long from its start to the bot's failure as at 60, 74.5 s and 75.8 s; with vsync off,
// the vblank every millisecond, about 660 FPS in the menus: the game has no cap of its own).
//
// The waits of src/thread_waits.cpp follow the vblank's rate (VblankPeriod).

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iterator>
#include <mutex>
#include <unordered_map>

REXCVAR_DEFINE_INT32(ghbandhero_frame_rate, 60, "GHBandHero",
                     "Frame rate: 60 as on the console, or 120, 144, 160, 165 or 240: the emulated vertical blank runs "
                     "at that rate (video_mode_refresh_rate, set at startup) and the game presents on every one; its "
                     "clock is real time, so it keeps its speed");

REXCVAR_DEFINE_BOOL(ghbandhero_frame_rate_fix, true, "GHBandHero",
                    "With the vertical blank above 60 Hz: what the game counts in frames keeps the console's speed "
                    "(script waits in game frames, the whammy on sustains, the loading screen's animations, clocks "
                    "counted in vertical blanks) "
                    "and scripts are given the button presses of the frames they skip (false = those run faster, to "
                    "compare)");

// The engine keeps its vertical blanks per second in the global 0x82B26E7C: 60, or 50 for a PAL video mode
// (sub_824072B8, sub_8249E0C8; a script can set it, sub_82515B88). Some of its clocks count vertical blanks and
// divide by it, so they would run as many times faster as the vblank's rate is above 60 (no visible case known: the
// star and the calibration Gustavo saw run fast were the loading screen and script waits, below). Those divisions use
// the global times the vblank's rate over 60. The global itself is left alone: other code compares it with 60
// (sub_8220A330 takes a 50 Hz branch otherwise) or uses it as the console's 60ths for per-frame steps measured in real
// time.
namespace {

// The emulated vertical blanks per second (the SDK's vblank thread): video_mode_refresh_rate with vsync, one a
// millisecond without. Read once a second.
double VblankRate() {
  thread_local double rate = 60.0;
  thread_local std::chrono::steady_clock::time_point read;
  const auto now = std::chrono::steady_clock::now();
  if (read == std::chrono::steady_clock::time_point{} || now - read > std::chrono::seconds(1)) {
    read = now;
    rate = rex::cvar::Query<bool>("vsync") ? std::clamp(rex::cvar::Query<double>("video_mode_refresh_rate"), 24.0, 240.0)
                                           : 1000.0;
  }
  return rate;
}

// The script waits counted in game frames ("Wait N gameframe(s)", in 588 of the game's 9005 scripts; the script
// function sub_824AB8C0 sets type 1 through sub_82573540, or type 8 through sub_825734F0; Band Hero's Yield, sub_82232280,
// a 1-frame type 1 wait while ResetYieldInfo's flag is set, GH Van Halen's did nothing): when each began and its count.
struct FrameWait {
  std::chrono::steady_clock::time_point start;  // when it would begin at 60 frames a second
  std::chrono::steady_clock::time_point end;    // start + frames / 60 s
};
std::mutex g_frame_waits_mutex;
std::unordered_map<uint32_t, FrameWait> g_frame_waits;  // by the wait's guest address (the script + 184)

bool HighRate() { return REXCVAR_GET(ghbandhero_frame_rate_fix) && VblankRate() != 60.0; }

// A frame at the chosen rate: one vertical blank.
double FrameSeconds() { return 1.0 / VblankRate(); }

// The frames as they come, measured on the main thread (GHBandHeroMainFrameHook, once a frame): the last one and an
// average of about the last ten. A frame lasts one vertical blank only while the game keeps the chosen rate; when it falls
// below it (at 240 Hz a frame that takes 5 ms is shown for 8.3), what is counted in frames below keeps the console's time
// by these, as the game's own clock does, not by a frame of 1 / the chosen rate (GH5's).
std::atomic<double> g_last_frame{0.0};     // seconds
std::atomic<double> g_average_frame{0.0};  // seconds

// The next frame's length, as far as it can be told: the recent average (a frame at the chosen rate until one is measured).
double FrameInterval() {
  const double average = g_average_frame.load(std::memory_order_relaxed);
  return average > 0.0 ? average : FrameSeconds();
}

// The length of the frame that just ended.
double LastFrameSeconds() {
  const double last = g_last_frame.load(std::memory_order_relaxed);
  return last > 0.0 ? last : FrameSeconds();
}

// The game's vertical blanks per second scaled to the emulated rate (unchanged at 60 Hz).
void ScaleVblankRate(PPCRegister& rate) {
  if (!REXCVAR_GET(ghbandhero_frame_rate_fix)) {
    return;
  }
  const double vblank_rate = VblankRate();
  if (vblank_rate != 60.0) {
    rate.u64 = uint64_t(int64_t(std::lround(double(int32_t(rate.u32)) * vblank_rate / 60.0)));
  }
}

}  // namespace

// [[midasm_hook]] 0x82414184 (sub_82414058, the main thread's frame, at its end: "addi r1,r1,128"; GH5's 0x82413BCC). A
// sample is kept between a 4000th and a 20th of a second: a load that stops the main thread is not a frame.
void GHBandHeroMainFrameHook() {
  static std::chrono::steady_clock::time_point last;  // only the main thread
  const auto now = std::chrono::steady_clock::now();
  if (last != std::chrono::steady_clock::time_point{}) {
    const double seconds = std::clamp(std::chrono::duration<double>(now - last).count(), 0.00025, 0.05);
    g_last_frame.store(seconds, std::memory_order_relaxed);
    const double average = g_average_frame.load(std::memory_order_relaxed);
    g_average_frame.store(average > 0.0 ? average + 0.1 * (seconds - average) : seconds, std::memory_order_relaxed);
  }
  last = now;
}

// [[midasm_hook]] 0x82631EF0 and 0x82632178 (sub_82631D88): before "extsw r8,r30" / "extsw r11,r30", r30 = the
// game's vertical blanks per second, which divides a clock counted in vertical blanks (the vblank counter
// sub_823B09D8 less the clock's start and its time paused, kept by sub_826305C8) into seconds, and turns seconds back
// into vertical blanks. The clock is advanced by sub_82634EC8 for an object's virtual update (sub_824CD4D0).
void GHBandHeroVblankClockHook(PPCRegister& r30) { ScaleVblankRate(r30); }

// GH Van Halen also scaled the frame's time (sub_822E2A80: the frame's vertical blanks over vertical blanks per
// second). Band Hero's (sub_823B0FA0) needs nothing: the vblank callback sub_823B0748 keeps each vertical blank's interval
// in time base ticks, and the frame's time (+52 and +64 of the timing block 0x82D7DD98) is their sum in seconds.

// Script waits in game frames: sub_82573840, each frame, takes one from the count at +4 of a type 1 or 8 wait and ends
// the wait in the frame after it reaches 0; its first update comes in the frame the script set the wait, so "Wait N
// gameframes" lasts N frames: N / 60 s on the console, half that at 120 FPS (Gustavo in GH Metallica: the
// calibration's beeps and targets ran fast; the calibration's beeps are 64 waits of 1 game frame each, 1.066 s).
// Above 60 Hz the count is held above 0 and let reach 0 in the frame before the one nearest the wait's end at 60
// frames a second, so the wait ends within half a frame of it. A wait set right as the previous one on the same script
// ended (a loop) begins at that one's end at 60 frames a second, not at the frame it was set in, so a loop keeps the
// console's pace on average whatever the frame rate (at 160 Hz a 1-frame wait can only last 12.5 or 18.75 ms).
// Measured with a temporary probe of PlaySound: the calibration's beeps every 1.066 s at 60; 0.533 s at 120 without
// this; 1.33 to 1.46 s with its first version, which ended each wait in the first frame after its end (late by up to a
// frame, and by a whole one whenever a frame came a little early).
namespace {

// The scripts that run every frame (below), and the name of the script a wait belongs to (+60 of the wait).
// "begin BandManager_PrepareBandMemberForRender Wait 1 gameframe repeat" (GH Metallica's BandManager_PrepareBandForRender
// under another name and key): the script function (sub_821B5370 -> sub_821B4650) has each musician place his
// instrument's strings (sub_821BA8A0: each string's start, end, the point the fretting finger presses and the point that
// vibrates, in the world, from the instrument's bones this frame). Held to 60 passes a second, the strings were placed
// every second or third frame at 120 or 160 FPS and stayed where the guitar had been in between: they shook around a
// guitar that moved (Gustavo in GH Metallica, 2026-11-04: his character's, while playing).
constexpr uint32_t kBandRenderScript = 0x02B91F04;
// The scripts that measure their own time by the frame's. The same bytes as GH5's, none new among the 326 scripts only
// this game has.
constexpr uint32_t kSelfTimedScripts[] = {
    0x74B936AB,  // star power's drain: current_deltatime off the meter each pass, or GetFrameLength's in a band
    // button_checker: ButtonCheckerPerFrame (sub_821C0EB0), the player's input object (sub_8219D8E8 or sub_8219BC38) and
    // sub_821E89D0, which takes 20 pixels * the frame's time / 0.1 s off each fret's height after a hit. Held to 60
    // passes a second the frets fell back in 266 ms at 160 FPS (measured in GH5), 100 ms every frame, as at 60.
    0x716BADDC,
    // Scripts that add GetDeltaTime (the frame's time) to their own timers on each pass, and step nothing else by the
    // pass: held to 60 passes a second their timers ran at 60 / 160 of the speed at 160 FPS. The battle paddles' timer, a
    // blinking text, a wait with a limit, a fade over a time, a list of timed events (GH5's five, the same bytes here).
    // Left held, as in GH5 and GH WoR: 0xCFDDF140 (the jam studio's arrows) also adds 0.1 a pass to the time that speeds
    // it up, so every frame it would speed up 2.67 times as soon; held, it scrolls at 60 / the rate.
    0xEC23DF7E, 0xE66D1DA1, 0x4E850682, 0xE20DF41E, 0x3E4FDDAF,
};
// The engine draws the stage and the crowd in one frame and the band in the next (IsPrimaryFrame, 0x316A336A,
// sub_821B64A0, from a count of real frames in sub_82414058). Two scripts align to it, as in GH Warriors of Rock: 0xCAD261DF
// ("if IsPrimaryFrame Wait 2 gameframes else Wait 1", GH5's only one) and 0x6506F6C8 ("if IsPrimaryFrame Wait 1
// gameframe"), so that what follows lands in one kind of frame, and their callers go on in whole frames: a camera cut's
// screen effect, and the band's clips and moves, "align, wait 1 or 2 game frames, change the musician, wait, change him
// back" (their other waits test the song's time, so running every frame does not speed them up). Callers of 0xCAD261DF:
// 0xF4A440B2, 0x9A8F399F, 0xC09883B8, 0xF1C14221; of 0x6506F6C8: 0x01DE43A7, 0x0DA14A2E, 0x9AFA7534 (GH5's, which call
// 0xCAD261DF there) and 0xE4DE9864 (not in GH5). The cut itself, 0xA5BB57BB, calls neither here. Held to 60 passes a
// second, in GH5 a wait of 1 lasted 3 frames at 160 FPS and fell in either kind: each cut showed 1 to 5 frames with no
// scene, the stage from a camera not yet placed, then the new shot (18 such runs in 8000 frames, none with this).
constexpr uint32_t kFrameExactScripts[] = {
    0xCAD261DF, 0x6506F6C8,                                      // the aligners
    0xF4A440B2, 0x9A8F399F, 0xC09883B8, 0xF1C14221,              // call 0xCAD261DF
    0x01DE43A7, 0x0DA14A2E, 0x9AFA7534, 0xE4DE9864,              // call 0x6506F6C8
};
// The drummer: script 0x473164C7 (GH5's bytes) counts down the time to each hit by its step, the length of the two frames
// of a pass (GetFrameLength before and after a "Wait 1 frame"). That wait is a timed one, 1/60 s, a frame on the
// console. At 160 FPS GetFrameLength is 6.25 ms but each wait lasts 18.75, so a step of 12.5 ms took 37.5: in GH5 his
// timers ran at a third of the speed, in slow motion and off the song (Gustavo). Above 60 Hz the script's waits are real
// frames (GHBandHeroDrummerWaitHook), as GH WoR's script wrote them, and the step is what the pass took.
constexpr uint32_t kDrummerScript = 0x473164C7;

bool RunsEveryFrame(uint32_t script) {
  return script == kBandRenderScript || script == kDrummerScript ||
         std::find(std::begin(kSelfTimedScripts), std::end(kSelfTimedScripts), script) != std::end(kSelfTimedScripts) ||
         std::find(std::begin(kFrameExactScripts), std::end(kFrameExactScripts), script) != std::end(kFrameExactScripts);
}

uint32_t WaitingScript(uint32_t wait) {
  auto* kernel = rex::system::kernel_state();
  const uint8_t* base = kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
  if (!base) {
    return 0;
  }
  uint32_t name;
  std::memcpy(&name, base + wait + 60, 4);
  return __builtin_bswap32(name);
}

}  // namespace

// [[midasm_hook]] at the entry of sub_82573540 and sub_825734F0 (setting a game-frame wait): r3 = the wait, r4 = N.
void GHBandHeroFrameWaitSetHook(PPCRegister& r3, PPCRegister& r4) {
  if (!HighRate()) {
    return;
  }
  // A script that measures its own time runs every frame, as the game wrote it. The star power's drain (script
  // 0x74B936AB) waits one game frame per pass and takes current_deltatime (the frame's time) off the meter each pass,
  // or GetFrameLength's in a band: held to 60 passes a second it drained 60 frames' time a second, and at 160 FPS star
  // power lasted 160 / 60 times as long (Gustavo, in GH Metallica and GH Van Halen). Band Hero has the same script; the
  // only other of its 9005 that reads either is the drummer's (kDrummerScript). The waiting script's name is at +60 of
  // its wait (+244 of the script; GH Van Halen's +48, +216).
  // The frets rising on a hit and falling back: the script button_checker (0x716BADDC) calls ButtonCheckerPerFrame
  // (sub_821C0EB0) and waits a game frame; that calls slot 2 of the player's input object (sub_8219D8E8 or sub_8219BC38)
  // and so sub_821E89D0, which takes 20 pixels * the frame's time / 0.1 s off each fret's button_up_pixel_array. Held to
  // 60 passes a second the fall took 266 ms at 160 FPS (Gustavo saw it slow in GH5; a temporary probe measured it), 100
  // ms every frame, as at 60.
  if (RunsEveryFrame(WaitingScript(r3.u32))) {
    std::lock_guard lock(g_frame_waits_mutex);
    g_frame_waits.erase(r3.u32);
    return;
  }
  const auto now = std::chrono::steady_clock::now();
  const auto frame = std::chrono::duration<double>(FrameInterval());
  std::lock_guard lock(g_frame_waits_mutex);
  FrameWait& wait = g_frame_waits[r3.u32];
  const bool follows = wait.end != std::chrono::steady_clock::time_point{} &&
                       std::chrono::abs(std::chrono::duration<double>(now - wait.end)) <= 1.5 * frame;
  wait.start = follows ? wait.end : now;
  wait.end = wait.start + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                              std::chrono::duration<double>(double(int32_t(r4.u32)) / 60.0));
}

// [[midasm_hook]] 0x824ABA68 (the script function Wait, sub_824AB8C0, the units frame and frames, before N * 16.67 ms
// becomes a timed wait; GH5's 0x824AB938): r30 = the script. True goes to the game-frame wait at 0x824ABAA8, with the
// same N.
bool GHBandHeroDrummerWaitHook(PPCRegister& r30) {
  if (!HighRate()) {
    return false;
  }
  auto* kernel = rex::system::kernel_state();
  const uint8_t* base = kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
  if (!base) {
    return false;
  }
  uint32_t name;
  std::memcpy(&name, base + r30.u32 + 244, 4);
  return __builtin_bswap32(name) == kDrummerScript;
}

// [[midasm_hook]] 0x82573884 (sub_82573840, a game-frame wait's count above 0): before "addi r11,r11,-1" and its
// store to +4, r3 = the wait, r11 = the count. Set to 1 (it becomes 0, the wait ends next frame) once the wait's end is
// less than a frame and a half away, so that next frame is the one nearest to it; else to 2 (it stays waiting). A frame
// is the frames' recent length (FrameInterval), so the nearest frame is still chosen when the rate falls below the
// chosen one.
void GHBandHeroFrameWaitCountHook(PPCRegister& r3, PPCRegister& r11) {
  if (!HighRate()) {
    return;
  }
  std::chrono::steady_clock::time_point end;
  {
    std::lock_guard lock(g_frame_waits_mutex);
    auto it = g_frame_waits.find(r3.u32);
    if (it == g_frame_waits.end()) {
      return;
    }
    end = it->second.end;
  }
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true)) {
    REXLOG_INFO("[frame rate] script waits in game frames last as at 60 FPS (vblank at {:.0f} Hz)", VblankRate());
  }
  const double left = std::chrono::duration<double>(end - std::chrono::steady_clock::now()).count();
  r11.u64 = left < 1.5 * FrameInterval() ? 1 : 2;
}

// Button presses polled by scripts: the script function ControllerMake (sub_824C0770; GuitarControllerMake calls it)
// tests the bits of the buttons pressed since the previous update (+52 of a controller's input state; sub_824B5A98
// updates the four controllers' once a frame, +48 = held, +52 = pressed, +56 = released), so a press shows for one
// frame. A script whose game-frame waits are held (above) runs only every second frame or so at 120 Hz and missed the
// presses of the frames between (Gustavo in GH Metallica: the video calibration's strums counted only sometimes; a
// probe at 160: the calibration's script polled every 2 or 3 frames and 3 of 7 strums fell in frames between, 7 of 7
// seen with this; the audio calibration polls the same way). Each script is given the presses of the updates since its previous run that
// it did not see, once, from the last 60th of a second and a frame (of the frames' recent length): they are added to the
// state's pressed bits while its ControllerMake runs, then the bits are put back.
namespace {

constexpr int kKeptUpdates = 8;

struct InputUpdates {
  uint32_t state = 0;  // the input state's guest address
  uint32_t seq = 0;    // updates so far
  uint32_t pressed[kKeptUpdates] = {};
  std::chrono::steady_clock::time_point when[kKeptUpdates] = {};
};

struct ScriptPolls {
  uint32_t run_seq = 0;   // the state's update at the script's current run
  uint32_t prev_seq = 0;  // and at its previous run
  std::chrono::steady_clock::time_point last;
};

struct PatchedState {
  uint32_t state, written, original;
};

std::mutex g_input_mutex;
InputUpdates g_input[4];
std::unordered_map<uint64_t, ScriptPolls> g_script_polls;  // by script << 32 | state
thread_local PatchedState g_patched[4];
thread_local int g_patched_count = 0;

uint8_t* GuestBase() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

uint32_t LoadWord(const uint8_t* base, uint32_t address) {
  uint32_t value;
  std::memcpy(&value, base + address, 4);
  return __builtin_bswap32(value);
}

void StoreWord(uint8_t* base, uint32_t address, uint32_t value) {
  value = __builtin_bswap32(value);
  std::memcpy(base + address, &value, 4);
}

}  // namespace

// [[midasm_hook]] 0x824B5C5C (sub_824B5A98, after the input state's held, pressed and released bits are stored):
// r30 = the state, r9 = the pressed bits.
void GHBandHeroInputUpdateHook(PPCRegister& r30, PPCRegister& r9) {
  if (!HighRate()) {
    return;
  }
  std::lock_guard lock(g_input_mutex);
  for (InputUpdates& input : g_input) {
    if (input.state == r30.u32 || input.state == 0) {
      input.state = r30.u32;
      const int slot = int(++input.seq % kKeptUpdates);
      input.pressed[slot] = r9.u32;
      input.when[slot] = std::chrono::steady_clock::now();
      return;
    }
  }
}

// [[midasm_hook]] at the entry of sub_824C0770 (ControllerMake): r4 = the script.
void GHBandHeroButtonPollHook(PPCRegister& r4) {
  g_patched_count = 0;
  uint8_t* base = GuestBase();
  if (!HighRate() || !base) {
    return;
  }
  const auto now = std::chrono::steady_clock::now();
  const auto window = std::chrono::duration<double>(1.0 / 60.0 + FrameInterval());
  std::lock_guard lock(g_input_mutex);
  if (g_script_polls.size() > 256) {
    std::erase_if(g_script_polls, [&](const auto& entry) { return now - entry.second.last > std::chrono::seconds(1); });
  }
  for (const InputUpdates& input : g_input) {
    if (input.state == 0) {
      continue;
    }
    ScriptPolls& polls = g_script_polls[uint64_t(r4.u32) << 32 | input.state];
    if (polls.last == std::chrono::steady_clock::time_point{} || now - polls.last > window * 4) {
      polls.run_seq = polls.prev_seq = input.seq;  // a script new to polling: only the current update
    } else if (polls.run_seq != input.seq) {
      polls.prev_seq = polls.run_seq;
      polls.run_seq = input.seq;
    }
    polls.last = now;
    uint32_t missed = 0;
    const uint32_t kept = polls.run_seq >= kKeptUpdates ? polls.run_seq - kKeptUpdates + 1 : 1;
    for (uint32_t seq = std::max(polls.prev_seq + 1, kept); seq < polls.run_seq; ++seq) {
      const int slot = int(seq % kKeptUpdates);
      if (now - input.when[slot] <= window) {
        missed |= input.pressed[slot];
      }
    }
    const uint32_t original = LoadWord(base, input.state + 52);
    if (missed & ~original) {
      StoreWord(base, input.state + 52, original | missed);
      g_patched[g_patched_count++] = {input.state, original | missed, original};
    }
  }
}

// [[midasm_hook]] 0x824C0830 (sub_824C0770's exit, every path): the pressed bits put back.
void GHBandHeroButtonPollEndHook() {
  uint8_t* base = GuestBase();
  for (int i = 0; i < g_patched_count && base; ++i) {
    const PatchedState& patched = g_patched[i];
    if (LoadWord(base, patched.state + 52) == patched.written) {
      StoreWord(base, patched.state + 52, patched.original);
    }
  }
  g_patched_count = 0;
}

// The whammy on a sustain (the note hold update sub_821E9FA8 -> sub_821E9C30, run every frame for each player:
// sub_821D51F8 -> sub_821EB290 -> sub_821EAB98, 120 calls a second at 120 FPS): sub_821E7168 (the whammy bar;
// sub_821E1D48 for the touch strip) compares the bar's position (0 at rest to 1) with the previous frame's and, if it
// moved more than 0.03, sets a countdown at +98688 to 5; sub_821E9C30 takes one from it each frame and, while above 0,
// the player is whammying (star power grows by the frame's time); after 5 frames in a row of whammying or not
// (+98692, +98696) it tells the held frets (sub_821CBE08), and a star power sustain turns blue or back. Above 60 Hz a
// frame's movement is smaller and 5 frames are shorter (Gustavo at 120 in GH Metallica: the blue came and went while
// he moved the bar). The position is compared with the one from a 60th of a second before, and the 5 frames are 5 / 60 s:
// as many frames as fit in it at the frames' recent length (FrameInterval), not at the chosen rate, so that they keep
// 5 / 60 s when the rate falls below it. The count of frames in a row is compared with 5 for equality, so each run of
// frames keeps the number it began with: one that changed as the rate moved could jump past it.
namespace {

struct WhammySamples {
  std::chrono::steady_clock::time_point when[8] = {};
  float position[8] = {};
  int next = 0;
};

std::mutex g_whammy_mutex;
std::unordered_map<uint64_t, WhammySamples> g_whammy;  // by the note hold object << 1 | axis

// The position the previous frame's is replaced by: the sample nearest a 60th of a second before (the newest at
// least that less half a frame old, if not more than a frame and a half over it); else the game's own.
void CompareWhammyOverSixtieth(int axis, uint32_t object, PPCRegister& position, PPCRegister& previous) {
  if (!HighRate()) {
    return;
  }
  const auto now = std::chrono::steady_clock::now();
  const double frame = FrameInterval();
  std::lock_guard lock(g_whammy_mutex);
  WhammySamples& samples = g_whammy[uint64_t(object) << 1 | uint64_t(axis)];
  for (int i = 1; i <= 8; ++i) {
    const int slot = (samples.next - i) & 7;
    const double age = std::chrono::duration<double>(now - samples.when[slot]).count();
    if (age >= 1.0 / 60.0 - 0.5 * frame) {
      if (age <= 1.0 / 60.0 + 1.5 * frame) {
        previous.f64 = samples.position[slot];
      }
      break;
    }
  }
  samples.when[samples.next] = now;
  samples.position[samples.next] = float(position.f64);
  samples.next = (samples.next + 1) & 7;
}

// The console's frames (60ths of a second) in frames of the recent length.
int WhammyFrames(int frames) { return std::max(1, int(std::lround(frames / 60.0 / FrameInterval()))); }

// A run of frames in a row of whammying or not, by its count's guest address: the count last seen and the number of
// frames its 5 / 60 s were given when it began.
struct WhammyRun {
  int32_t last = 0;
  int frames = 0;
};
std::unordered_map<uint32_t, WhammyRun> g_whammy_runs;  // under g_whammy_mutex

int WhammyRunFrames(uint32_t count_address, int32_t count) {
  std::lock_guard lock(g_whammy_mutex);
  WhammyRun& run = g_whammy_runs[count_address];
  if (run.frames == 0 || count <= run.last) {  // the first seen, or a new run (the count went back)
    run.frames = WhammyFrames(5);
  }
  run.last = count;
  return run.frames;
}

}  // namespace

// [[midasm_hook]] 0x821E72E4 and 0x821E73DC (sub_821E7168) and 0x821E1ED4 (sub_821E1D48): before "fsubs" of the
// position and the previous frame's: r30 / r31 = the note hold object, f30 / f0 = the position, f0 / f13 = the previous
// (the guitar's in f0 here, GH Van Halen's in f13).
void GHBandHeroWhammyHook(PPCRegister& object, PPCRegister& position, PPCRegister& previous) {
  CompareWhammyOverSixtieth(0, object.u32, position, previous);
}

// [[midasm_hook]] 0x821E73F0 (sub_821E7168, a controller's second axis): r30, f29 = the position, f0 = the previous.
void GHBandHeroWhammy2Hook(PPCRegister& object, PPCRegister& position, PPCRegister& previous) {
  CompareWhammyOverSixtieth(1, object.u32, position, previous);
}

// [[midasm_hook]] 0x821E7310, 0x821E7410 (r10) and 0x821E1F00 (r9): the countdown of 5 frames before it is stored.
void GHBandHeroWhammyCountdownHook(PPCRegister& frames) {
  if (HighRate()) {
    frames.u64 = uint64_t(WhammyFrames(int32_t(frames.u32)));
  }
}

// [[midasm_hook]] 0x821E9DE0 and 0x821E9EC0 (sub_821E9C30): before "cmpwi cr6,r11,5", r11 = the frames in a row of
// whammying or not (dead after the comparison), r30 / r10 = that count's address (+98692 / +98696 of the note hold
// object): 5 when they reach 5 / 60 s.
void GHBandHeroWhammyStateHook(PPCRegister& count_address, PPCRegister& r11) {
  if (HighRate()) {
    const int32_t count = int32_t(r11.u32);
    r11.u64 = count == WhammyRunFrames(count_address.u32, count) ? 5 : 0;
  }
}

// The loading screen (sub_825DB8C8, drawn every frame while the byte 0x82DB251C is set: sub_82428B08 ->
// sub_82426FB0) animates by the frame, not by time:
// - the boot's spinning star (sub_825DB550: the four vertices of the mesh at [0x82F028CC + 80] turned about their
//   center by 0.175 radians each frame, 10.5 a second at 60; GH Van Halen's 0.06);
// - the icon the script function DisplayLoadingScreen can give it (sub_824274A8, its state at 0x82B27720, set through
//   sub_82427030) turns by a whole number of degrees each frame (+48, added to the angle at +44; not seen);
// - in its mode 2 (not seen) a grid waves with two phases that advance by constants each frame.
// Above 60 Hz each step is scaled by the frame's length in 60ths of a second; the icon's fraction of a degree is carried
// to the next frame. The render thread draws it while the main thread loads, so each animation measures its own frames
// (sub_825DB8C8 steps each once a frame: the waves after the grid's loops, then calls the star and the icon); a step after
// more than a 10th of a second without one is a frame at the chosen rate (the screen shown again), and none is longer
// than a 20th. (Its flipbook, sub_825DB7A0, already runs on the time base.)
namespace {

enum LoadingStep { kLoadingStar, kLoadingIcon, kLoadingWave, kLoadingWave2, kLoadingSteps };

double FrameStepScale(LoadingStep step) {
  static std::chrono::steady_clock::time_point last[kLoadingSteps];  // only the render thread draws the loading screen
  const auto now = std::chrono::steady_clock::now();
  double seconds = FrameSeconds();
  if (last[step] != std::chrono::steady_clock::time_point{}) {
    const double since = std::chrono::duration<double>(now - last[step]).count();
    if (since <= 0.1) {
      seconds = std::min(since, 0.05);
    }
  }
  last[step] = now;
  return 60.0 * seconds;
}

}  // namespace

// [[midasm_hook]] 0x825DB664 (sub_825DB550): after "lfd f31,-24376(r10)", f31 = the star's turn in radians each frame.
void GHBandHeroLoadingStarHook(PPCRegister& f31) {
  if (HighRate()) {
    f31.f64 *= FrameStepScale(kLoadingStar);
  }
}

// [[midasm_hook]] 0x824274F4 (sub_824274A8): before "add r11,r10,r11", r10 = the icon's step in degrees, r11 = its angle.
void GHBandHeroLoadingSpinnerHook(PPCRegister& r10) {
  if (!HighRate()) {
    return;
  }
  static double carry = 0.0;  // only the render thread draws the loading screen
  const double step = double(int32_t(r10.u32)) * FrameStepScale(kLoadingIcon) + carry;
  const double whole = std::floor(step);
  carry = step - whole;
  r10.u64 = uint64_t(int64_t(whole));
}

// [[midasm_hook]] 0x825DBAF0 and 0x825DBB00 (sub_825DB8C8's mode 2): before "fadds f0,f0,f29" and "fadds f0,f13,f0",
// f29 / f0 = the step each wave phase advances by.
void GHBandHeroLoadingWaveHook(PPCRegister& step) {
  if (HighRate()) {
    step.f64 = double(float(step.f64 * FrameStepScale(kLoadingWave)));
  }
}
void GHBandHeroLoadingWave2Hook(PPCRegister& step) {
  if (HighRate()) {
    step.f64 = double(float(step.f64 * FrameStepScale(kLoadingWave2)));
  }
}

// The strings' vibration: each frame sub_821B3830 multiplies each string's amplitude by 0.99 (the script global
// 0xB70CCCA3) and adds the frame's time to its timer; sub_821BA8A0 places each string displaced by amplitude *
// sin(2 pi * 62 Hz (0xFD146572) * timer). At 60 FPS a 62 Hz vibration sampled 60 times a second shows as a slow wobble of
// 62 - 60 = 2 Hz, what the console shows; placed every frame above 60 (kBandRenderScript), the real 62 Hz would show, and
// the 0.99 a frame fades it faster. Above 60 Hz the frequency is the one 60 FPS shows and the fade 0.99 per 60th of a
// second (GH Metallica's): each frame by its own length (LastFrameSeconds), not by a frame at the chosen rate.
// [[midasm_hook]] 0x821BA920 (sub_821BA8A0: after the call returning the frequency, before "fmr f29,f1"): f1 = Hz.
void GHBandHeroStringFrequencyHook(PPCRegister& f1) {
  if (HighRate()) {
    f1.f64 -= 60.0 * std::round(f1.f64 / 60.0);
  }
}

// [[midasm_hook]] 0x821B386C (sub_821B3830: after the call returning the fade, before the strings' loop): f1 = the
// amplitude's factor each frame.
void GHBandHeroStringFadeHook(PPCRegister& f1) {
  if (HighRate() && f1.f64 > 0.0) {
    f1.f64 = double(float(std::pow(f1.f64, 60.0 * LastFrameSeconds())));
  }
}
