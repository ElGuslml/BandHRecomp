// ghbandhero - Guitar Hero World Tour's Star Power sound (ghbandhero_star_power_reverb, off: as on the console). Ported from GH
// Warriors of Rock's src/star_power_reverb.cpp (its docs/star-power-reverb.md has the measurements).
//
// World Tour (its default.xex and its scripts, decoded from its qb.pab.xen and named by its dbg.pak.xen): preload_song
// (0x94323F7F, scripts\guitar\guitar_song.qb) creates a send stack for each song on starpower_sendeffect_slot (1),
// CreateEffectSendStack effects = [Starpower_Echo, Starpower_Reverb, HighPass_StarPower]
// (scripts\game\audio\audio_effects_settings.qb), end_song destroys it, and the pause mutes and unmutes it
// (PauseGh3Sounds, UnpauseGh3Sounds). Star Power on (GuitarEvent_StarPowerOn -> GH_Star_Power_Verb_On, 0xDF84B375,
// scripts\game\audio\global_sound_logic.qb) sets the echo's delay from the song's tempo (get_song_tempo_cfunc), then
// the echo's mix and the reverb for the player's instrument, single player or not, and sends the player's own stream to
// the stack (SongSetStarpowerEffectsSendVolume, at once: 2^(dB / 6), at most 4, World Tour's sub_820E2378); Star Power
// off (GH_Star_Power_Verb_Off, 0x4CB84F65) cuts that send (-100 dB). The band and the crowd do not go through it.
//
// Band Hero keeps World Tour's script names (GH_Star_Power_Verb_On 0xDF84B375 and _Off 0x4CB84F65, the same file) and the
// three structs with World Tour's values, unused (no script names them). Its Star Power still works out the echo's
// delay and does not use it, and only raises the player's send to the venue's reverb: SongSetVenueEffectSendVolume
// player vol = 0 dB guitar, -10 bass, -3 drums (VocalDSPSetParams for vocals), -100 off, not in the jam session. Its
// one stack is the venue's, at the boot, on slot 1 (Create_Venue_Effects_Settings from guitar_startup_spawned: a
// sfxreverb named VenueVerb). Its engine (FMOD Ex, linked into the executable, so recompiled code) still has all World
// Tour used: the Echo, SfxReverb and Highpass types and their parameters (the table 0x82B1ED68, names filled at the
// start), two send stack slots (0x82D80330, 12 bytes each: the stack's first effect, its return connection, its return
// volume; sub_823CC068, sub_823CC088, sub_823CBF58, sub_823CBFD0) of which slot 2 is never used, and two sends per
// track of the song player (sub_82168AF0, sub_82168C10, sub_82168DD0).
//
// With the option, the boot creates World Tour's stack on slot 2 right after the venue's (the hook in
// CreateEffectSendStack), and a player's Star Power sends that player's tracks to it with World Tour's settings instead
// of raising the venue's send (the hook at sub_821669D8, the one place both scripts' SongSetVenueEffectSendVolume
// player = ... reaches). The engine's own script functions apply everything: CreateEffectSendStack (sub_823B6618) and
// SetSoundBussEffects (sub_823B6530), given QB structs written in guest memory as a script would hand them, and the
// track's send (sub_82168AF0). The engine applied a send at once with the next slot's volume, and cleared a byte of the
// next slot's send (fixed below for the tracks slot 2 sends, with the option on: for slot 2 that byte is past the
// track); slot 2's sends are cut at the song player's clean-up and removed by a track's destruction, and slot 2 follows
// the pause. The game's code is called from the hooks on the thread's own context, below its live frames, the context
// handed back as it was after (LiveContext, GuestCall), and only after every pointer it is given was found in the
// game's memory. Anything not as expected leaves the game's own behaviour, with a line in the log.
//
// The addresses and offsets are GH Warriors of Rock's translated to Band Hero's (every function compared instruction by
// instruction; Band Hero's song player is a static object, its tracks 112 bytes, its sends 24, and its effect pool a stack of
// free pointers instead of a free list). Checked in GH Warriors of Rock on 2026-11-15: there the first run found the
// echo's delay worked out from 60000 / the beat (the cfunc's tempo) instead of the beat (fixed in EchoDelay), and
// testing builds then showed the stack as World Tour's: wired send -> echo -> sfxreverb -> highpass -> the master, each
// DSP active, World Tour's settings reached, the echo at 1.5 beats and the 3.24 s reverb measured in the sound. With
// World Tour's single guitar settings the stack's sound was about 13 dB under that game's whole mix, and
// ghbandhero_star_power_reverb_send_db and ghbandhero_star_power_reverb_multi_preset let the user raise it. Not yet run in Band Hero.

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/memory/utils.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/thread_state.h>
#include <rex/system/xmemory.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

REXCVAR_DEFINE_BOOL(ghbandhero_star_power_reverb, false, "GHBandHero",
                    "Guitar Hero World Tour's Star Power sound: while a player's Star Power is on, that player's "
                    "instrument goes through an echo timed to the song, a reverb and a high pass, with World Tour's "
                    "settings for each instrument; false = as on the console (Star Power raises the player's send to "
                    "the venue's reverb)");
REXCVAR_DEFINE_DOUBLE(ghbandhero_star_power_reverb_send_db, 0.0, "GHBandHero",
                      "With ghbandhero_star_power_reverb: dB added to World Tour's send of the player's instrument into the "
                      "echo and the reverb (0 = World Tour's level; +4 to +8 for more of the effect; the send stops at "
                      "4x, +12 dB, the engine's limit)");
REXCVAR_DEFINE_BOOL(ghbandhero_star_power_reverb_multi_preset, false, "GHBandHero",
                    "With ghbandhero_star_power_reverb: one player's Star Power takes World Tour's settings for several "
                    "players (no echo, a large 3 s reverb) instead of the single player ones; false = World Tour's");

// The game's functions called from the hooks (REX_IMPORT, rex/hook.h: the generated code's own __imp__ symbols, called
// with an explicit context, GuestCall below). The two script functions do not read r4 (the script): 0 is passed.
REX_IMPORT(__imp__sub_823B6618, CreateEffectSendStack, uint32_t(uint32_t, uint32_t));  // r3 = the struct
REX_IMPORT(__imp__sub_823B6530, SetSoundBussEffects, uint32_t(uint32_t, uint32_t));    // r3 = the struct
REX_IMPORT(__imp__sub_82168AF0, SetTrackSend, uint32_t(uint32_t, uint32_t, float, float));  // track, slot, vol, s
REX_IMPORT(__imp__sub_823CBFD0, MuteSendReturn, uint32_t(uint32_t, uint32_t));              // slot, mute

