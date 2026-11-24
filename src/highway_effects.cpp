// ghbandhero - the note highway's Star Power, band moment and revival effects at the highway's scale (ghbandhero_highway_scale,
// src/mods.cpp). Ported from GH Warriors of Rock (its src/highway_effects.cpp, docs/mods.md, local_docs/54, where it
// was checked in the game: its glows on the rails at 85 %); untested here.
//
// The highway scale with one player is the first row of highway_scale_chart, the engine's own path for two to four
// players: update_highway_scale (compiled, scripts\qtocpp_scriptlist.qb) scales the highway, its rails and the gems
// through the rows of the position table, sets the highway sprite's x scale, and moves and turns the two rail
// containers (sidebar_container_left/rightp<n>) without scaling them (SetScreenElementProps here, GH Warriors of Rock's
// fastsetscreenelementprops); their angle is atan2(the unscaled height, the scaled half difference of the widths).
// Create_Highway_Star_Power_Effect (scripts\guitar\guitar_starpower.qb, from star_power_activate),
// Create_Highway_Band_Moment_Effect (band_moments.qb) and Create_Revival_Highway_glow (guitar_revival.qb) then make plain
// sprites (Mat_sidebar_GLOW_02, their stars and particles as children): glows in the rail containers at (0, -1.4 x the
// unscaled highway_height) with fixed scales ((+-1.5, 4) for Star Power), and a rush up the middle in
// fretbar_containerp<n> (its x already scaled) at (0, 0.9 x the height), and animate them with se_setprops (scales,
// positions relative to where they are, {(+-140, -80) relative} and rot_angle -+50 with one player, the fade's turn).
// So at any scale but the console's they are drawn at the console's size and lean as the containers do; the console's
// two to four player highways (0.74 to 0.97) look the same. This game has none of GH Warriors of Rock's other effects
// (band_moment_create_nowbar_glow_fx's ..._indy names at -0.8 h and 1.4 h, Create_Revival_Highway_glow_Ankh01, the
// Quest characters' Create_Pandora_RP_FX_LVL1 / LVL2).
//
// With ghbandhero_highway_scale away from 100 and one highway on screen (the one-player row in place: the Highway_2Dp<n>
// sprites setup_highway creates and destroy_highway destroys are counted), the effects' own elements get it: in
// CreateScreenElement (sub_823E47E8, the C function table's entry at 0x82B0B360; r3 = the params struct) and the
// screen elements' se_setprops (sub_823EA7D0's branch at 0x823EA8AC, before this->vtable[+8](this, params): the
// element's id at +44, the params in r30), the rail ones' scale and positions times s and their turn by the containers'
// angle error, the middle ones' height, position and scale in y times s. The positions computed from the height are
// recognised by their value (the struct may share the script's local), so none is scaled twice. DestroyScreenElement is
// sub_823E43C8 (table entry 0x82B0B36C). The three functions are GH Warriors of Rock's sub_823C6BF0, sub_823D4B38 and
// sub_823C6708, the same code but for data addresses; the se_setprops branch differs: GH Warriors of Rock's keeps the
// params in r29 and calls vtable[+4].

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <string>
#include <string_view>
#include <unordered_map>

#include "mods.h"

