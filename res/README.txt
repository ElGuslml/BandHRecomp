BandHRecomp {VERSION} - Band Hero (Xbox 360), for Windows
========================================================================

A static recompilation of the Xbox 360 game to native Windows. The game's code is already translated and
compiled into BandHeroRecomp.exe: you do not need to compile anything. You do need your own copy of the game,
because this package contains no game files.

This is a beta. Not affiliated with Microsoft, Activision or the game's developers.


REQUIREMENTS

- Windows 10 or 11, 64-bit.
- A GPU with up-to-date drivers that support Vulkan (NativeXRenderer, the default renderer) or
  Direct3D 12 (the Xenos renderer).
- A CPU with SSE4.2 (practically any 64-bit CPU from 2009 on).
- Microsoft Visual C++ Redistributable 2015-2022 (x64). Most PCs already have it. If Windows says that
  MSVCP140.dll or VCRUNTIME140.dll is missing, install it from:
  https://aka.ms/vs/17/release/vc_redist.x64.exe
- Your Band Hero disc as an ISO file (tested with the disc sold in the USA; about 5.1 GB once installed).
- An Xbox 360 guitar. Any other controller works as a guitar too, and so does the keyboard. An Xbox One
  guitar (such as the PDP Riffmaster for Xbox) needs Microsoft GameInput: the installer offers it, and so does
  the launcher's Controls page while it is missing (Windows asks for permission to install it).
- To sing (experimental): any microphone connected to the PC, and a controller for the singer.


INSTALLATION

1. Either run the installer (BandHRecomp-{VERSION}-setup.exe: it installs for your Windows user, without
   administrator rights), or extract this zip to any folder, for example C:\Games\BandHRecomp.

2. Run "Band Hero Launcher.exe". The first time, it asks for your Xbox 360 ISO, checks that it is the right
   game and extracts its files next to the launcher, then asks you to create your profile (your saves,
   achievements and playtime, shared with the other ElGus Recomps launchers). You only do this once.

   You can also copy the game files yourself instead: put the contents of the disc into the "assets"
   folder, so that assets\default.xex exists (extract-xiso, https://github.com/XboxDev/extract-xiso/releases,
   gets them out of an ISO).

3. Choose your options and press PLAY. Next time you can start BandHeroRecomp.exe directly: it uses the
   options you saved.

4. Calibrate: in the game's options, run the audio and video calibration once, so that the notes match what
   you hear on your own speakers or headphones.

If your default.xex is not the version this build was made from (CRC32 57900866), the game warns you: a
different version will most likely crash.


"WINDOWS PROTECTED YOUR PC" / UNKNOWN PUBLISHER

The executables are not code-signed, so Windows SmartScreen warns about an unknown publisher the first time
you run them. To avoid it with the zip, unblock it before you extract it: right-click it, Properties, tick
"Unblock" at the bottom of the General tab, OK, and then extract it. Otherwise click "More info" and then
"Run anyway" in the warning (once for each program).


BAND HERO LAUNCHER

The launcher works with the mouse, the keyboard or a controller. Changes are saved automatically and apply
when you press PLAY.

- Play: starts the game, with your current settings, achievements and updates at a glance. The first time,
  and after an update of the game or of your graphics driver, it first shows "Compiling shaders" for a
  moment: it prepares the game's graphics for your graphics card, so that the game does not stutter the
  first time it shows something (with NativeXRenderer). Skip starts the game at once; it then compiles
  them as you play.
- Your profile (your picture at the top right): hours played, streaks, the week's activity, your games
  and your favorite achievements; edit your name, picture and color, or switch profiles.
