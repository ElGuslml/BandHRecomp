// ghbandhero - the song's clocks from the vertical blanks' own times, and each song's clock report (src/song_clock.cpp).
#pragma once

namespace ghbandhero {

// A song's music was started / stopped (src/song_sync.cpp's hooks): the bounds of the clock's report in the log.
void SongClockSongStarted();
void SongClockSongEnded();

}  // namespace ghbandhero
