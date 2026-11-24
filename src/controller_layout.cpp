// ghbandhero - a controller plays a song as in Guitar Hero III, on the game's own controller code (GH Warriors of Rock's
// src/controller_layout.cpp and its docs/controller-gh3-layout.md, the same engine and the same code, other addresses).
//
// Guitar Hero III played a plain controller on a layout of its own, and Guitar Hero: World Tour's engine kept that
// code: Band Hero still has it, but gives a plain controller the vocals, so the SDK tells the game a controller is a guitar
// (src/startup.cpp's gamepad_subtype) and the game played it as an Xbox 360 guitar (frets A, B, Y, X and LB, strumming
// on the D-pad). With gamepad_guitar_layout = "gh3" these hooks make the gameplay take such a controller for what it
// is, so the game's controller code plays it:
// - sub_821E15B8 (GH III's frets and strum; only from the input logic): the frets pressed this frame from LT (green),
//   LB (red), RB (yellow), RT (blue), A (orange) and B (open, a bass's), the ones held from the same buttons; a strum
//   whenever a fret is pressed (none on a release); nothing held marks the player released.
// - sub_821D51F8 (GuitarInputLogicPerFrame's per-player logic): a controller's rules, as GH III's: no tolerance strums
//   (disallow_tolerance_strums is a guitar's), one gem of the window for the anchoring, a fret-only hit after the
//   frets were let go.
// - check_for_star_power_CFunc (sub_822390E8): Star Power on the D-pad.
// - ControlWhammyPitchShift (sub_821E92B0): the whammy bar on the stick pushed furthest; and the whammy's movement on a
//   sustain (sub_821E7168: star power, the sustain's look), either stick moved.
// - ButtonCheckerPerFrame (sub_821C3450, sub_821E2350) and GetHeldPattern (sub_821D4508): the frets held, the ones
//   lit on the highway.
// Each of them asks sub_821D3720(controller, port), true for a port of 4 or more or a controller of the game's device
// type 8 or 9 (guitar, bass); sub_821D4508 asks inline. The menus and the lobby's scripts (IsGuitarController,
// GetControllerType, MonitorControllerStates) still see a guitar, so the controller can choose one.
//
// Extended sustains (Band Hero's too; Guitar Hero III had none): a chord played while an earlier note's sustain is still
// held. The controller code's chord rule (GH III's own) counts a chord only after every fret was let go since the last
// hit (the player's flag +0x180A4, set by sub_821E15B8 when nothing is held, cleared at each hit), so with the
// sustained fret held the chord never counted. A guitar's chord has no such rule and leaves the sustained frets out
// through the gem's mask (sub_821E1478), which a controller's chord then goes through too. Three more hooks ask instead
// that the chord's own frets were each let go since the last hit: a chord held on into the next still does not count,
// and a sustained fret outside the chord no longer stops it.
//
// And the sustain itself (sub_821E9FA8, each frame): every sustained fret ends on its own when let go, and the whole
// sustain ends when the frets held among the current note's (its mask) are some of its frets but not all of them, the
// sustained fret of an earlier note with it. On a guitar the chord stays held and is strummed again; on the controller
// it is let go and pressed again, and a trigger and a button are never let go in the same frame, so the frame between
// ended the earlier note's sustain too. For the controller code, while an earlier note's fret still sustains, some of
// the current note's frets held count as all of them: the ones let go end on their own, the earlier one goes on. A
// fret pressed outside the note still ends it all, and a sustain without an earlier note's is GH III's.

#include <rex/input/input_system.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>

namespace {

// The game's controller ports: XInput's users.
constexpr uint32_t kPorts = 4;

// Each port's last answer, for the log: -1 not asked yet.
std::array<std::atomic<int>, kPorts> g_logged = {-1, -1, -1, -1};

bool PlaysGh3Layout(uint32_t port) {
  if (port >= kPorts) {
    return false;  // no controller of this PC (a computer player)
  }
  const bool gh3 = rex::input::PlaysGh3Layout(port);
  if (g_logged[port].exchange(gh3 ? 1 : 0) != (gh3 ? 1 : 0)) {
    REXLOG_INFO("[controller layout] port {}: {}", port,
                gh3 ? "a controller (or the keyboard) on Guitar Hero III's layout, played by the game's controller code"
                    : "a guitar (or a controller or the keyboard as one), played as one");
  }
  return gh3;
}

// A player's frets: whether the controller code reads them, the ones held at its last read, and the ones held at every
// read since the last hit (the game's patterns: green 0x10000, red 0x1000, yellow 0x100, blue 0x10, orange 0x1, open
// 0x100000).
struct Frets {
  uint32_t player = 0;  // the player object's guest address, 0: a free slot
  bool controller_code = false;
  uint32_t held = 0;
  uint32_t kept = 0;
};

// Every player whose frets the game reads (sub_821E15B8); the oldest slot taken over if they ever run out.
constexpr size_t kSlots = 16;
std::mutex g_frets_mutex;
std::array<Frets, kSlots> g_frets;
size_t g_next_slot = 0;

Frets* FindFrets(uint32_t player) {
  for (Frets& f : g_frets) {
    if (f.player == player) {
      return &f;
    }
  }
  return nullptr;
}

Frets& SlotFor(uint32_t player) {
  if (Frets* f = FindFrets(player)) {
    return *f;
  }
  Frets& f = g_frets[g_next_slot];
  g_next_slot = (g_next_slot + 1) % kSlots;
  f = Frets{player};
  return f;
}

// The player's frets whose sustain still runs (sub_821E9FA8's +0x1819C; GH Warriors of Rock's +0x181E0).
constexpr uint32_t kSustainingFrets = 0x1819C;

uint8_t* GuestBase() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

uint32_t LoadWord(const uint8_t* base, uint32_t address) {
  uint32_t value;
  std::memcpy(&value, base + address, 4);
  return __builtin_bswap32(value);
}

}  // namespace

