// ghbandhero - a drum kit on five lanes unless the launcher marks it four-lane (GH Warriors of Rock's src/drums.cpp, the same
// code; this game's controller port keeps its flags and its reads counter at other offsets).
//
// The game tells a Guitar Hero kit from a Rock Band kit by itself. sub_82489908 (a port's capabilities) makes a drum kit
// (XInput subtype 8) the game's type 11 and remembers bit 0 of its capability flags as 128 at +216 of the port.
// sub_82489C10 (the port's state, each frame) packs the buttons PS2-style and active-low into a halfword (0x200 is L3,
// the left stick's click) and then, at 0x8248A078: type 11 with L3 held and without the flag -> type 10, Guitar Hero's
// five lanes; type 10 with L3 let go within its first 300 reads (+244) -> 11 again; held 300 reads, 10 for good.
// UseFourLaneHighway (sub_821D4288 -> sub_821D4190) keeps type 11 on the four-lane highway, a Rock Band kit's. So a kit
// is five-lane only while it holds the left stick's click from the start: a Guitar Hero kit's own sign, which neither
// the keyboard nor a Rock Band kit, a MIDI adapter or a controller played as drums gives (GH Warriors of Rock, a
// player's PS3 kit through a MIDI Pro Adapter and the keyboard, 2026-11-15: every one of them played on four lanes,
// whatever the launcher's Drum kit row said).
//
// The launcher's choice decides instead: four lanes is the SDK's flag 1 (gamepad_device_subtypes "8:1", +216 & 128),
// and any other drum kit becomes type 10 at its first read, for good (the reads counter at 300).
//
// Off (ghbandhero_drum_kit_five_lanes = false): the game's own test.

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <atomic>
#include <cstdint>
#include <cstring>

REXCVAR_DEFINE_BOOL(ghbandhero_drum_kit_five_lanes, true, "GHBandHero",
                    "A drum kit plays on five lanes (Guitar Hero's) unless the launcher marks it four-lane "
                    "(gamepad_device_subtypes \"8:1\"); false = the game's own test, five lanes only for a kit that "
                    "holds the left stick's click (a Guitar Hero kit's sign), four for every other");

namespace {

constexpr uint32_t kPortType = 8;      // 11: a drum kit as read from its capabilities; 10: a five-lane kit
constexpr uint32_t kPortFlags = 216;   // 128: the capability flags' bit 0, the four-lane mark (GH WoR: +120)
constexpr uint32_t kPortReads = 244;   // a five-lane kit's reads; from 300 on its type stays (GH WoR: +252)
constexpr uint32_t kPortUser = 12;
constexpr uint32_t kTypeDrumKit = 11;
constexpr uint32_t kTypeFiveLanes = 10;
constexpr uint32_t kFourLaneMark = 128;
constexpr uint32_t kReadsForGood = 300;

uint8_t* GuestBase() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

uint32_t Load32(const uint8_t* base, uint32_t address) {
  uint32_t value;
  std::memcpy(&value, base + address, 4);
  return __builtin_bswap32(value);
}

void Store32(uint8_t* base, uint32_t address, uint32_t value) {
  value = __builtin_bswap32(value);
  std::memcpy(base + address, &value, 4);
}

}  // namespace

// [[midasm_hook]] 0x8248A078 (sub_82489C10, "cmpwi cr6,r11,11" after "lwz r11,8(r30)"): r11 = the port's type, r30 =
// the port. A drum kit without the four-lane mark becomes type 10 here, in the register and in the port.
void GHBandHeroDrumKitLanesHook(PPCRegister& type, PPCRegister& port) {
  if (type.u32 != kTypeDrumKit || !REXCVAR_GET(ghbandhero_drum_kit_five_lanes)) {
    return;
  }
  uint8_t* base = GuestBase();
  if (!base || (Load32(base, port.u32 + kPortFlags) & kFourLaneMark)) {
    return;
  }
  type.u64 = kTypeFiveLanes;
  Store32(base, port.u32 + kPortType, kTypeFiveLanes);
  Store32(base, port.u32 + kPortReads, kReadsForGood);
  static std::atomic<uint32_t> logged{0};
  const uint32_t user = Load32(base, port.u32 + kPortUser);
  if (user < 32 && !(logged.fetch_or(1u << user) & (1u << user))) {
    REXLOG_INFO("[drums] slot {}: a drum kit on five lanes (not marked four-lane; ghbandhero_drum_kit_five_lanes)", user);
  }
}