namespace {

// --- The engine's data, as the functions named read it ---

constexpr uint32_t kSlot = 2;       // the send stack slot used here: the engine's second, free in this game
constexpr uint32_t kVenueSlot = 1;  // venue_sendeffect_slot (0xE0EC7ED6), read into [SAP + 56] by sub_82167B20
constexpr uint32_t kSlotTable = 0x82D80330;  // the send stack slots, 12 bytes each from slot 1 (two: the next
                                             // variable is at 0x82D80348)
constexpr uint32_t kVenueStack = kSlotTable;           // slot 1's first effect (0: not created)
constexpr uint32_t kSlot2Stack = kSlotTable + 12;      // slot 2's first effect (0: not created)
constexpr uint32_t kSlot2Return = kSlotTable + 16;     // slot 2's return connection (sub_823CBE40, sub_823CBFD0)
// The effects' pool: a pointer to it at 0x82D80320 (made by sub_823CAED8 at the sound system's init, sub_823CB678), a
// 24-byte object: +0 the effects' memory (capacity x 544 bytes), +4 an array of pointers to the free effects, +8 the
// effects taken now, +12 the capacity (max_sound_effects, 64). sub_823CB970 takes the pointer at +4 [+8] and adds 1
// to +8 while +8 is below +12; full, it goes on with no effect (r31 = 0) and writes through it (from 0x823CB9E4).
// sub_823CB8E0 gives effects back (+8 less 1, the pointer stored at +4 [+8]). It keeps no count of the most taken at
// once: GHBandHeroStarPowerPoolHook follows it.
constexpr uint32_t kEffectPoolPointer = 0x82D80320;
constexpr uint32_t kPoolObjectSize = 24;
constexpr uint32_t kPoolPointers = 4;
constexpr uint32_t kPoolTaken = 8;
constexpr uint32_t kPoolCapacity = 12;
// An effect (sub_823CB970): +0 its id, +4 its parameters (sub_823CAD08; their +0 is its name, sub_823CA900), +532 the
// next one in its stack (sub_823CBA20), +536 its FMOD DSP (sub_823CBCB0; 0 when FMOD did not create it).
constexpr uint32_t kEffectSize = 544;
constexpr uint32_t kEffectName = 4;
constexpr uint32_t kEffectNext = 532;
constexpr uint32_t kEffectDsp = 536;
// The players (sub_822466C8, sub_822466B0): their count and their infos, player 1 first; +108 is 1 for a player who
// plays (isSinglePlayerGame counts them with sub_822466C8(0, 0, 0), as GH Warriors of Rock's sub_82174E48 does; Band Hero's
// own script function was not located), +232 the length of the song's beat in ms, an integer (player 1's:
// get_song_tempo_cfunc 0x821B5678 hands it to the script as beat_duration, 0x0EBB7064, and 60000 / it as tempo,
// 0x8B4A1490, through sub_821B55E8; World Tour's 0x82181F10 and 0x82181E50 do the same from their own field).
constexpr uint32_t kPlayerCount = 0x82CE9230;
constexpr uint32_t kPlayerTable = 0x82CE9234;
constexpr uint32_t kPlayerPlays = 116;  // GH5: +108 (Band Hero's player info has 8 more bytes before it)
constexpr uint32_t kPlayerBeat = 240;   // GH5: +232
// The song player (SAP: a static object at 0x82CC8440 in the image's data, the r3 SongSetVenueEffectSendVolume's
// sub_82163CC0 hands sub_821669D8) as sub_821669D8 reads it: +56 the venue's slot (-1 before its init), +1564+2924 its
// state (sub_82161A00; no send in states 1 and 5), +1564+2948 its players, 40 bytes each (sub_821619B8): +0 the part
// (sub_82163FE8: 1 guitar, 2 bass, 4 drums, 8 vocals), +16 the player's stem object (one of four, 328 bytes, at SAP +
// 1564 + 1604; sub_82161C28), +32 a byte, 0 for no player. The drums are the SAP's own four tracks.
constexpr uint32_t kSapVenueSlot = 56;
constexpr uint32_t kSapState = 1564 + 2924;
constexpr uint32_t kSapPlayers = 1564 + 2948;
constexpr uint32_t kSapPlayerSize = 40;
constexpr uint32_t kSapSize = kSapPlayers + 4 * kSapPlayerSize;
constexpr uint32_t kPlayerPart = 0;
constexpr uint32_t kPlayerStem = 16;
constexpr uint32_t kPlayerActive = 32;
constexpr uint32_t kStemTrack = 76;  // the stem object's track that sub_821669D8 sends (+188 is the other one)
constexpr std::array<uint32_t, 4> kSapDrumTracks = {884, 548, 772, 660};  // kick, snare, tom, cymbal (sub_82166B08)
constexpr int32_t kPartGuitar = 1;
constexpr int32_t kPartBass = 2;
constexpr int32_t kPartDrums = 4;
// A track (sub_82168F98, 112 bytes; the SAP's at +100 to +1444, 112 apart, a stem's at +76 and +188): +0 its own
// volume (a 24-byte ramp like a send's), +24 and +28 the first and the last effect of its own chain (sub_82168DD0
// destroys the chain from +24; 0 before they are created and after its destruction, sub_82168AF0 then returning at
// once; the last one's DSP, +536, feeds its outputs and its sends), +32 and +36 its first output's DSP and the
// connection to it (sub_82168600; sub_82168D08 is whether it has one), +48 and +52 its connections to slot 1's and slot
// 2's stacks, +56 and +80 the two slots' sends (24 bytes each: +0 the volume now, +4 the volume a ramp starts from, +8
// its target, +12 and +16 its start and end in ms, +20 a byte set when the send is set and cleared once applied, +21
// set while it ramps, +22 set once the send is set, which the frame's update sub_82168C10 then applies, on the tracks
// it is called for).
constexpr uint32_t kTrackSize = 112;
constexpr uint32_t kTrackLastEffect = 28;
constexpr uint32_t kTrackOutput = 36;
constexpr uint32_t kTrackSends = 56;
constexpr uint32_t kSendSize = 24;
constexpr uint32_t kTrackSlot2Volume = kTrackSends + kSendSize;  // slot 2's send, its volume at +0

// A call's frames start this far below the live r1 (rex::ppc::ImportFunction's isolated call uses 0x70, an APC's
// FunctionDispatcher::Execute 176): the guest callee's parameter area stays clear of the live frame.
constexpr uint32_t kCallFrame = 0x80;

// Lines logged of each kind that repeats.
constexpr uint32_t kLoggedFallbacks = 20;
constexpr uint32_t kLoggedSends = 100;
constexpr uint32_t kLoggedCleanups = 20;

bool Enabled() {
  static const bool enabled = REXCVAR_GET(ghbandhero_star_power_reverb);
  return enabled;
}

// The two settings for the user (read once at the start).
double SendDbKnob() {
  static const double db = REXCVAR_GET(ghbandhero_star_power_reverb_send_db);
  return db;
}
bool MultiPresetKnob() {
  static const bool multi = REXCVAR_GET(ghbandhero_star_power_reverb_multi_preset);
  return multi;
}

// --- Guest memory ---

rex::memory::Memory* GuestMemory() {
  auto* kernel = rex::system::kernel_state();
  return kernel ? kernel->memory() : nullptr;
}

// Whether [address, address + size) is the game's memory, readable (and writable when asked), all in one heap. The
// image's data pages (the SAP, the players' table, the slot table) are read-write (xex_module.cpp: XEX_SECTION_DATA).
bool GuestRange(uint32_t address, uint32_t size, bool writable) {
  rex::memory::Memory* memory = GuestMemory();
  if (!memory || address == 0 || size == 0 || address > UINT32_MAX - (size - 1)) {
    return false;
  }
  rex::memory::BaseHeap* heap = memory->LookupHeap(address);
  if (!heap) {
    return false;
  }
  const rex::memory::PageAccess access = heap->QueryRangeAccess(address, address + size - 1);
  return writable ? access == rex::memory::PageAccess::kReadWrite : access != rex::memory::PageAccess::kNoAccess;
}

uint32_t Load32(uint32_t address) {
  uint32_t value;
  std::memcpy(&value, GuestMemory()->TranslateVirtual(address), 4);
  return __builtin_bswap32(value);
}

uint8_t Load8(uint32_t address) { return *GuestMemory()->TranslateVirtual(address); }

void Store32(uint32_t address, uint32_t value) {
  value = __builtin_bswap32(value);
  std::memcpy(GuestMemory()->TranslateVirtual(address), &value, 4);
}

// --- Calling the game's code from a hook ---

// The thread's own guest context, when it is the one the hook runs in (the hook's r1 is that context's r1): the game's
// code is then called on it, below the live frames. Null otherwise (a context the SDK made for a call of its own), and
// the game's code is not called.
PPCContext* LiveContext(PPCRegister& r1) {
  rex::runtime::ThreadState* thread = rex::runtime::ThreadState::Get();
  PPCContext* ctx = thread ? thread->context() : nullptr;
  return ctx && &ctx->r1 == &r1 && r1.u32 > kCallFrame ? ctx : nullptr;
}

// A call of the game's code from a hook, made as the SDK's FunctionDispatcher::ExecuteTrap makes an APC's: on the
// thread's own context, its frames kCallFrame below the live r1, and the whole context with its floating-point state
// handed back as it was once the call object is gone. The thread's r1 so follows the callee, and what the SDK may run
// on the thread meanwhile from that context (an APC in an alertable wait, at r1 - 176) starts below the callee's
// frames instead of over them. The hook's registers are the context's: they hold the callee's values until then.
class GuestCall {
 public:
  explicit GuestCall(PPCContext& live) : live_(live), saved_(live) {}
  ~GuestCall() {
    live_ = saved_;
    live_.fpscr.restoreGuestBits(saved_.fpscr.csr);
  }
  GuestCall(const GuestCall&) = delete;
  GuestCall& operator=(const GuestCall&) = delete;