// [[midasm_hook]]s right after `bl sub_821D3720` (it leaves r4, the port, as it was) in the gameplay's functions: a
// controller on Guitar Hero III's layout is not a guitar there.
void GHBandHeroControllerLayoutHook(PPCRegister& r3, PPCRegister& r4) {
  if ((r3.u32 & 0xFF) != 0 && PlaysGh3Layout(r4.u32)) {
    r3.u64 = 0;
  }
}

// [[midasm_hook]] 0x821D46AC (sub_821D4508, GetHeldPattern's: the inline check's result in r11, the port in r31).
void GHBandHeroControllerLayoutHeldHook(PPCRegister& r11, PPCRegister& r31) {
  if ((r11.u32 & 0xFF) != 0 && PlaysGh3Layout(r31.u32)) {
    r11.u64 = 0;
  }
}

// [[midasm_hook]] 0x821E1634 (sub_821E15B8, the frets' read, right after its `bl sub_821D3720`; the player in r26): as
// GHBandHeroControllerLayoutHook, and the player noted as read by the controller code or not.
void GHBandHeroControllerReadHook(PPCRegister& r3, PPCRegister& r4, PPCRegister& player) {
  GHBandHeroControllerLayoutHook(r3, r4);
  std::lock_guard lock(g_frets_mutex);
  Frets& f = SlotFor(player.u32);
  const bool controller_code = (r3.u32 & 0xFF) == 0;
  if (f.controller_code != controller_code) {
    f.controller_code = controller_code;
    f.held = f.kept = 0;
  }
}

// [[midasm_hook]] 0x821E1800 (sub_821E15B8's controller path, once the frets are read: the player in r26, the frets
// held in r31, 0 for none).
void GHBandHeroControllerFretsHook(PPCRegister& player, PPCRegister& held) {
  std::lock_guard lock(g_frets_mutex);
  Frets& f = SlotFor(player.u32);
  f.held = held.u32;
  f.kept &= held.u32;
}

// [[midasm_hook]] 0x821E727C (sub_821E7168, the whammy's movement on a sustain, right after its `bl sub_821D3750`,
// IsGuitarController, left a guitar for the menus; the player in r30): a player on the controller code is not a guitar
// there either. A guitar's bar is the right stick's X from the bar's rest (+156 of the player info), so a stick moved
// over most of its travel barely counted; a controller's is either stick moved, its X or its Y (the one off the centre,
// the left first), as Guitar Hero III's.
void GHBandHeroControllerWhammyHook(PPCRegister& r3, PPCRegister& player) {
  if ((r3.u32 & 0xFF) == 0) {
    return;
  }
  std::lock_guard lock(g_frets_mutex);
  if (const Frets* f = FindFrets(player.u32); f && f->controller_code) {
    r3.u64 = 0;
  }
}

// [[midasm_hook]] 0x821D5FDC (sub_821D51F8: a gem hit, where the game clears the player's flag; the player in r26).
void GHBandHeroControllerHitHook(PPCRegister& player) {
  std::lock_guard lock(g_frets_mutex);
  if (Frets* f = FindFrets(player.u32)) {
    f->kept = f->held;
  }
}

// [[midasm_hook]] 0x821D6100 (sub_821D51F8: a controller's chord, the player's flag just read into r11; the gem's
// frets in r27, the player in r26): the flag counts as set when none of the chord's frets was held since the last hit.
void GHBandHeroControllerChordHook(PPCRegister& r11, PPCRegister& gem_frets, PPCRegister& player) {
  if (r11.u32 == 1) {
    return;
  }
  std::lock_guard lock(g_frets_mutex);
  const Frets* f = FindFrets(player.u32);
  if (f && f->controller_code && (f->kept & gem_frets.u32) == 0) {
    r11.u64 = 1;
  }
}

// [[midasm_hook]] 0x821EA394 (sub_821E9FA8, each frame of a sustain, once each sustained fret let go has ended: r25 the
// frets held among the note's mask, r26 the note's frets, the player in r31). GH Warriors of Rock's site has a register
// nonzero once the sustain is over, from a block Band Hero's function does not have: here the game's own test that follows
// decides that (the same as on a guitar).
void GHBandHeroControllerSustainHook(PPCRegister& r25, PPCRegister& r26, PPCRegister& r31) {
  const uint32_t held = r25.u32;
  const uint32_t note = r26.u32;
  if (held == note || held == 0 || (held & ~note) != 0) {
    return;  // as the note, nothing of it, or a fret outside it: as the game decides
  }
  {
    std::lock_guard lock(g_frets_mutex);
    const Frets* f = FindFrets(r31.u32);
    if (!f || !f->controller_code) {
      return;
    }
  }
  const uint8_t* base = GuestBase();
  if (!base) {
    return;
  }
  const uint32_t sustaining = LoadWord(base, r31.u32 + kSustainingFrets);
  if ((sustaining & ~note) != 0) {
    r25.u64 = note;  // an earlier note's fret still sustains: some of this note's frets held count as all
  }
}
