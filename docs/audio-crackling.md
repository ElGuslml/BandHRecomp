# Crackling in songs on slower PCs

**Fixed (2026-11-10), in the SDK (change 82).** Reported in Guitar Hero: Warriors of Rock, which spaces its audio the
same way as Band Hero; the fix is the same here.

**Symptom:** on a slower CPU (a player's Xeon E5-2640 v3), a crackle through the whole of every song, on either output,
never in the menus. The log has an `audio_keep_time: 1 frames (5 ms) ... dropped` line every few hundred ms of a song.

**Cause:** the game's sound is made a frame (5.33 ms) at a time by its audio callback. FMOD mixes on a thread of its own
and polls how far its ring has been read, so the calls are spaced at least 3 ms apart
(`audio_callback_min_interval_us = 3000`; without it FMOD missed blocks and the music played as noise). The SDK counted
the 3 ms from the end of the last call, so a frame cost the callback's work plus 3 ms: past about 2.3 ms of work the
sound fell behind and `audio_keep_time` dropped a frame to catch up, a click each time. On the player's PC 1000 instead
of 3000 stopped it.

**Fix:** the SDK counts the 3 ms from the start of the last call: a frame costs the longer of the two, so the callback
can take up to 5.3 ms, and two calls still start at least 3 ms apart for FMOD. Measured in Warriors of Rock with the
callback made slower: 2.5 ms more, 955 frames dropped in 40 s of a song before and none after; 4 ms more, none after on
either output. On a fast PC nothing changes (the callback takes well under a millisecond there).

**Turning it off:** `audio_callback_min_interval_us = 0` in `ghbandhero.toml` turns the spacing off (FMOD's noise may come back);
`audio_keep_time = false` turns the catch-up off (the sound then stays late after a hold-up, until a pause).