  template <typename Function, typename... Args>
  uint32_t operator()(const Function& function, Args... args) {
    live_.r1.u64 = saved_.r1.u32 - kCallFrame;
    return function(live_, GuestMemory()->virtual_membase(), args...);
  }

 private:
  PPCContext& live_;
  PPCContext saved_;
};

// --- QB structs in guest memory ---

// A QB key: the CRC-32 of the name in lower case, without the final inversion.
constexpr uint32_t QbKey(std::string_view name) {
  uint32_t crc = 0xFFFFFFFF;
  for (char c : name) {
    crc ^= uint8_t(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
    }
  }
  return crc;
}
static_assert(QbKey("effects") == 0xB7F4C355 && QbKey("slot") == 0x53F1DF98 && QbKey("time") == 0x906B67BA);
static_assert(QbKey("starpowerecho") == 0x8ED352EE && QbKey("starpowerreverb") == 0xD7EF1990);
static_assert(QbKey("starpowerhighpass") == 0x6ABADB4C && QbKey("HighPass") == 0xD47A9AF8);

// A component of a QB struct: an integer, a float, a name (its key) or an array of structs. The types are the byte +2
// of a component (sub_823A72D8, which also reads an integer where a float is asked for).
struct QbField;
using QbStruct = std::vector<QbField>;
struct QbField {
  enum class Type : uint32_t { kInteger = 1, kFloat = 2, kArray = 12, kName = 13 };
  uint32_t key;
  Type type;
  uint32_t value = 0;
  std::vector<QbStruct> structs{};
  uint32_t* address = nullptr;  // receives the guest address of the value written
};

QbField Int(std::string_view name, int32_t value) { return {QbKey(name), QbField::Type::kInteger, uint32_t(value)}; }
QbField Float(std::string_view name, float value, uint32_t* address = nullptr) {
  return {QbKey(name), QbField::Type::kFloat, std::bit_cast<uint32_t>(value), {}, address};
}
QbField Name(std::string_view name, std::string_view value) {
  return {QbKey(name), QbField::Type::kName, QbKey(value)};
}
QbField Structs(std::string_view name, std::vector<QbStruct> structs) {
  return {QbKey(name), QbField::Type::kArray, 0, std::move(structs)};
}

// Writes QB structs into a block of guest memory as the script compiler lays out a struct in a script (token 0x4A),
// each struct's header first, then its components in order, an array's structs after its own component, with its
// pointers already absolute (what the engine's sub_823A6AC8 makes of a script's struct when it loads it): a struct is
// 0x00000100 and its first component; a component is 0x0001TT00 (TT its type), its key, its value and the next
// component (0 for the last); an array is 0x00010A00, its count and its one struct or the list of them.
class QbWriter {
 public:
  QbWriter(uint32_t start, uint32_t size) : next_(start), end_(start + size) {}

  uint32_t Struct(const QbStruct& fields) {
    const uint32_t header = Alloc(8);
    Store(header, 0x00000100);
    uint32_t link = header + 4;
    for (const QbField& field : fields) {
      const uint32_t component = Alloc(16);
      Store(link, component);
      link = component + 12;
      Store(component, 0x00010000 | (uint32_t(field.type) << 8));
      Store(component + 4, field.key);
      if (field.type != QbField::Type::kArray) {
        Store(component + 8, field.value);
        if (field.address) {
          *field.address = component + 8;
        }
        continue;
      }
      const uint32_t array = Alloc(12);
      Store(component + 8, array);
      Store(array, 0x00010A00);
      Store(array + 4, uint32_t(field.structs.size()));
      if (field.structs.size() == 1) {
        Store(array + 8, Struct(field.structs[0]));
      } else {
        const uint32_t list = Alloc(4 * uint32_t(field.structs.size()));
        Store(array + 8, list);
        for (size_t i = 0; i < field.structs.size(); ++i) {
          Store(list + 4 * uint32_t(i), Struct(field.structs[i]));
        }
      }
    }
    return failed_ ? 0 : header;
  }

  bool failed() const { return failed_; }

 private:
  uint32_t Alloc(uint32_t size) {
    if (failed_ || end_ - next_ < size) {
      failed_ = true;
      return 0;
    }
    const uint32_t address = next_;
    next_ += size;
    return address;
  }
  void Store(uint32_t address, uint32_t value) {
    if (!failed_) {
      Store32(address, value);
    }
  }

