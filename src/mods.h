// ghbandhero - the launcher's Mods page (src/mods.cpp).

#pragma once

namespace ghbandhero::mods {

// The mods' changes of the game's script data, from their cvars (src/script_globals.h); once, before the script
// interpreter's hooks first run (src/debug_menu.cpp).
void Configure();

// The one-player highway as the game draws it with the mods (after Configure): its scale (ghbandhero_highway_scale, 1 = the
// console's), its height and top width (highway_guitar1's, those of the length and perspective), its bottom width
// (512). src/highway_effects.cpp places the Star Power, band moment and revival effects by them.
struct Highway {
  double scale = 1.0;
  double height = 350.0;
  double top_width = 160.0;
  double bottom_width = 512.0;
};
const Highway& OnePlayerHighway();

}  // namespace ghbandhero::mods
