# Documentation

Short notes on each problem found while making the game run, and what was done about it. Each one gives
the symptom, the cause, the fix and how to turn the fix off, so that you can check it on your own machine.

Band Hero is Guitar Hero 5's code base one step on, so it has GH5's problems. GH5Recomp's fixes (most of them GH
Metallica's and GH Van Halen's before) were ported with every address translated; the investigations are in those
projects.

| Note | Status |
|---|---|
| [The fixes ported from Guitar Hero 5](fixes-from-gh5.md) | Working: the threads' waits, the song clock, the audio, the guitar, the keyboard, the controllers, frame rates above 60 (the camera cuts, the drummer, the frames' real length), ultrawide, the screen effects' switches, quitting from the main menu; songs played with an Xbox 360 guitar, the languages and Low latency audio checked; drums and singing not tried yet |
| [NativeXRenderer in Band Hero](native-x-renderer-ghbandhero.md) | Working, the default renderer: GH5's map and profile, translated; SMAA and FXAA on the scene's copy; shadow quality with GH5's filter fix; the observer at 100 % |
| [Clothes shooting out for a frame](clothes-stretching.md) | GH5's fix: the skinning waits for the animation that writes its bones |
| [Downloadable content](dlc.md) | Not tested: GH5's page, unchanged |
| [Stutters the first time something is shown](compiling-shaders.md) | Fixed: a pipeline list ships with the game; the launcher compiles it for your graphics card at the first Play ("Compiling shaders"; NativeXRenderer) |

Building the project, its configuration and how the SDK is changed: [`development.md`](development.md).