  uint32_t next_;
  uint32_t end_;
  bool failed_ = false;
};

// World Tour's settings, in GH_Star_Power_Verb_On's words (World Tour's scripts\game\audio\global_sound_logic.qb,
// 0xDF84B375), some of its floats written as integers (the same values: sub_823A72D8 reads an integer where a float is
// asked for).
enum Echo { kEchoGuitarSingle, kEchoBass, kEchoOther, kEchoCount };
enum Reverb { kReverbGuitarSingle, kReverbBass, kReverbDrumsSingle, kReverbMulti, kReverbCount };

// setsoundbusseffects {effects = [{effect = echo name = starpowerecho WetMix drymix DecayRatio}]}
QbStruct EchoMix(QbField wet, QbField dry, QbField decay) {
  return {Structs("effects", {{Name("effect", "echo"), Name("name", "starpowerecho"), wet, dry, decay}})};
}

// setsoundbusseffects {effects = [{effect = sfxreverb name = starpowerreverb DryLevel ReverbLevel Reflectionslevel
// Room RoomRollOffFactor = 18 ReverbDelay = 0.1 DecayTime RoomHF}] time = 0.1}
QbStruct ReverbMix(int32_t dry, int32_t reverb, int32_t reflections, int32_t room, QbField decay_time,
                   int32_t room_hf) {
  return {Structs("effects", {{Name("effect", "sfxreverb"), Name("name", "starpowerreverb"), Int("drylevel", dry),
                               Int("reverblevel", reverb), Int("reflectionslevel", reflections), Int("room", room),
                               Int("roomrollofffactor", 18), Float("reverbdelay", 0.1f), decay_time,
                               Int("roomhf", room_hf)}}),
          Float("time", 0.1f)};
}

// The structs in guest memory (one block from the SDK's system heap, which zeroes it: a struct's last component links
// to 0; written once at the boot).
struct Blobs {
  uint32_t create = 0;
  uint32_t echo_delay = 0;
  uint32_t echo_delay_value = 0;  // the delay's float, written before each Star Power
  std::array<uint32_t, kEchoCount> echo{};
  std::array<uint32_t, kReverbCount> reverb{};
};
Blobs g_blobs;
constexpr uint32_t kBlobBlockSize = 0x1000;  // the nine structs take 1848 bytes

bool WriteBlobs() {
  rex::memory::Memory* memory = GuestMemory();
  const uint32_t block = memory ? memory->SystemHeapAlloc(kBlobBlockSize) : 0;
  if (!block || !GuestRange(block, kBlobBlockSize, true)) {
    return false;
  }
  QbWriter writer(block, kBlobBlockSize);
  Blobs blobs;
  // World Tour's preload_song: CreateEffectSendStack slot = $starpower_sendeffect_slot effects = [{$Starpower_Echo}
  // {$Starpower_Reverb} {$HighPass_StarPower}] (audio_effects_settings.qb's structs, with their names; Band Hero's copies of
  // the three structs have the same values), on slot 2.
  blobs.create = writer.Struct(
      {Int("slot", int32_t(kSlot)),
       Structs("effects",
               {{Name("effect", "echo"), Name("name", "starpowerecho"), Int("delay", 600), Float("decayratio", 0.15f),
                 Int("maxchannels", 6), Int("drymix", 1), Int("wetmix", 1)},
                {Name("effect", "sfxreverb"), Name("name", "starpowerreverb"), Int("drylevel", -10000),
                 Int("room", -100), Int("roomhf", -698), Int("roomrollofffactor", 10), Float("decaytime", 3.24f),
                 Float("decayhfratio", 0.33f), Int("reflectionslevel", -1166), Float("reflectionsdelay", 0.02f),
                 Int("reverblevel", 18), Float("reverbdelay", 0.05f), Int("diffusion", 100), Int("density", 100),
                 Int("hfreference", 5000)},
                {Name("effect", "HighPass"), Name("name", "starpowerhighpass"), Int("cutoff", 250),
                 Int("resonance", 1)}})});
  // setsoundbusseffects effects = [{effect = echo name = starpowerecho delay = <beat_duration>}]
  blobs.echo_delay = writer.Struct({Structs(
      "effects", {{Name("effect", "echo"), Name("name", "starpowerecho"),
                   Float("delay", 600.0f, &blobs.echo_delay_value)}})});
  blobs.echo[kEchoGuitarSingle] = writer.Struct(EchoMix(Int("wetmix", 1), Float("drymix", 0.7f),
                                                        Float("decayratio", 0.25f)));
  blobs.echo[kEchoBass] = writer.Struct(EchoMix(Float("wetmix", 0.3f), Float("drymix", 0.7f),
                                                Float("decayratio", 0.25f)));
  blobs.echo[kEchoOther] = writer.Struct(EchoMix(Int("wetmix", 0), Float("drymix", 0.2f), Int("decayratio", 0)));
  blobs.reverb[kReverbGuitarSingle] = writer.Struct(ReverbMix(-1500, -500, 0, -1000, Float("decaytime", 0.1f), -1));
  blobs.reverb[kReverbBass] = writer.Struct(ReverbMix(-1400, 900, 0, -2000, Float("decaytime", 0.1f), -1));
  blobs.reverb[kReverbDrumsSingle] = writer.Struct(ReverbMix(-2000, 1700, -10000, -2000, Int("decaytime", 3), -1));
  blobs.reverb[kReverbMulti] = writer.Struct(ReverbMix(-1000, 2000, 1000, -2000, Int("decaytime", 3), -500));
  if (writer.failed() || !blobs.echo_delay_value) {
    return false;
  }
  g_blobs = blobs;
  return true;
}

// --- World Tour's presets (GH_Star_Power_Verb_On) ---

struct Preset {
  const char* name;
  Echo echo;
  Reverb reverb;
  float send_db;  // SongSetStarpowerEffectsSendVolume vol
};
// The first activation (Star_power_verb_is_on = 0), single player.
constexpr Preset kGuitarSingle{"guitar, single player", kEchoGuitarSingle, kReverbGuitarSingle, 4.0f};
constexpr Preset kBassSingle{"bass, single player", kEchoBass, kReverbBass, 8.0f};
constexpr Preset kDrumsSingle{"drums, single player", kEchoOther, kReverbDrumsSingle, 12.0f};
// The first activation, multiplayer.
constexpr Preset kGuitarMulti{"guitar, multiplayer", kEchoOther, kReverbMulti, 6.0f};
constexpr Preset kBassMulti{"bass, multiplayer", kEchoOther, kReverbMulti, 8.0f};
constexpr Preset kDrumsMulti{"drums, multiplayer", kEchoOther, kReverbMulti, 7.0f};
// Star_power_verb_is_on already 1, multiplayer (the switch at the script's start; single player returns there).
constexpr Preset kGuitarAgain{"guitar, multiplayer, already on", kEchoOther, kReverbMulti, 6.0f};
constexpr Preset kBassAgain{"bass, multiplayer, already on", kEchoBass, kReverbBass, 8.0f};
constexpr Preset kDrumsAgain{"drums, multiplayer, already on", kEchoOther, kReverbMulti, 7.0f};

const Preset& PresetFor(int32_t part, bool single, bool already_on) {
  if (already_on) {
    return part == kPartGuitar ? kGuitarAgain : part == kPartBass ? kBassAgain : kDrumsAgain;
  }
  if (single) {
    return part == kPartGuitar ? kGuitarSingle : part == kPartBass ? kBassSingle : kDrumsSingle;
  }
  return part == kPartGuitar ? kGuitarMulti : part == kPartBass ? kBassMulti : kDrumsMulti;
}

// World Tour's dB to the send's volume (its sub_820E2378): 2^(dB / 6), at most 4.
float SendVolume(float db) { return std::fmin(std::exp2(db / 6.0f), 4.0f); }

// The echo's delay in ms (GH_Star_Power_Verb_On, from get_song_tempo_cfunc's beat_duration, player 1's beat in ms):
// 1.5 beats if that is 600 ms or less, else the beat halved while above 400 ms, three times at most, and 400 at most.
// The halving is done on floats here (the script's integer, if it halves it as one, differs by half a ms at most). 0
// when the beat is 0 or less (World Tour's script would then ask for a delay of 0, below the echo's 10 ms: the delay
// is left as it was). False when player 1's info is not the game's memory.
bool EchoDelay(float* delay_ms, int32_t* beat_out) {
  const uint32_t table = Load32(kPlayerTable);
  if (!GuestRange(table, 4, false)) {
    return false;
  }
  const uint32_t player1 = Load32(table);
  if (!GuestRange(player1, kPlayerBeat + 4, false)) {
    return false;
  }
  const int32_t beat_ms = int32_t(Load32(player1 + kPlayerBeat));
  *beat_out = beat_ms;
  if (beat_ms <= 0) {
    *delay_ms = 0.0f;
    return true;
  }
  float beat = float(beat_ms);
  const float one_and_a_half = beat * 1.5f;
  if (one_and_a_half > 600.0f) {
    for (int i = 0; i < 3; ++i) {
      if (beat > 400.0f) {
        beat = beat / 2.0f;
      }
    }
    if (beat > 400.0f) {
      beat = 400.0f;
    }
  } else {
    beat = one_and_a_half;
  }
  *delay_ms = beat;
  return true;
}

// isSinglePlayerGame (sub_822466C8(0, 0, 0) == 1): one player who plays. Empty when the players' table is not the
// game's memory.
std::optional<bool> SinglePlayer() {
  const int32_t count = int32_t(Load32(kPlayerCount));
  if (count <= 0) {
    return false;
  }
  const uint32_t table = Load32(kPlayerTable);
  if (count > 8 || !GuestRange(table, 4 * uint32_t(count), false)) {
    return std::nullopt;
  }
  int playing = 0;
  for (int32_t i = 0; i < count; ++i) {
    const uint32_t player = Load32(table + 4 * uint32_t(i));
    if (!GuestRange(player, kPlayerPlays + 4, false)) {
      return std::nullopt;
    }
    playing += Load32(player + kPlayerPlays) == 1 ? 1 : 0;
  }
  return playing == 1;
}

// --- The players' tracks ---

struct Tracks {
  std::array<uint32_t, 4> address{};
  uint32_t count = 0;
  int32_t part = 0;
};

// The tracks sub_821669D8 sends for `player` (1 to 4), chosen as it chooses them (0x821669EC to 0x82166AEC): none in
// the SAP's states 1 and 5, before its init, for a player not there, or for vocals; the stem object's track for
// guitar and bass while it has its output (sub_82168D08); the SAP's four drum tracks for drums. None either where a
// pointer is not the game's memory.
Tracks PlayerTracks(uint32_t sap, uint32_t player) {
  Tracks tracks;
  if (player < 1 || player > 4 || !GuestRange(sap, kSapSize, true)) {
    return tracks;
  }
  const uint32_t state = Load32(sap + kSapState);
  if (state == 5 || state == 1 || int32_t(Load32(sap + kSapVenueSlot)) == -1) {
    return tracks;
  }
  const uint32_t info = sap + kSapPlayers + kSapPlayerSize * (player - 1);
  if (Load8(info + kPlayerActive) == 0) {
    return tracks;
  }
  tracks.part = int32_t(Load32(info + kPlayerPart));
  if (tracks.part == kPartGuitar || tracks.part == kPartBass) {
    const uint32_t stem = Load32(info + kPlayerStem);
    const uint32_t track = stem + kStemTrack;
    if (stem && GuestRange(track, kTrackSize, true) && Load32(track + kTrackOutput) != 0) {
      tracks.address[tracks.count++] = track;
    }
  } else if (tracks.part == kPartDrums) {
    for (uint32_t offset : kSapDrumTracks) {
      tracks.address[tracks.count++] = sap + offset;
    }
  }
  return tracks;
}

// Whether a track is the game's memory and has its effect chain (sub_82168AF0 sets nothing on one without it, as
// after the track's destruction).
bool TrackLive(uint32_t track) {
  return GuestRange(track, kTrackSize, true) && Load32(track + kTrackLastEffect) != 0;
}

const char* PartName(int32_t part) {
  return part == kPartGuitar ? "guitar" : part == kPartBass ? "bass" : part == kPartDrums ? "drums" : "other";
}

// --- State ---

std::atomic<bool> g_stack_tried{false};
std::atomic<bool> g_ready{false};       // the stack on slot 2 is there, its three effects with their DSPs
std::atomic<bool> g_tail_muted{false};  // slot 2's return muted by the song's clean-up, until the next Star Power

// Under g_mutex, never held while the game's code runs.
std::mutex g_mutex;
std::array<std::vector<uint32_t>, 4> g_sent;  // each player's tracks sent to slot 2
bool g_verb_on = false;                        // World Tour's Star_power_verb_is_on

std::atomic<uint32_t> g_fallbacks{0};
std::atomic<uint32_t> g_ons{0};
std::atomic<uint32_t> g_offs{0};
std::atomic<uint32_t> g_cleanups{0};

void LogFallback(std::string_view what) {
  const uint32_t n = g_fallbacks.fetch_add(1, std::memory_order_relaxed);
  if (n < kLoggedFallbacks) {
    REXLOG_WARN("[star power reverb] {}: the game's own behaviour{}", what,
                n + 1 == kLoggedFallbacks ? " (the next such lines are not logged)" : "");
  }
}

// --- The effect pool ---

// The pool's object, its effects taken now and its capacity. False when the pointer is not the game's memory or the
// counts are not a pool's.
bool PoolCounts(uint32_t* pool, uint32_t* taken, uint32_t* capacity) {
  *pool = Load32(kEffectPoolPointer);
  if (!GuestRange(*pool, kPoolObjectSize, false)) {
    return false;
  }
  *taken = Load32(*pool + kPoolTaken);
  *capacity = Load32(*pool + kPoolCapacity);
  return *capacity <= 4096 && *taken <= *capacity;
}

// Whether the effect pool can give `count` more effects: sub_823CB970's allocation, simulated (the free pointers from
// [+4] [+8] on, each an effect in the game's memory). `free_effects` gets how many it has free.
bool PoolHasRoom(uint32_t count, uint32_t* free_effects) {
  *free_effects = 0;
  uint32_t pool = 0;
  uint32_t taken = 0;
  uint32_t capacity = 0;
  if (!PoolCounts(&pool, &taken, &capacity)) {
    return false;
  }
  *free_effects = capacity - taken;
  if (*free_effects < count) {
    return false;
  }
  const uint32_t pointers = Load32(pool + kPoolPointers);
  if (!GuestRange(pointers + 4 * taken, 4 * count, false)) {
    return false;
  }
  for (uint32_t i = 0; i < count; ++i) {
    if (!GuestRange(Load32(pointers + 4 * (taken + i)), kEffectSize, true)) {
      return false;
    }
  }
  return true;
}

// The stack on slot 2 as sub_823CBE40 left it: three effects named as asked, each with its DSP, and the return
// connection. Null when it is so; else what is missing.
const char* CheckStack() {
  static constexpr std::array<std::pair<uint32_t, const char*>, 3> kEffects = {{
      {QbKey("starpowerecho"), "the echo (starpowerecho)"},
      {QbKey("starpowerreverb"), "the reverb (starpowerreverb)"},
      {QbKey("starpowerhighpass"), "the high pass (starpowerhighpass)"},
  }};
  uint32_t effect = Load32(kSlot2Stack);
  for (const auto& [name, what] : kEffects) {
    if (!GuestRange(effect, kEffectSize, false) || Load32(effect + kEffectName) != name) {
      return what;
    }
    if (Load32(effect + kEffectDsp) == 0) {
      return "a DSP from FMOD (its memory pool, fmod_memory_size, may be full)";
    }
    effect = Load32(effect + kEffectNext);
  }
  if (effect != 0) {
    return "the end of the stack after its three effects";
  }
  if (Load32(kSlot2Return) == 0) {
    return "the stack's return connection";
  }
  return nullptr;
}

// Slot 2's send cut on `tracks` (those still with their effect chain: a destroyed track's destruction removed its
// connections), at once. The count of those cut.
uint32_t CutSends(GuestCall& call, const std::vector<uint32_t>& tracks) {
  uint32_t cut = 0;
  for (uint32_t track : tracks) {
    if (TrackLive(track)) {
      call(SetTrackSend, track, kSlot, 0.0f, 0.0f);
      ++cut;
    }
  }
  return cut;
}

// The effect pool's peak. The pool keeps no count of the most taken at once, so GHBandHeroStarPowerPoolHook follows every
// effect taken from the boot on (World Tour's stack included), and the effects taken now ([pool + 8]) are added at each
// check. The boot's check covers only the stack's own three effects: the song player takes its effects later (at its
// init and at each song's load: a mixer for each track, its effect chains, two more for each guitar or bass player,
// sub_82167B20 and sub_82167538) and the sounds and busses take theirs from the same pool, while with the pool full
// the engine writes through a null effect (sub_823CB970 from 0x823CB9E4). A warning each time the peak rises with
// kPoolMargin effects or fewer left: three more taken by the option are then what a bigger game could miss.
constexpr uint32_t kPoolMargin = 3;
std::atomic<uint32_t> g_pool_peak{0};    // the most effects taken at once seen
std::atomic<uint32_t> g_pool_warned{0};  // the peak the last check saw

void NotePoolTaken(uint32_t taken) {
  uint32_t peak = g_pool_peak.load(std::memory_order_relaxed);
  while (taken > peak && !g_pool_peak.compare_exchange_weak(peak, taken, std::memory_order_relaxed)) {
  }
}

void LogPoolPeak() {
  uint32_t pool = 0;
  uint32_t taken = 0;
  uint32_t capacity = 0;
  if (!PoolCounts(&pool, &taken, &capacity)) {
    return;
  }
  NotePoolTaken(taken);
  const uint32_t peak = g_pool_peak.load(std::memory_order_relaxed);
  uint32_t seen = g_pool_warned.load(std::memory_order_relaxed);
  do {
    if (peak <= seen) {
      return;
    }
  } while (!g_pool_warned.compare_exchange_weak(seen, peak, std::memory_order_relaxed));
  const uint32_t left = capacity > peak ? capacity - peak : 0;
  if (left <= kPoolMargin) {
    REXLOG_WARN("[star power reverb] the effect pool's peak: {} of {} effects taken at once (3 of them World Tour's "
                "stack), {} left: a game with more players may find it full with the option on (a crash); turn "
                "ghbandhero_star_power_reverb off if it does",
                peak, capacity, left);
  }
}

}  // namespace