namespace {

constexpr uint32_t QbKey(std::string_view name) {
  uint32_t crc = 0xFFFFFFFF;
  for (char c : name) {
    crc ^= uint8_t(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
    for (int bit = 0; bit < 8; ++bit) crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
  }
  return crc;
}

constexpr uint32_t kId = QbKey("id");
constexpr uint32_t kScale = QbKey("scale");
constexpr uint32_t kPos = QbKey("pos");
constexpr uint32_t kRotAngle = QbKey("rot_angle");
static_assert(kId == 0x40C698AF && kScale == 0x13B9DA7B && kPos == 0x7F261953 && kRotAngle == 0xED344E1B);
constexpr uint8_t kTypeInteger = 1;
constexpr uint8_t kTypeFloat = 2;
constexpr uint8_t kTypeVector2 = 5;
constexpr uint8_t kTypeStruct = 10;
constexpr uint8_t kTypeName = 13;
constexpr uint32_t kElementId = 44;  // a screen element's id (CreateScreenElement and sub_823EA7D0 read +44)

// The effect's elements: on a rail (in its container, -1 left, 1 right) or up the middle (in the fretbar container).
enum class Kind { kRail, kMiddle, kHighway };
struct Target {
  Kind kind;
  int side = 0;
  int player = 0;
};

// The scripts' FormatText names ('...p%p'), each player's.
const std::unordered_map<uint32_t, Target>& Targets() {
  static const std::unordered_map<uint32_t, Target> targets = [] {
    std::unordered_map<uint32_t, Target> t;
    for (int p = 1; p <= 4; ++p) {
      const std::string n = std::to_string(p);
      auto add = [&](std::initializer_list<const char*> names, Kind kind, int side) {
        for (std::string name : names) {
          const size_t at = name.find('#');
          name.replace(at, 1, n);
          t[QbKey(name)] = {kind, side, p};
        }
      };
      add({"sidebar_left_spglowp#", "sidebar_left2_spglowp#", "sidebar_left_bmglowp#", "sidebar_left_rvglowp#"},
          Kind::kRail, -1);
      add({"sidebar_right_spglowp#", "sidebar_right2_spglowp#", "sidebar_right_bmglowp#", "sidebar_right_rvglowp#"},
          Kind::kRail, 1);
      add({"sidebar_center_sprushp#", "sidebar_center_bmrushp#", "sidebar_center_bmglowp#", "sidebar_center_rvrushp#"},
          Kind::kMiddle, 0);
      add({"highway_2dp#"}, Kind::kHighway, 0);
    }
    return t;
  }();
  return targets;
}

// Each player's highway sprite on screen (Highway_2Dp<n> created and not destroyed): with one, the one-player row of
// highway_scale_chart is the highways' scale (get_highway_pos_and_scale's num_non_vocals_players; the vocals have no
// such sprite).
std::atomic<bool> g_highway[4] = {false, false, false, false};

uint8_t* Base() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

// Guest addresses from the game's own structs (its heaps, virtual and physical, and the image): not null, aligned.
bool Plausible(uint32_t address) { return address >= 0x10000 && address < 0xFFFF0000 && !(address & 3); }

uint32_t Load32(uint8_t* base, uint32_t address) {
  uint32_t v;
  std::memcpy(&v, base + address, 4);
  return __builtin_bswap32(v);
}
void Store32(uint8_t* base, uint32_t address, uint32_t value) {
  value = __builtin_bswap32(value);
  std::memcpy(base + address, &value, 4);
}
float LoadF(uint8_t* base, uint32_t address) {
  uint32_t v = Load32(base, address);
  float f;
  std::memcpy(&f, &v, 4);
  return f;
}
void StoreF(uint8_t* base, uint32_t address, float f) {
  uint32_t v;
  std::memcpy(&v, &f, 4);
  Store32(base, address, v);
}

// A struct's component of that name (type at +2, name at +4, value at +8, next at +12; the first at the struct's +4),
// or 0.
uint32_t Find(uint8_t* base, uint32_t s, uint32_t name) {
  if (!Plausible(s)) return 0;
  uint32_t c = Load32(base, s + 4);
  for (int guard = 0; Plausible(c) && guard < 64; ++guard) {
    if (Load32(base, c + 4) == name) return c;
    c = Load32(base, c + 12);
  }
  return 0;
}

// A vector's two floats (the component's value points at a header, then x and y), or 0.
uint32_t Vector2(uint8_t* base, uint32_t component) {
  if (!component || base[component + 2] != kTypeVector2) return 0;
  const uint32_t v = Load32(base, component + 8);
  return Plausible(v) ? v + 4 : 0;
}

struct Correction {
  float scale;  // s
  float turn;   // degrees the rail containers lack (the left one's)
  float height; // highway_height
};

// The correction for a player's effect: the one-player row in place (that player's the only highway) with a scale of
// the mods'.
bool CorrectionFor(int player, Correction& out) {
  const ghbandhero::mods::Highway& h = ghbandhero::mods::OnePlayerHighway();
  if (std::abs(h.scale - 1.0) < 1e-6 || player < 1 || player > 4) return false;
  for (int p = 1; p <= 4; ++p) {
    if (g_highway[p - 1].load(std::memory_order_relaxed) != (p == player)) return false;
  }
  // update_highway_scale's angle: atan(s (bottom - top) / 2, height); the rails' own: atan((bottom - top) / 2, height).
  const double half = (h.bottom_width - h.top_width) / 2.0;
  constexpr double kDegrees = 57.29577951308232;
  out.scale = float(h.scale);
  out.turn = float((std::atan(half / h.height) - std::atan(h.scale * half / h.height)) * kDegrees);
  out.height = float(h.height);
  return true;
}

void Note(uint32_t id, const Correction& c) {
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true)) {
    REXLOG_INFO("[mods] the highway's effects (first {:08X}) at the highway's scale {:.0f} %, the rails' turned {:.2f} "
                "degrees",
                id, c.scale * 100.0f, c.turn);
  }
}

