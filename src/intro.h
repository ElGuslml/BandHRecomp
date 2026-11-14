// ghbandhero - the boot's intro video ended as soon as the game has loaded (src/intro.cpp).

#pragma once

#include <cstdint>

namespace ghbandhero {

// Whether ghbandhero_skip_intro is on and the boot's intro video has not been ended yet: src/debug_menu.cpp's hooks on
// the script interpreter run while it is.
bool IntroSkipPending();

// Whether the script global `global`, read by the running script `script_name` (src/debug_menu.cpp's "$name" resolve
// hook), reads as the integer 1: end_movie_early in destroy_loading_screen_spawned, while the intro is skipped.
bool IntroReadsAsOne(uint32_t script_name, uint32_t global);

}  // namespace ghbandhero