// [[midasm_hook]] 0x823CB9B0 (sub_823CB970, an effect taken from the pool: stw r7,8(r11), r7 = the effects taken now,
// on the path that found a free one; not a branch target). Only follows the pool's peak for LogPoolPeak (Band Hero's pool
// keeps no count of the most taken at once); it changes nothing.
void GHBandHeroStarPowerPoolHook(PPCRegister& r7) {
  if (!Enabled()) {
    return;
  }
  NotePoolTaken(r7.u32);
}

// [[midasm_hook]] 0x823B6698 (sub_823B6618, the script function CreateEffectSendStack: after sub_823CC068 created the
// stack, r30 = the slot). The boot's Create_Venue_Effects_Settings (from guitar_startup_spawned) creates the venue's
// stack on slot 1 here, once: World Tour's stack is created on slot 2 right after it, through the same script function
// (its own call comes back here with r30 = 2). Before it, the effect pool must have three free effects (full, the
// engine writes through a null effect), which covers the stack's own effects only: the song player takes its own later,
// and the pool's peak is followed from then on (LogPoolPeak). After it, each effect must have its DSP (FMOD's own pool
// has no check before). Anything else leaves the option off for the session, with the reason in the log.
void GHBandHeroStarPowerStackHook(PPCRegister& r1, PPCRegister& r30) {
  if (!Enabled() || r30.u32 != kVenueSlot || g_stack_tried.exchange(true)) {
    return;
  }
  uint32_t free_effects = 0;
  const char* why = nullptr;
  PPCContext* live = LiveContext(r1);
  if (!live) {
    why = "the hook did not run in its thread's own context";
  } else if (Load32(kVenueStack) == 0) {
    why = "the venue's stack (slot 1) was not created";
  } else if (Load32(kSlot2Stack) != 0) {
    why = "slot 2 already has a stack";
  } else if (!PoolHasRoom(3, &free_effects)) {
    why = "the effect pool (max_sound_effects) has fewer than 3 free effects";
  } else if (!WriteBlobs()) {
    why = "no guest memory for the QB structs";
  }
  if (why) {
    REXLOG_WARN("[star power reverb] no stack on slot 2: {} ({} effects free); the option stays off (the game's own "
                "Star Power sound)",
                why, free_effects);
    return;
  }
  {
    GuestCall call(*live);
    call(CreateEffectSendStack, g_blobs.create, 0u);
  }
  const char* missing = Load32(kSlot2Stack) == 0 ? "everything: CreateEffectSendStack created nothing" : CheckStack();
  if (missing) {
    REXLOG_WARN("[star power reverb] the stack on slot 2 lacks {}: the option stays off (the game's own Star Power "
                "sound), the stack unused",
                missing);
    return;
  }
  uint32_t pool = 0;
  uint32_t taken = 0;
  uint32_t capacity = 0;
  if (PoolCounts(&pool, &taken, &capacity)) {
    NotePoolTaken(taken);
  }
  g_pool_warned.store(g_pool_peak.load(std::memory_order_relaxed), std::memory_order_relaxed);
  g_ready.store(true, std::memory_order_release);
  REXLOG_INFO("[star power reverb] World Tour's stack on slot 2: echo, sfxreverb, highpass (starpowerecho, "
              "starpowerreverb, starpowerhighpass, each with its DSP; {} effects were free before it, {} of {} taken "
              "now, {} at once so far); Star Power sends the player's tracks to it",
              free_effects, taken, capacity, g_pool_peak.load(std::memory_order_relaxed));
}

