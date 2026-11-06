// ghbandhero - the profile follows the controller that starts the game (src/players.cpp).

#pragma once

namespace ghbandhero {

// Each frame (src/quit_prompt.cpp's hook), whether Party Play is shown and a dialog box is over the screen: the first
// time Party Play shows, the first controller to press a button becomes the profile's.
void UpdatePressWindow(bool party_play, bool dialog);

}  // namespace ghbandhero
