// ghbandhero - the game's clock at the rate of the timebase it reads.
//
// The engine's timer (sub_823B0CC0 sets it up, sub_823B0C20 and the vertical blank callback read it) measures time
// with mftb, divided by the frequency KeQueryPerformanceFrequency reports (stored at 0x82D7DDB8). The original
// Xbox 360's kernel reports 50,000,000 while its timebase runs at 49,875,000 Hz, and the game corrects it: "if the
// frequency is 50,000,000, use 49,875,000". The SDK's timebase runs at 50,000,000 Hz, as it reports, so with the
// correction the game's clock ran 0.25 % fast: the notes went ahead of the song's audio, which plays at the sound
// device's 48 kHz, by about 150 ms a minute, until a pause put them back together (the game seeks the audio to its
// clock when a song resumes; measured in GH Metallica, the same code in GH Van Halen and here). The later consoles
// (the Corona and Waitsburg boards) have the same problem with Guitar Hero.
//
// The hook keeps the frequency the SDK reports.

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>

#include <atomic>

REXCVAR_DEFINE_BOOL(ghbandhero_timer_fix, true, "GHBandHero",
                    "Songs: keep the timer frequency the system reports (the timebase's 50 MHz) instead of the "
                    "original console's 49.875 MHz; false = the original code (the audio drifts out of sync "
                    "with the notes, as on the later Xbox 360 models)");

// [[midasm_hook]] 0x823B0D5C (sub_823B0CC0, the timer's setup): before "cmplw cr6,r11,r7", r11 = the frequency
// KeQueryPerformanceFrequency returned and r7 = 50,000,000; when they are equal the game stores 49,875,000.
// r7 is not used after the comparison (lis r7 overwrites it). GH Van Halen's compared with r6.
void GHBandHeroTimerFrequencyHook(PPCRegister& r7) {
  if (!REXCVAR_GET(ghbandhero_timer_fix)) {
    return;
  }
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true)) {
    REXLOG_INFO("[timer] keeping the reported timer frequency (ghbandhero_timer_fix)");
  }
  r7.u32 = 0;
}