// [[midasm_hook]] 0x821669D8 (entry of sub_821669D8, return_on_true: r3 = the SAP, r4 = the player, 1 to 4, f1 = the
// send's volume; f2, its time in seconds, is not needed). Its one caller is SongSetVenueEffectSendVolume (0x82163CC0,
// at 0x82163D38, the call with a player), which the disc's scripts call with a player only from GH_Star_Power_Verb_On
// (vol = 0, -10, -3 dB; sub_82160BF8 makes them 2^(dB / 6): f1 = 1, 0.315, 0.707) and GH_Star_Power_Verb_Off (-100 dB:
// f1 = 0); every other script passes a track. The original sets the send of the venue's slot ([SAP + 56]) on the
// player's tracks and returns 1. Star Power on (f1 > 0): World Tour's GH_Star_Power_Verb_On instead, as it orders it:
// the echo's delay, the echo's mix, the reverb (time = 0.1), then slot 2's send on the player's tracks, at once; the
// venue's send is left as it was. Off: slot 2's send cut on the tracks the player's Star Power sent, then the original
// (the venue's send to 0, as on the console). With World Tour's flag: on sets it, off clears it, and an activation
// while it is set takes the "already on" presets in multiplayer and changes nothing in single player. The flag is the
// hook's own: Band Hero's script keeps World Tour's Star_power_verb_is_on, but sets it before it calls the send on a first
// activation, so it reads 1 here every time. It is set only by the ons that reach here with tracks to send: World Tour
// set it for any part, but a vocalist's on does not come here (GH_Star_Power_Verb_On only calls VocalDSPSetParams for
// vocals), while every part's off does. The echo's delay is left as it was for a beat of 0.
bool GHBandHeroStarPowerSendHook(PPCRegister& r1, PPCRegister& r3, PPCRegister& r4, PPCRegister& f1) {
  if (!Enabled()) {
    return false;
  }
  const uint32_t player = r4.u32;
  const bool on = f1.f64 > 0.0;
  if (!g_ready.load(std::memory_order_acquire)) {
    if (on) {
      LogFallback("Star Power on with no stack on slot 2 (the boot's line says why)");
    }
    return false;
  }
  PPCContext* live = LiveContext(r1);
  if (!live) {
    LogFallback("Star Power: the hook did not run in its thread's own context");
    return false;
  }
  if (!on) {
    std::vector<uint32_t> sent;
    {
      std::lock_guard lock(g_mutex);
      g_verb_on = false;
      if (player >= 1 && player <= 4) {
        sent.swap(g_sent[player - 1]);
      }
    }
    if (!sent.empty()) {
      GuestCall call(*live);
      const uint32_t cut = CutSends(call, sent);
      if (g_offs.fetch_add(1, std::memory_order_relaxed) < kLoggedSends) {
        REXLOG_INFO("[star power reverb] player {} off: slot 2's send cut on {} of the {} track{} sent{}", player,
                    cut, sent.size(), sent.size() == 1 ? "" : "s",
                    cut < sent.size() ? " (the others destroyed, their connections with them)" : "");
      }
    }
    return false;
  }

  const Tracks tracks = PlayerTracks(r3.u32, player);
  if (tracks.count == 0) {
    return false;  // the original sends nothing either (vocals, no player, the SAP not ready)
  }
  float delay_ms = 0.0f;
  int32_t beat_ms = 0;
  const std::optional<bool> single = SinglePlayer();
  if (!single || !EchoDelay(&delay_ms, &beat_ms) || !GuestRange(g_blobs.echo_delay_value, 4, true)) {
    LogFallback("Star Power on: the players' table or the guest structs are not the game's memory");
    return false;
  }
  // World Tour's single player settings, or its settings for several players with ghbandhero_star_power_reverb_multi_preset.
  const bool single_preset = *single && !MultiPresetKnob();
  bool already_on;
  {
    std::lock_guard lock(g_mutex);
    already_on = g_verb_on;
    if (!(already_on && single_preset)) {
      g_verb_on = true;
    }
  }
  if (already_on && single_preset) {
    // World Tour's GH_Star_Power_Verb_On: "if ($Star_power_verb_is_on = 1) if not isSinglePlayerGame ... endif
    // return" (Band Hero's goes on to its send instead; World Tour's is kept).
    if (g_ons.fetch_add(1, std::memory_order_relaxed) < kLoggedSends) {
      REXLOG_INFO("[star power reverb] player {} on ({}): World Tour's flag still on in single player (its last "
                  "Star Power off was not seen): nothing changed, as World Tour does",
                  player, PartName(tracks.part));
    }
    r3.u64 = 1;
    return true;
  }
  const Preset& preset = PresetFor(tracks.part, single_preset, already_on);
  const float send_db = preset.send_db + float(SendDbKnob());
  const float volume = SendVolume(send_db);
  if (delay_ms > 0.0f) {
    Store32(g_blobs.echo_delay_value, std::bit_cast<uint32_t>(delay_ms));
  }
  uint32_t sent = 0;
  {
    GuestCall call(*live);
    if (delay_ms > 0.0f) {
      call(SetSoundBussEffects, g_blobs.echo_delay, 0u);
    }
    call(SetSoundBussEffects, g_blobs.echo[preset.echo], 0u);
    call(SetSoundBussEffects, g_blobs.reverb[preset.reverb], 0u);
    if (g_tail_muted.exchange(false) && Load32(kSlot2Return) != 0) {
      call(MuteSendReturn, kSlot, 0u);
    }
    for (uint32_t i = 0; i < tracks.count; ++i) {
      if (TrackLive(tracks.address[i])) {
        call(SetTrackSend, tracks.address[i], kSlot, volume, 0.0f);
        ++sent;
      }
    }
  }
  {
    std::lock_guard lock(g_mutex);
    std::vector<uint32_t>& recorded = g_sent[player - 1];
    for (uint32_t i = 0; i < tracks.count; ++i) {
      if (std::find(recorded.begin(), recorded.end(), tracks.address[i]) == recorded.end()) {
        recorded.push_back(tracks.address[i]);
      }
    }
  }
  if (g_ons.fetch_add(1, std::memory_order_relaxed) < kLoggedSends) {
    const std::string delay =
        delay_ms > 0.0f
            ? fmt::format("echo delay {:.0f} ms (a beat of {} ms, {:.0f} BPM)", delay_ms, beat_ms, 60000.0f / beat_ms)
            : fmt::format("echo delay left as it was (a beat of {} ms)", beat_ms);
    REXLOG_INFO("[star power reverb] player {} on ({}): World Tour's {} preset, {}, send {:+.1f} dB ({:.3f}) on {} of "
                "{} track{}",
                player, PartName(tracks.part), preset.name, delay, send_db, volume, sent, tracks.count,
                tracks.count == 1 ? "" : "s");
  }
  LogPoolPeak();  // the song's load is done by now
  r3.u64 = 1;
  return true;
}

