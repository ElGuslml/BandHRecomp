# Clothes shooting out for a frame

**Symptom (in Guitar Hero 5).** Above 60 FPS a musician's clothes, most of all a long coat, shot out across the screen
for a single frame, in long wedges ending in a point, several times a song, on both renderers. Band Hero has the same
code, instruction for instruction; nobody has seen it here yet, because nobody has played it.

**Cause.** An order in the game. The engine skins its characters on the CPU: the three job workers compute each piece's
vertices from a palette of 128 matrices, and the draw reads the result. Updating a skeleton starts the animation jobs that
write that palette, in stages, then queues the skinning of its pieces (`sub_82476758`, through `sub_8254B268`), and only
then waits for those jobs. So the skinning could read a palette half written, with the bones of the clothes all still on
one matrix, and what hangs from them went with it. In GH5 a probe found the palette changing under the skinning 19 to 48
times a song, once at the very second a stretch was noted down. It also happens at 60 FPS, fewer times. GH Warriors of
Rock's own code waits first.

**Fix.** `src/skinning.cpp` (GH5's): a hook at the start of `sub_82476758` does that wait first, so the skinning starts
on a finished palette. In GH5 the race went down to 1 to 3 times a song and four songs at 160 FPS showed no stretch; the
one frame where the whole band moves took about 30 ms there and about 44 with it, in Party Play.

**Turn it off.** `ghbandhero_skinning_order_fix = false` in `ghbandhero.toml`.
