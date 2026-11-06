// ghbandhero - quitting from the main menu.
//
// The game has no way out of itself (on the console the dashboard was). In the main menu, B now asks whether to quit
// the game: the SDK's quit prompt (rex/ui/overlay/quit_prompt.h, on the recomp branch), a dialog in the game's
// language where A quits (the window closes, as with its close button) and B cancels. The game does not see those
// presses. GH Van Halen also offered it on its title screen ("PRESS ANY BUTTON TO ROCK"); Band Hero starts in Party Play
// instead (songs playing while the players join, "PLAY NOW" / "START MAIN MENU"), where B is a joined player's red
// fret: not offered there yet.
//
// The game's screens are made of screen elements that its scripts create and destroy (CreateScreenElement,
// sub_823E47E8; DestroyScreenElement, sub_823E43C8), each with an id, the checksum of its name. One of them is on the
// main menu only (found in the scripts, DATA/COMPRESSED/PAK/qb.pab.xen): 0xA7988D2C, the container the main menu's
// script creates (destroyed by ui_destroy_mainmenu), the same as GH Van Halen's. The engine finds an element by id in
// a hash table (sub_82217940, the table object at [0x82DABF94]); the hook reads the same table once per frame on the
// main thread, which runs the scripts, and offers the prompt while that element exists and no dialog box does
// (0xCC5FECBC, dialog_box_container: a message box over the menu takes B itself; GH Van Halen's popupelement).
// quit_prompt = false (an SDK cvar) disables it. The same frame tells src/players.cpp when Party Play shows: 0x22F50BBE,
// a container its scripts only fade when players join, and ui_deinit_freeplay destroys (found by comparing every
// element's id at each screen).

#include "players.h"

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>
#include <rex/ui/overlay/quit_prompt.h>

#include <cstdint>
#include <cstring>

namespace {

constexpr uint32_t kElementTable = 0x82DABF94;  // the screen elements' table object (a pointer)
constexpr uint32_t kMainMenuContainer = 0xA7988D2C;
constexpr uint32_t kPartyPlay = 0x22F50BBE;
constexpr uint32_t kDialogBox = 0xCC5FECBC;  // QBKey("dialog_box_container")

const uint8_t* GuestBase() {
  auto* kernel = rex::system::kernel_state();
  return kernel && kernel->memory() ? kernel->memory()->virtual_membase() : nullptr;
}

uint32_t Load32(const uint8_t* base, uint32_t address) {
  uint32_t value;
  std::memcpy(&value, base + address, 4);
  return __builtin_bswap32(value);
}

// sub_82217940: the element with this id, or 0. The table object holds two hash tables (+12, +16; GH Van Halen's +8,
// +12), each {uint32 bits, entry* buckets, ..., uint8 keep_empty at +21}; an entry is {uint32 key, uint32 value,
// entry* next} and the chain starts at buckets[id & ((1 << bits) - 1)].
uint32_t FindElement(const uint8_t* base, uint32_t id) {
  const uint32_t table = Load32(base, kElementTable);
  if (!table) {
    return 0;
  }
  for (uint32_t offset : {12u, 16u}) {
    const uint32_t hash = Load32(base, table + offset);
    if (!hash) {
      continue;
    }
    const uint32_t bits = Load32(base, hash);
    const uint32_t buckets = Load32(base, hash + 4);
    if (!buckets || bits > 24) {
      continue;
    }
    uint32_t entry = buckets + (id & ((1u << bits) - 1)) * 12;
    for (int length = 0; entry && length < 4096; ++length) {
      if (Load32(base, entry) == id) {
        if (const uint32_t value = Load32(base, entry + 4)) {
          return value;
        }
        break;
      }
      entry = Load32(base, entry + 8);
    }
  }
  return 0;
}

}  // namespace

// [[midasm_hook]] 0x82414118 (sub_82414058, the main thread's frame, before it waits for the previous one): once per
// frame.
void GHBandHeroQuitPromptHook() {
  const uint8_t* base = GuestBase();
  if (!base) {
    return;
  }
  const bool main_menu = FindElement(base, kMainMenuContainer) != 0;
  const bool party_play = FindElement(base, kPartyPlay) != 0;
  const bool dialog = FindElement(base, kDialogBox) != 0;
  static int last_state = -1;
  const int state = (main_menu ? 1 : 0) | (dialog ? 2 : 0) | (party_play ? 4 : 0);
  if (state != last_state) {
    REXLOG_DEBUG("[quit_prompt] main menu {} dialog {} party play {}", main_menu, dialog, party_play);
    last_state = state;
  }
  ghbandhero::UpdatePressWindow(party_play, dialog);
  if (main_menu && !dialog) {
    rex::ui::OfferQuitPrompt();
  }
}