// [[midasm_hook]] 0x82168BD4 (sub_82168AF0, a track's send set: before "addi r10,r11,56", "stb r8,76(r11)" and
// "lfs f1,56(r11)", r11 = the track (r31) + 24 * the slot (r29)). After it stores the slot's new state in its send at
// +56 + 24 * (slot - 1) (+20, +21 and +22 set) and brings the send's volume to now (sub_82160B58), sub_82168AF0
// applies the send at once with sub_823CC250(slot, ..., f1) (0 removes the connection, above 0 sets it or makes it),
// but r11 is rebuilt from the slot itself (rlwinm r11,r29,1 + add + rlwinm r11,r7,3 at 0x82168BB8 to 0x82168BD0): the
// byte cleared after the apply (+20) and f1 are the next slot's. On the console slot 2 is 0 on every track, so slot 1's
// send is applied at once with 0: the connection is removed, and the frame's update sub_82168C10 makes it again with
// the right volume on the tracks it is called for (sub_82166238's nine, the stems' through sub_82161A50). It is not
// called for SAP + 324 and SAP + 436, the guitar and bass tracks of SongSetVenueEffectSendVolume track = guitar or bass
// (sub_82166B08), so the venue send the snapshots give them (GHBANDHERO_Music_Change_DSP_Snapshot: 0 and +2 dB, then -11 and
// -5) is never applied: kept so, the engine's read is left alone while slot 2 is 0. With slot 2 used (only on the
// stems' and the drums' tracks, both updated each frame), slot 1's send would take slot 2's volume (the drums' Star
// Power send, up to 4, into the venue's reverb for a frame, from a snapshot), and a track's destruction (sub_82168DD0:
// slot 1 then slot 2 set to 0 through here, then the connections forgotten) would leave slot 1's on at slot 2's level.
// Slot 2's own send reads +104 (0 from the constructor), and its clear writes a zero byte at +124, past the 112-byte
// track: +12 of the next one (the most significant byte of its own ramp's start, in ms; the SAP's tracks and a stem's
// two are 112 apart). With the option on and the stack there, r11 is the slot being set (r31 + 24 * (slot - 1)), so
// both the clear and the volume are the slot's own (the clear what the frame's update does after each apply,
// 0x82168CDC): for slot 2 always, for slot 1 while slot 2's volume is not 0. r10 (addi r10,r11,56) is not read after.
void GHBandHeroTrackSendFixHook(PPCRegister& r11, PPCRegister& r29, PPCRegister& r31) {
  if (!g_ready.load(std::memory_order_relaxed)) {
    return;
  }
  const uint32_t slot = r29.u32;
  if ((slot != 1 && slot != 2) || r11.u32 != r31.u32 + kSendSize * slot) {
    return;
  }
  if (slot == 1 && (Load32(r31.u32 + kTrackSlot2Volume) & 0x7FFFFFFF) == 0) {
    return;  // the console's read: slot 2's volume, 0
  }
  r11.u64 = r31.u32 + kSendSize * (slot - 1);
}