// The props of one of the effect's elements: its scale, its position, its turn.
void Correct(uint8_t* base, uint32_t props, const Target& target, const Correction& c, uint32_t id) {
  const float s = c.scale;
  if (uint32_t v = Vector2(base, Find(base, props, kScale))) {
    if (target.kind == Kind::kRail) StoreF(base, v, LoadF(base, v) * s);
    StoreF(base, v + 4, LoadF(base, v + 4) * s);
  }
  if (uint32_t pos = Find(base, props, kPos)) {
    if (base[pos + 2] == kTypeVector2) {
      // (0, -1.4 h) on a rail, (0, 0.9 h) up the middle: by value, so a shared one is not scaled twice.
      const uint32_t v = Vector2(base, pos);
      const float factor = target.kind == Kind::kRail ? -1.4f : 0.9f;
      if (v && std::abs(LoadF(base, v)) < 0.01f && std::abs(LoadF(base, v + 4) - factor * c.height) < 0.5f) {
        StoreF(base, v + 4, factor * c.height * s);
      }
    } else if (base[pos + 2] == kTypeStruct && target.kind == Kind::kRail) {
      // {(x, y) relative}: the fade's move, a struct of its own each time.
      uint32_t inner = Load32(base, pos + 8);
      if (Plausible(inner)) {
        for (uint32_t e = Load32(base, inner + 4), guard = 0; Plausible(e) && guard < 8; e = Load32(base, e + 12), ++guard) {
          if (uint32_t v = Vector2(base, e)) {
            StoreF(base, v, LoadF(base, v) * s);
            StoreF(base, v + 4, LoadF(base, v + 4) * s);
            break;
          }
        }
      }
    }
  }
  if (target.kind == Kind::kRail && std::abs(c.turn) > 0.01f) {
    if (uint32_t r = Find(base, props, kRotAngle)) {
      float angle = base[r + 2] == kTypeFloat     ? LoadF(base, r + 8)
                    : base[r + 2] == kTypeInteger ? float(int32_t(Load32(base, r + 8)))
                                                  : NAN;
      if (!std::isnan(angle)) {
        // The left container turns by +angle, the right one by -angle.
        base[r + 2] = kTypeFloat;
        StoreF(base, r + 8, angle - float(target.side) * c.turn);
      }
    }
  }
  Note(id, c);
}

}  // namespace

// [[midasm_hook]] 0x823E47E8 (the entry of sub_823E47E8, CreateScreenElement: r3 = its params).
void GHBandHeroHighwayEffectCreateHook(PPCRegister& r3) {
  if (std::abs(ghbandhero::mods::OnePlayerHighway().scale - 1.0) < 1e-6) return;
  uint8_t* base = Base();
  if (!base || !Plausible(r3.u32)) return;
  const uint32_t id_component = Find(base, r3.u32, kId);
  if (!id_component || base[id_component + 2] != kTypeName) return;
  const uint32_t id = Load32(base, id_component + 8);
  auto it = Targets().find(id);
  if (it == Targets().end()) return;
  if (it->second.kind == Kind::kHighway) {
    g_highway[it->second.player - 1].store(true, std::memory_order_relaxed);
    return;
  }
  Correction c;
  if (CorrectionFor(it->second.player, c)) Correct(base, r3.u32, it->second, c, id);
}

// [[midasm_hook]] 0x823E43C8 (the entry of sub_823E43C8, DestroyScreenElement: r3 = its params): a highway sprite gone.
void GHBandHeroHighwayEffectDestroyHook(PPCRegister& r3) {
  if (std::abs(ghbandhero::mods::OnePlayerHighway().scale - 1.0) < 1e-6) return;
  uint8_t* base = Base();
  if (!base || !Plausible(r3.u32)) return;
  const uint32_t id_component = Find(base, r3.u32, kId);
  if (!id_component || base[id_component + 2] != kTypeName) return;
  auto it = Targets().find(Load32(base, id_component + 8));
  if (it != Targets().end() && it->second.kind == Kind::kHighway) {
    g_highway[it->second.player - 1].store(false, std::memory_order_relaxed);
  }
}

// [[midasm_hook]] 0x823EA8AC (sub_823EA7D0, a screen element's member functions: se_setprops (0x9F23DC21), before its
// SetProperties call; a branch target, only from that key's compare; r31 = the element, r30 = the params).
void GHBandHeroHighwayEffectPropsHook(PPCRegister& r31, PPCRegister& r30) {
  if (std::abs(ghbandhero::mods::OnePlayerHighway().scale - 1.0) < 1e-6) return;
  uint8_t* base = Base();
  if (!base || !Plausible(r31.u32) || !Plausible(r30.u32)) return;
  const uint32_t id = Load32(base, r31.u32 + kElementId);
  auto it = Targets().find(id);
  if (it == Targets().end() || it->second.kind == Kind::kHighway) return;
  Correction c;
  if (CorrectionFor(it->second.player, c)) Correct(base, r30.u32, it->second, c, id);
}
