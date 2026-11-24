// ghbandhero - the song's clocks from the vertical blanks' own times (ghbandhero_song_clock_exact; off: the game's own sums), and
// each song's clock report in the log. GH Warriors of Rock's src/song_clock.cpp (its docs/song-clock.md, local_docs/61,
// 62, 64), on Band Hero's code.
//
// The song's clock is a double in seconds at 0x82D81898 (GetSongTime, GetSongTimeMs, IncrementSongTimer read it;
// ResetSongTimer, sub_823EB5A8, sets it). Band Hero keeps a second one at 0x82D818A0 (+8 of the first), which ResetSongTimer
// sets to the same time and part of the gameplay reads (sub_823EB7C0, sub_823EBB28: sub_82203918, sub_82204ED0,
// sub_82209A10, ...). GH Warriors of Rock dropped it (its update, called from the same place of the main loop, is an
// empty function, sub_8247F600). Each main-thread frame:
// - sub_823EBDC0 (from sub_822398A8) advances the first: c = the vertical blank counter (0x82D7DFD0, sub_823B09D8),
//   L = the count at the last update (+16 of the clock), n = c - L, and the clock gets sub_823B0A38(n) seconds times
//   the time scale; L = c after. Unlike GH Warriors of Rock's (sub_823D1490) it never counts a blank in advance: a
//   frame that finds no new blank adds nothing.
// - sub_823EBF10 (from sub_82428B08, the main loop) advances the second the same way from its own count (+24 of the
//   first clock), after taking up the blanks the first one set aside while the time scale was below 1.
// sub_823B0A38(n) sums n intervals from a ring of the last 10 (the vblank callback sub_823B0748 writes each blank's
// interval in time base ticks at 0x82D7DD98 + 128 + 8 * index, the blank's own time at +576, then the count at +568),
// GH Warriors of Rock's sub_8239FE30 instruction for instruction, and so with its faults, measured there against the
// steady clock:
// - the sum starts at the ring's next slot, the oldest interval, not the newest: an interval of 10 blanks ago stands
//   for the newest of the n. Nothing while the blanks are even (V-Sync). With V-Sync off the blank is a timer, its
//   intervals uneven, and the sums no longer add up to the time that passed: -110 to +608 ppm from run to run at
//   480 Hz in GH Warriors of Rock (up to 36 ms a minute, the notes off the music).
// - n > 10: the ring holds 10 intervals and the sum walks it again, so a frame longer than 10 blanks adds the wrong
//   ones: the clock stepped by 3 to 10 ms either way at each 150 ms hitch of the main thread there.
// (Part of GH Warriors of Rock's drift with V-Sync off was the SDK's: its guest clock gained on the host's, 200 to
// 260 ppm, fixed in the SDK, change 123. The blanks' times here are that clock's.)
//
// With ghbandhero_song_clock_exact (on) each clock's step is the time between the blanks themselves: a hook in the vblank
// callback keeps each blank's time (the game's own, +576, by its count), and each update adds T(c) - T(L) in place of
// the game's sum. The frames' logic is the game's own, so the notes move as before, and with even blanks every step is
// what the game's sum gives.
//
// The report, with the option on or off: at a song's end "[song sync] the song's clock ..." gives the updates, those
// that found no new blank or more than 10 at once, and how far the game's own sums are from the blanks' times for each
// clock: with the option off that is the clock's error in the song.

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

REXCVAR_DEFINE_BOOL(ghbandhero_song_clock_exact, true, "GHBandHero",
                    "The song's clocks advanced by the time between the vertical blanks themselves: they no longer "
                    "drift from the music with V-Sync off (the game sums the wrong blanks' intervals when the blanks "
                    "are uneven) nor step at a hitch longer than 10 blanks. false = the game's own sums of the last "
                    "blanks' intervals");

