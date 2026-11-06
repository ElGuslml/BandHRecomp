# Documentation

Short notes on each problem found while making the game run, and what was done about it. Each one gives
the symptom, the cause, the fix and how to turn the fix off, so that you can check it on your own machine.

Band Hero is Guitar Hero 5's code base one step on, so it has GH5's problems. GH5Recomp's fixes (most of them GH
Metallica's and GH Van Halen's before) were ported with every address translated; the investigations are in those
projects.

| Note | Status |
|---|---|
| [The fixes ported from Guitar Hero 5](fixes-from-gh5.md) | In testing: the threads' waits, the song clock, the audio, the guitar, the keyboard, the controllers, frame rates above 60, ultrawide, the screen effects' switches, quitting from the main menu; checked by the tools, not yet by a player |
| [NativeXRenderer in Band Hero](native-x-renderer-ghbandhero.md) | Working: GH5's map and profile, translated, with FXAA and SMAA as GH Warriors of Rock; the observer at 100 % |
| [Downloadable content](dlc.md) | Not tested: GH5's page, unchanged |
