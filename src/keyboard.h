// ghbandhero - the keyboard's strum on each fret, only in a song (src/keyboard.cpp).

#pragma once

namespace ghbandhero {

// Before the guest starts: the SDK's keyboard strums (mnk_tap_target) only while a song runs.
void InitializeKeyboard();
// Whether a song is playing now: a player's update ran within the last quarter of a second (the gate above). For
// NativeXRenderer's summary of the song's frames (native_x_renderer::SetMainActivity, src/framerate.cpp).
bool PlayingSong();

}  // namespace ghbandhero