- Achievements: the game's 49 achievements, unlocked or not. In the game, F7 shows the same list.
- Controls: your guitars, controllers, the keyboard and the microphone.
  - A guitar or a controller: what each of its buttons does. A guitar's frets are A, B, Y, X and LB of an
    Xbox controller, strum is the D-pad, the whammy bar the right stick (a controller's, pushed to either
    side) and Star Power Back. A controller's Instrument: Guitar, Drums, or Vocals for the singer (see
    SINGING). With Drums, Drum kit chooses 5 lanes (Guitar Hero) or 4 lanes (the game's Rock Band highway, as
    with a Rock Band drum kit on the console); a real drum kit has the same choice.
    Play style, each controller its own: "Controller · GH III" (the default) plays songs as in Guitar Hero III, with the
    frets on LT, LB, RB, RT and A and the open note (bass) on B, no strum (pressing a fret plays its note),
    Star Power on the D-pad or Back, and the whammy bar on either stick; the game plays it with Guitar Hero
    III's own controller rules. Its buttons in a song are its own ("In a song": each fret, the open note,
    Star Power and the whammy bar on up to two buttons; a whammy bar on a button is all or nothing), apart
    from the menus' ("Navigation"). "Guitar · Xbox 360" plays it as an Xbox 360 guitar (for a guitar Windows
    sees as a controller, through an adapter), and "Strum when a fret is
    pressed" makes its frets strum as you press them, in songs. Each controller keeps its own assignments. Whammy bar: how far the game reads the bar.
    If it reads pressed with the bar at rest, or does nothing (some third-party guitars), press Assign,
    push the bar all the way and hold it, then let it go; X clears the assignment. Tilt: the same, if tilting
    gives you no Star Power (a Wii or PS3 guitar): press Assign, tilt the neck up as for Star Power and hold it,
    then go back to your playing position.
  - Keyboard (experimental): turn on "Play with keyboard and mouse" to play with the keyboard as a guitar
    (frets D F J K L, Star Power with Space, Start with Enter), and change any key. Play style:
    "Controller · GH III" (the default) plays songs as in Guitar Hero III: no strum (pressing a fret key plays
    its note), the open note (bass) on 3, the whammy bar on the Right arrow; the arrows do nothing in a song.
    "Guitar · Xbox 360" strums with the Up and Down arrows, and "Strum when a fret is pressed" makes each
    fret strum as you press it, in songs. Shift, Ctrl
    and Alt can be keys of their own (press and release one alone): each side is a separate key, so Left
    Alt can be Star Power. F1 to F12 can be assigned too: a key on a control only presses that control.
    Instrument: Drums plays the keyboard as a drum kit with keys of its own (5 lanes: D red, F yellow,
    J blue, K orange, L green; 4 lanes: K green; Space the kick pedal, Backspace Star Power); your guitar
    keys stay as they were.
  - Microphone (experimental): the microphone you sing with, its channel and gain, and its level (see
    SINGING).
- DLC: choose the folder where you keep your Xbox 360 packages (LIVE, PIRS or CON files, in it or its
  subfolders). The game reads them from there: nothing is copied, and the other ElGus Recomps launchers can
  use the same folder. The packages are listed by the game they were made for, each with a switch; those
  the game cannot read come turned off (with one on, the game would say a package is damaged and turn off
  all your DLC). DLC installed by an earlier version keeps working; if your folder has the same packages,
  "Remove copies" deletes the installed copies to free space (it asks first, and never touches your folder).