namespace {

// The game's timing block: the time base's ticks a second (+32) and the vertical blank count (+568, 0x82D7DFD0).
constexpr uint32_t kTiming = 0x82D7DD98;
constexpr uint32_t kTicksPerSecond = kTiming + 32;
constexpr uint32_t kBlankCountOffset = 568;
// The last update's count, +16 of each clock.
constexpr uint32_t kLastCountOffset = 16;

// The blanks' times kept: 16 s at 1000 Hz, 102 s at 160.
constexpr uint64_t kKept = 16384;

struct Blanks {
  std::mutex mutex;
  bool any = false;
  uint64_t count[kKept] = {};
  int64_t ticks[kKept] = {};
};
Blanks g_blanks;

uint8_t* GuestBase() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

uint64_t Load64(const uint8_t* base, uint32_t address) {
  uint64_t value;
  std::memcpy(&value, base + address, 8);
  return __builtin_bswap64(value);
}

// The time of blank `count`, in ticks, if it is still kept. With g_blanks.mutex held.
bool BlankTime(uint64_t count, int64_t& ticks) {
  const uint64_t slot = count % kKept;
  if (!g_blanks.any || g_blanks.count[slot] != count) {
    return false;
  }
  ticks = g_blanks.ticks[slot];
  return true;
}

// One clock's figures in a song.
struct ClockReport {
  uint64_t updates = 0, idle = 0, many = 0, unmeasured = 0;
  double game = 0.0, blanks = 0.0;  // seconds: the game's sums, the blanks' times
};

// One song's figures.
struct Report {
  bool active = false;
  ClockReport clock[2];
};
std::mutex g_report_mutex;
Report g_report;

// An update of clock `which` (0 = the first, 1 = the second) from blank `last` to blank `count`, over `blanks` blanks;
// f1 = the game's sum for them, in seconds, replaced by the blanks' times with ghbandhero_song_clock_exact.
void Step(int which, uint64_t last, uint64_t count, int32_t blanks, PPCRegister& f1) {
  const bool idle = blanks <= 0;
  bool measured = false;
  double step = 0.0;
  if (!idle && last + uint64_t(blanks) == count) {
    int64_t now = 0, before = 0;
    bool have;
    {
      std::lock_guard lock(g_blanks.mutex);
      have = BlankTime(count, now) && BlankTime(last, before);
    }
    const uint8_t* base = GuestBase();
    const uint64_t ticks_per_second = base ? Load64(base, kTicksPerSecond) : 0;
    if (have && ticks_per_second != 0 && now >= before) {
      step = double(now - before) / double(ticks_per_second);
      measured = true;
    }
  }
  {
    std::lock_guard lock(g_report_mutex);
    if (g_report.active) {
      ClockReport& report = g_report.clock[which];
      ++report.updates;
      report.idle += idle;
      report.many += blanks > 10;
      report.unmeasured += !idle && !measured;
      if (measured) {
        report.game += f1.f64;
        report.blanks += step;
      }
    }
  }
  if (measured && REXCVAR_GET(ghbandhero_song_clock_exact)) {
    f1.f64 = step;
  }
}

std::string Describe(const ClockReport& report) {
  return fmt::format("{} updates ({} with no new blank, {} with more than 10{}), the game's own sums {:+.2f} ms from "
                     "the blanks' times over {:.1f} s",
                     report.updates, report.idle, report.many,
                     report.unmeasured ? fmt::format(", {} not measured", report.unmeasured) : std::string(),
                     (report.game - report.blanks) * 1000.0, report.blanks);
}

}  // namespace

// [[midasm_hook]] 0x823B077C (sub_823B0748, the vblank callback: "addi r3,r8,1", after the blank's time, r9 ticks since
// the game's start, was stored at +576 of the timing block r11 and before the count at +568 advances): the blank's
// time kept by the count it is about to get, so it is there by the time an update can read that count.
void GHBandHeroSongClockBlankHook(PPCRegister& r9, PPCRegister& r11) {
  const uint8_t* base = GuestBase();
  if (!base) {
    return;
  }
  const uint64_t count = Load64(base, r11.u32 + kBlankCountOffset) + 1;
  std::lock_guard lock(g_blanks.mutex);
  const uint64_t slot = count % kKept;
  g_blanks.count[slot] = count;
  g_blanks.ticks[slot] = int64_t(r9.u64);
  g_blanks.any = true;
}

// [[midasm_hook]] 0x823EBE40 (sub_823EBDC0, the first clock's update: "frsp f12,f1", f1 = sub_823B0A38's sum for the
// update's blanks in seconds, before the time scale; r30 = the blank count c, r31 = the clock, whose +16 still holds
// the last update's count L; the game summed c - L blanks, 32-bit).
void GHBandHeroSongClockStepHook(PPCRegister& f1, PPCRegister& r30, PPCRegister& r31) {
  const uint8_t* base = GuestBase();
  if (!base) {
    return;
  }
  const uint64_t last = Load64(base, r31.u32 + kLastCountOffset);
  Step(0, last, r30.u64, int32_t(r30.u32 - uint32_t(last)), f1);
}

// [[midasm_hook]] 0x823EC080 and 0x823EC0D0 (sub_823EBF10, the second clock's update, after its two calls of
// sub_823B0A38: f1 = the sum in seconds; r28 = the blank count c, r30 = the blanks summed, r31 = the second clock,
// whose +16 holds the count they start from: its last update's, plus the blanks it took up from the first clock).
void GHBandHeroSongClock2StepHook(PPCRegister& f1, PPCRegister& r28, PPCRegister& r30, PPCRegister& r31) {
  // No blank to add (the slower path's second call follows its first with none left): nothing to count.
  const uint8_t* base = GuestBase();
  if (!base || int32_t(r30.u32) <= 0) {
    return;
  }
  Step(1, Load64(base, r31.u32 + kLastCountOffset), r28.u64, int32_t(r30.u32), f1);
}

namespace ghbandhero {

void SongClockSongEnded() {
  Report report;
  {
    std::lock_guard lock(g_report_mutex);
    report = g_report;
    g_report = {};
  }
  if (!report.active || report.clock[0].updates == 0) {
    return;
  }
  const bool exact = REXCVAR_GET(ghbandhero_song_clock_exact);
  REXLOG_INFO("[song sync] the song's clock: {}; the second clock (its updates that added blanks): {} ({})",
              Describe(report.clock[0]),
              Describe(report.clock[1]),
              exact ? "the sums not used: ghbandhero_song_clock_exact" : "the clocks' own: ghbandhero_song_clock_exact is off");
}

void SongClockSongStarted() {
  SongClockSongEnded();
  std::lock_guard lock(g_report_mutex);
  g_report = {};
  g_report.active = true;
}

}  // namespace ghbandhero