// [[midasm_hook]] 0x82165F80 (entry of sub_82165F80; r3, the SAP, is not needed: the tracks are those recorded): the
// song player's clean-up, which destroys its players' stem objects (sub_82161C28: their tracks through sub_82168DD0)
// and sets the drums' venue send to 0. It runs before a song is loaded (sub_82167538 at 0x82167578), at the song
// player's init (sub_82167B20, after sub_82165EE0 destroyed the SAP's own tracks, the drums' among them) and at its
// deinit (sub_82167178, after sub_82160D50 and sub_82165EE0 destroyed the stems' and the SAP's tracks); these are its
// only callers, so nothing at a song's end reaches it. Slot 2's send is cut at once on every track a Star Power sent
// that is still there (Star Power may still be on: the scripts that end a song can kill the off): at a song's load,
// before the stems' tracks are destroyed, while the drums' tracks live on from the init to the deinit. A track already
// destroyed lost its connections with its destruction (sub_82168DD0 through sub_82168AF0, fixed at 0x82168BD4). World
// Tour's flag is cleared (its start_gem_scroller clears it at each song), and slot 2's return is muted so that what is
// left of the echo and the reverb does not ring into the next song: World Tour destroyed its stack at end_song. The
// next Star Power unmutes it, as a new song's stack started in World Tour. The effect pool's peak is checked
// (LogPoolPeak).
void GHBandHeroStarPowerCleanupHook(PPCRegister& r1) {
  if (!g_ready.load(std::memory_order_acquire)) {
    return;
  }
  std::vector<uint32_t> sent;
  {
    std::lock_guard lock(g_mutex);
    g_verb_on = false;
    for (std::vector<uint32_t>& tracks : g_sent) {
      for (uint32_t track : tracks) {
        if (std::find(sent.begin(), sent.end(), track) == sent.end()) {
          sent.push_back(track);
        }
      }
      tracks.clear();
    }
  }
  LogPoolPeak();
  PPCContext* live = LiveContext(r1);
  if (!live) {
    LogFallback("the song's clean-up: the hook did not run in its thread's own context (the tracks' destruction "
                "still removes their sends)");
    return;
  }
  GuestCall call(*live);
  const uint32_t cut = CutSends(call, sent);
  if (!g_tail_muted.exchange(true) && Load32(kSlot2Return) != 0) {
    call(MuteSendReturn, kSlot, 1u);
  }
  if (!sent.empty() && g_cleanups.fetch_add(1, std::memory_order_relaxed) < kLoggedCleanups) {
    REXLOG_INFO("[star power reverb] the song player's clean-up: slot 2's send cut on {} of the {} track{} still "
                "sent{}, slot 2's return muted until the next Star Power",
                cut, sent.size(), sent.size() == 1 ? "" : "s",
                cut < sent.size() ? " (the others destroyed, their connections with them)" : "");
  }
}

// [[midasm_hook]] 0x823CBFD0 (entry of sub_823CBFD0, r3 = the slot, r4 = mute: the script functions MuteEffectSend
// and UnMuteEffectSend, sub_823B6720 and sub_823B6788, its only callers). The game mutes the venue's slot at the pause
// (PauseGh3Sounds) and unmutes it after (UnpauseGh3Sounds, freeplay_unpause_spawned): slot 2 follows, as World Tour's
// stack did, except an unmute while the song's clean-up holds it muted. Its own call for slot 2 comes back here and is
// left alone.
void GHBandHeroStarPowerMuteHook(PPCRegister& r1, PPCRegister& r3, PPCRegister& r4) {
  if (r3.u32 != kVenueSlot || !g_ready.load(std::memory_order_acquire)) {
    return;
  }
  const bool mute = (r4.u32 & 0xFF) != 0;
  if ((!mute && g_tail_muted.load(std::memory_order_relaxed)) || Load32(kSlot2Return) == 0) {
    return;
  }
  PPCContext* live = LiveContext(r1);
  if (!live) {
    LogFallback("the pause: the hook did not run in its thread's own context (slot 2 not muted with slot 1)");
    return;
  }
  GuestCall call(*live);
  call(MuteSendReturn, kSlot, mute ? 1u : 0u);
}