- Mods (changes to the game itself; each applies when the game starts):
  - Highway: your own pictures on the note highway (NativeXRenderer only). Folder: where they are (PNG, JPEG,
    BMP or TGA, any size, stretched over the highway; by default Highways in the game's user data folder);
    A opens it in Explorer or changes it.
    Image: A opens the gallery of that folder: Original (the game's own), Random (a different picture at
    each song) and your pictures; X marks a favorite, Y gives a bright picture a darkening of its own, LB
    and RB show All, Favorites (Random among them) or Recent, A uses the picture. Random among: All or
    Favorites. Dim bright pictures (Off by default): darkens every picture toward black so that the notes
    stand out (Automatic: as much as each one is bright). The preview at the right draws the highway with
    the game's own textures, read from your game files: an approximation of the game's look.
    Highway scale (100 % by default): the note highway smaller or larger when you play alone, 80 to 120 %, from the strike line, with its gems and meters. Highway length (100 % by default): each
    note shown that much earlier (125, 150 or 170 %), as in Clone Hero: the highway continues beyond its
    far end in its own perspective, and the notes reach the strike line at the same speed; the timing to
    hit them does not change; every player gets it; if it does not fit the screen it is shortened.
    Highway perspective (100 % by default): 25 to 150 %, flatter (as seen from higher, the far notes
    larger, the longer highway taller) or steeper. The notes take the same time to cross the highway, so
    below 100 % they reach the strike line slower (about a fifth slower at 75 %), above it faster. Notes
    at the console's speed (off by default, under the perspective): they reach the strike line as fast
    as at 100 % whatever the perspective, so below 100 % each note shows for less time (1.18 s on Expert
    at 75 % instead of 1.5); the game's Hyperspeed works on top of it. Highway style (Original by
    default): Dark is the community's black highway. Highway opacity (100 % by default): below it the
    venue shows through the highway (the notes and lines stay solid).
  - Sound: Whammy sound effect (on by default): off, the
    whammy bar no longer bends your sustained notes' pitch (it still wobbles them and fills Star Power).
    Lower the track on a miss (on by default, as on the console), and under it
    Instrument volume on a miss (Muted by default, as on the console): how much of your guitar's or bass's
    track stays when you miss a note or strum without one, 10 to 100 %; with two players on the same part
    only 100 % changes it.
    World Tour's Star Power sound (off by default): Star Power puts your instrument through Guitar Hero
    World Tour's echo, reverb and high pass instead of the venue's reverb. Two more settings in ghbandhero.toml
    raise it: ghbandhero_star_power_reverb_send_db (dB added to its send, 0 by default) and
    ghbandhero_star_power_reverb_multi_preset (true: World Tour's settings for several players, a large reverb
    with no echo, for one player too).
  - Tools: Game debug menu (off by default): the debug menu its developers used,
    in the main menu and the pause menu (its texts in English; Unlock All changes your
    save). Debug keys (off by default): F3, F4 and ` open the developer's windows in the game (frame
    times, settings, console).
  - Custom textures (NativeXRenderer only, no launcher row): an image in the "textures" folder next to
    GHBandHeroRecomp.exe, named by a texture's hash (16 hexadecimal digits, then anything; .png, .jpg or .jpeg;
    subfolders too), replaces that texture. The note highway's is DD482D15759B682F (512x1024; it repeats
    along the highway, so its top and bottom should join), for example "DD482D15759B682F my highway.png".
    To find others, add to ghbandhero.toml native_x_renderer_texture_log = true (each texture's hash in the log)
    or native_x_renderer_texture_dump = true (each one as a DDS file in texture_dump). The
    Highway's pictures win over this folder for the highway.
- Settings:
  - Display: the monitor, fullscreen, borderless or window, the resolution, the image format (fill a screen
    wider than 16:9, where the stage is seen wider while the note highway, the HUD and the menus stay 16:9
    in the center, or keep 16:9 with bars). Smoothness: the frame rate, 60 FPS (original), and 72, 75, 90, 100, 120,
    144, 160, 165 or 240 FPS (experimental; the songs keep their speed and stay in sync at every frame rate), and 300,
    360, 480 or Unlimited (experimental, with V-Sync off: with V-Sync on the game runs at your monitor's rate), V-Sync
    (on by default: no tearing, and no extra delay; leave it on with a G-Sync or FreeSync monitor too), and
    Venue at the full frame rate (off by default): the game draws its venue and its band in turns, each at half
    the frame rate (30 times a second on the console), while the note highway uses the full rate; on, the whole
    scene is drawn in every frame, for about twice the work for the processor and the graphics card (at 160 FPS
    a busy song can drop below it on a fast PC in GH Warriors of Rock; 120 FPS or lower leaves it room).
  - Graphics: the renderer and the graphics card. NativeXRenderer (the default) draws the game natively with
    Vulkan; Xenos emulates the Xbox 360 GPU instead (DirectX 12 or Vulkan): choose it if something looks wrong
    with NativeXRenderer, and tell us where. Graphics card: on a PC with two (a laptop's integrated and
    dedicated ones), the one the game draws with. Upscaling: AMD FSR 1 or NVIDIA NIS, and under it its quality
    and the internal resolution the game is drawn at. Image: antialiasing, sharpness, anisotropic filtering and
    the shadows' quality (up to four times the original resolution).
  - Effects: the game's screen effects (bloom, depth of field, color correction, film grain, vignette), each
    on by default; off, the grain goes with the color correction.
  - Audio: the output device (Windows default, or another one), the channels (Automatic: 5.1 on surround
    speakers, stereo on the rest; 5.1; Stereo; Mono: the same sound on both sides; the game is told its speakers,
    and the notes' timing stays the same with each), the volume (0 to 200 %: above 100 % the game is louder than
    its own mix and its loudest moments can distort), mute while another window is in front, and Low latency
    audio (off by default): the game takes your audio device in exclusive mode and the sound comes sooner (about
    55 ms on a USB audio interface); other programs cannot play on that device while the game runs.
    Calibrate the audio again after changing it.
  - Game: the launcher's language and the game's (English, French, German, Italian or Spanish; the game
    has no language menu of its own). Skip the intro video (off by default): the intro video at the start ends
    as soon as the game has loaded, and Party Play begins.
  - Updates: the launcher checks for a new version when it starts (you can turn that off) and installs it
    for you, keeping your saves, settings and game files. Get beta versions (off by default) offers the betas
    too. Play shows a new version as a button when there is one.


SINGING (EXPERIMENTAL)

Any microphone connected to the PC works as the game's Xbox 360 USB microphone:
1. In the launcher's Controls tab, choose Microphone (under the keyboard) and pick your microphone, or the
   Windows default. With an audio interface, pick the channel of the input your microphone is on.
2. Sing as loud as you will in a song and watch the Level bar: it should reach the yellow without touching
   the red. Change the Gain if it does not (it starts at -12 dB).
3. Choose the controller that sings and set its Instrument to Vocals. In Quickplay, that controller sings.
If the pitch line jumps around, lower the Gain; if the game does not hear you, raise it. Headphones help: with
speakers, the microphone hears the song too. While a microphone is chosen, Windows shows that the game is using
it.


MORE THAN ONE CONTROLLER

The game starts in Party Play. The first controller to press a button there plays with your profile and its
saves. The other controllers join as its guests, as on the Xbox 360 ("Name (1)", "Name (2)"): they play without
saves of their own.


QUITTING THE GAME

In the main menu, press B (the red fret): the game asks whether to quit. A (the green fret, or Enter) quits, B
(or Escape) goes back. The game starts in Party Play, where B is a player's red fret: press Start there to reach
the main menu.


STEAM BIG PICTURE

Add BandHeroRecomp.exe as a non-Steam game: it starts straight into the game with the options saved by
Band Hero Launcher. You can also add "Band Hero Launcher.exe" to change options with a controller.


FILES

- ghbandhero.toml           Base settings. Created with the defaults this game needs if it is missing.
- ghbandhero_options.toml   What Band Hero Launcher saves. Loaded after ghbandhero.toml.
- logs\                     Log files: the game's (ghbandhero_NNN.log) and the launcher's (launcher_NNN.log,
                            with a .dmp file when it stopped responding). Attach the latest ones when you report
                            a problem.
- userdata\                 The shader cache. Keep this folder if you move the game to another folder. If the
                            game folder cannot be written (for example under Program Files), it goes to
                            Documents\ghbandhero instead.
- Your saves, achievements and playtime are in your profile: %APPDATA%\ElGuslml Recomps.

Command line options (--name=value, for example in Steam launch options) win over both files, e.g.
--user_language=1 (English), --ghbandhero_frame_rate=120 or --native_x_renderer=false (the Xenos renderer).


REPORTING A PROBLEM

Tell us on Discord (https://discord.gg/XQtM5TmNs8) or open an issue on the project's page, with where in the
game it happened, a screenshot if you can, and the latest files in the logs folder (the game's and, if the
launcher misbehaved, the launcher's).


KNOWN ISSUES

- Band Hero shares its engine with Guitar Hero 5, with GH5Recomp's, GHVanHRecomp's, GHMetalRecomp's and
  GHWoRRecomp's fixes. Songs have been played with an Xbox 360 guitar; drums and singing have not been tested
  in this game yet.
- Singing is experimental: if the pitch line jumps or the game misses your voice, change the microphone's
  Gain in the launcher.
- Some USB audio interfaces add delay in Windows' shared audio mode (a Focusrite Scarlett about 20 ms more
  than a motherboard's audio): calibrate the audio in the game's options.
- Above 60 FPS is experimental: something may still run too fast. Tell us what and where.
- Playing with the keyboard is experimental.
- Downloadable content: the packages are read (Guitar Hero 5's among them, as on the console), but their songs
  have not been played in this game yet.
- The first time an effect appears, it may show up a few frames late.
- Xbox One guitars (PDP Riffmaster, PDP Jaguar, Mad Catz Stratocaster) have not been played with yet: tell us
  how yours goes, with the game's latest log.


MORE INFORMATION

{MORE_INFO}

BandHRecomp's own code is under the MIT license (LICENSE). It is built on the ReXGlue SDK and other
components under their own licenses: see THIRD_PARTY_NOTICES.md and the "licenses" folder.
