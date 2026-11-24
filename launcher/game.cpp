// This game's own texts (game.h): the first run's screens and the Instruments and gamepad page in the game's spirit
// (GH Van Halen's, from its mockup), the Game page's switches, where the game shows its DLC, and the guitar's and the
// drums' functions on the Keyboard and Gamepad pages, in the launcher's languages (English, German, French, Spanish,
// Italian).

#include "game.h"

#include "settings.h"

namespace launcher::game {

const GameTexts& GameTextsFor(uint32_t language) {
  static const GameTexts kEnglish{
      .welcome_title = "Welcome backstage",
      .welcome_text = "Before you hit the stage, the game has to be installed. The launcher extracts its files from "
                      "your Xbox 360 ISO; you only have to do this once.",
      .toggles = {
          "Bloom", "The glow around lights and bright surfaces.",
          "Depth of field", "The background blurred behind what is in focus.",
          "Color correction", "Each venue's look; off turns the grain off too.",
          "Film grain", "The grain over the image. Needs the color correction.",
          "Vignette", "Darker corners, where a venue turns it on.",
          "Venue at the full frame rate", "The venue and the band drawn in every frame, not every other one.",
          "Skip the intro video", "The intro ends as soon as the game has loaded.",
          "Game debug menu", "Neversoft's debug menu in the main menu and the pause menu.",
          "Whammy sound effect", "The whammy bar bends the pitch of your sustained notes.",
          "World Tour's Star Power sound", "Star Power with World Tour's echo and reverb.",
          "Lower the track on a miss", "Your part quieter after a miss, as on the console.",
          "Notes at the console's speed", "The notes reach the strike line as fast as at 100 %.",
      },
      .toggle_help = {
          "The halo of light around the stage's lights, the pyrotechnics and the brightest surfaces. Off, the image is sharper and flatter.",
          "Blurs the background in some camera shots and behind the menus and the pause menu, as a camera's lens does. Off, everything stays sharp.",
          "The brightness, contrast and saturation each venue sets, the fades to black and the grey image while the rock meter is red. Off, the colors are the scene's own and the film grain goes off with it: without this pass the grain is far stronger.",
          "A fine moving grain over the image, as on film. It only shows with the color correction on.",
          "Darkens the corners of the image in the venues and moments that use it.",
          "The game draws its 3D scene over two frames: the venue and the crowd in one, the band and the screen effects in the next. So the stage moves at half the frame rate (30 times a second on the console) while the note highway and the HUD use the full rate. On, the whole scene is drawn in every frame: the stage as smooth as the highway, for about twice the work a frame for the processor and the graphics card: at 160 FPS even a fast PC can drop to about 130 in busy songs, while 120 FPS or lower leaves it room. If the frame rate drops, lower it or turn this off.",
          "The game plays its intro video while it loads at each start. With this on, the video ends as soon as the game has loaded, as if you pressed Start then, and Party Play begins at once.",
          "Adds a \"(debug menu)\" item to the main menu and a Debug Menu item to the pause menu: the menu the game's developers used (Unlock All, Skip Into Song, Change Venue, Select Slomo, Show Song Time, Toggle GPU Time, ...). Its texts are in English, some items do nothing without the developers' tools, and Unlock All changes your save as playing does.",
          "On, as on the console, moving the whammy bar on a sustained note bends your instrument's pitch. Off, the note keeps its pitch; the whammy bar still wobbles the note on the highway and still fills Star Power on Star Power notes.",
          "In Guitar Hero World Tour, Star Power sent your instrument through an echo timed to the song, a reverb and a high pass, set for each instrument and for one player or several, until Star Power ended. Band Hero kept those settings but only raises the venue's reverb. With this on, Star Power sounds as in World Tour; the band, the crowd and the vocals do not change.",
          "On, as on the console, a missed note or a strum without one lowers your instrument's track until you hit a note again; the volume under it says how much is left (0 % silences it). Off, your track never changes.",
          "With a perspective other than 100 %, the notes take the same time to cross the highway, so they reach the strike line slower below 100 % (about a fifth slower at 75 %) and faster above it. On, the time on the highway changes with the perspective so they reach it at the console's speed: below 100 % each note shows for less time (on Expert 1.18 s instead of 1.5 at 75 %; a longer highway gives it back), above 100 % for longer. The game's Hyperspeed works on top of it as usual. With two or more players their highways' own perspectives make the notes a little faster than on the console (4-7 % at 75 %). Off, the console's times.",
      },
      .choices = {
          "Highway scale", "The note highway's size when you play alone.",
          "Highway length", "Notes seen earlier, from further away.",
          "Highway perspective", "How tilted the highway looks: flatter or steeper.",
          "Highway style", "The look of the highway behind the notes.",
          "Highway opacity", "How much of the venue shows through the highway.",
          "Instrument volume on a miss", "How loud your part stays when you miss a note.",
      },
      .choice_help = {
          "Makes the note highway smaller or larger when you play alone, anchored at the strike line: the gems, the sidebars, the rock meter and the Star Power meter with it. At 80 % more of the stage shows around it. 100 % is the console's size; with two or more players the game's own sizes stay.",
          "Shows each note that much earlier, as in Clone Hero: the console's highway continues beyond its far end in its own perspective, so it gets a little taller and the extra notes come from further away, while every note still reaches the strike line at the same speed. On Expert a note normally shows 1.5 s before you play it; at 150 %, 2.25 s. The timing to hit the notes does not change. It applies to every player. With a flatter perspective the longer part takes more of the screen. 100 % is the console's highway.",
          "The highway's perspective, as if the camera looked from higher (below 100 %: the far end wider and its notes larger, more of the highway easy to read) or lower (above 100 %: the far end narrower and more compressed). The notes take the same time to cross the highway, so below 100 % they move slower near the strike line (about a fifth slower at 75 %) and above 100 % faster: the switch under it keeps the console's speed, or the game's Hyperspeed makes up for it. The length above continues the same perspective. 100 % is the console's.",
          "Dark is the community's black highway: the highway's picture turned black, so the gems, the sustains and the lines stand out against it.",
          "Below 100 % the venue shows through the note highway: the notes, the lines and the sidebars stay solid. 100 % is the console's opaque highway.",
          "When you miss a note or strum without one, the game mutes your instrument's track until you hit a note again. Here you choose how much of it stays: 0 % mutes it as on the console, 50 % halves it, 100 % keeps it as it was. Guitar and bass; with two players on the same part (they share its track) only 100 % changes it. The drums are not changed.",
      },
      .choice_values = {{{}, {}, {}, {"Original", "Dark"}, {}, {"Muted"}}},
      .done_title = "The stage is yours!",
      .done_text = "The game was installed. Go play now, or first review the display, graphics and instrument settings; "
                   "you can always change them in the launcher.",
      .dlc_where = "The game lists the songs in Quickplay's song list.",
      .extracting_title = "Tuning up the stage",
      .extracting_text = "The game is being extracted. It can take a few minutes; do not close the launcher.",
      .keyboard_tap_title = "Strum when a fret is pressed",
      .keyboard_tap_hint = "Each fret key strums as you press it; frets pressed within 20 ms of each other are one chord, one strum.",
      .part_instrument = "Guitar",
      .part_vocals = "Vocals",
  };
  static const GameTexts kGerman{
      .welcome_title = "Willkommen im Backstage",
      .welcome_text = "Bevor es auf die Bühne geht, muss das Spiel installiert werden. Der Launcher entpackt die Dateien "
                      "aus deinem Xbox-360-ISO; das musst du nur einmal tun.",
      .toggles = {
          "Bloom", "Das Leuchten um Lichter und helle Flächen.",
          "Tiefenschärfe", "Der unscharfe Hintergrund hinter dem Fokus.",
          "Farbkorrektur", "Der Look jedes Ortes; aus schaltet auch die Körnung aus.",
          "Filmkörnung", "Die Körnung über dem Bild. Braucht die Farbkorrektur.",
          "Vignette", "Dunklere Ecken, wo ein Ort sie einschaltet.",
          "Bühne mit voller Bildrate", "Bühne und Band in jedem Bild statt in jedem zweiten.",
          "Intro-Video überspringen", "Das Intro endet, sobald das Spiel geladen ist.",
          "Debug-Menü des Spiels", "Das Debug-Menü von Neversoft im Hauptmenü und im Pausenmenü.",
          "Whammy-Klangeffekt", "Der Whammy-Hebel verbiegt die Tonhöhe gehaltener Noten.",
          "Star-Power-Klang aus World Tour", "Star Power mit dem Echo und dem Hall aus World Tour.",
          "Spur bei Fehlern leiser", "Dein Part nach einem Fehler leiser, wie auf der Konsole.",
          "Noten im Tempo der Konsole", "Die Noten erreichen die Schlaglinie so schnell wie bei 100 %.",
      },
      .toggle_help = {
          "Der Lichtschein um die Bühnenlichter, die Pyrotechnik und die hellsten Flächen. Aus wirkt das Bild schärfer und flacher.",
          "Macht den Hintergrund in manchen Kameraeinstellungen sowie hinter den Menüs und dem Pausenmenü unscharf, wie ein Kameraobjektiv. Aus bleibt alles scharf.",
          "Helligkeit, Kontrast und Sättigung jedes Auftrittsortes, die Abblenden ins Schwarze und das graue Bild, solange die Rock-Anzeige rot ist. Aus zeigt die eigenen Farben der Szene, und die Filmkörnung geht mit aus: ohne diesen Durchgang ist sie viel stärker.",
          "Eine feine, bewegte Körnung über dem Bild, wie bei Film. Nur mit eingeschalteter Farbkorrektur sichtbar.",
          "Dunkelt die Bildecken an den Orten und in den Momenten ab, die sie verwenden.",
          "Das Spiel zeichnet seine 3D-Szene über zwei Bilder: den Auftrittsort und das Publikum in einem, die Band und die Bildeffekte im nächsten. So bewegt sich die Bühne mit der halben Bildrate (auf der Konsole 30-mal pro Sekunde), während Notenbahn und HUD die volle nutzen. Eingeschaltet wird die ganze Szene in jedem Bild gezeichnet: die Bühne so flüssig wie die Notenbahn, für etwa die doppelte Arbeit pro Bild für Prozessor und Grafikkarte: Bei 160 FPS kann selbst ein schneller PC in vollen Songs auf etwa 130 fallen, bei 120 FPS oder weniger bleibt Luft. Sinkt die Bildrate, senke sie oder schalte es aus.",
          "Das Spiel zeigt bei jedem Start sein Intro-Video, während es lädt. Eingeschaltet endet das Video, sobald das Spiel geladen ist, als hättest du in dem Moment Start gedrückt, und das Spiel geht sofort los.",
          "Fügt dem Hauptmenü den Eintrag \"(debug menu)\" und dem Pausenmenü den Eintrag Debug Menu hinzu: das Menü der Entwickler des Spiels (Unlock All, Skip Into Song, Change Venue, Select Slomo, Show Song Time, Toggle GPU Time, ...). Seine Texte sind englisch, manche Einträge tun ohne die Werkzeuge der Entwickler nichts, und Unlock All ändert deinen Spielstand wie das Spielen.",
          "An, wie auf der Konsole: Bewegst du den Whammy-Hebel auf einer gehaltenen Note, verbiegt er die Tonhöhe deines Instruments. Aus: Die Note behält ihre Tonhöhe; der Whammy-Hebel lässt die Note auf der Bahn weiter wackeln und füllt auf Star-Power-Noten weiter Star Power.",
          "In Guitar Hero World Tour schickte Star Power dein Instrument durch ein Echo im Takt des Songs, einen Hall und einen Hochpass, eingestellt für jedes Instrument und für einen oder mehrere Spieler, bis Star Power endete. Band Hero behielt diese Einstellungen, verstärkt aber nur den Hall des Auftrittsortes. Eingeschaltet klingt Star Power wie in World Tour; die Band, das Publikum und der Gesang ändern sich nicht.",
          "An, wie auf der Konsole: Eine verpasste Note oder ein Anschlag ohne Note macht die Spur deines Instruments leiser, bis du wieder eine Note triffst; die Lautstärke darunter sagt, wie viel bleibt (0 % schaltet sie stumm). Aus: Deine Spur ändert sich nie.",
          "Mit einer Perspektive außer 100 % brauchen die Noten gleich lang über die Bahn, erreichen die Schlaglinie also unter 100 % langsamer (bei 75 % etwa ein Fünftel) und über 100 % schneller. Eingeschaltet ändert sich die Zeit auf der Bahn mit der Perspektive, sodass sie sie im Tempo der Konsole erreichen: Unter 100 % ist jede Note kürzer zu sehen (auf Experte bei 75 % 1,18 s statt 1,5; eine längere Bahn gleicht das aus), über 100 % länger. Hyperspeed im Spiel wirkt wie gewohnt zusätzlich. Mit zwei oder mehr Spielern sind die Noten durch die eigenen Perspektiven ihrer Bahnen etwas schneller als auf der Konsole (4-7 % bei 75 %). Aus: die Zeiten der Konsole.",
      },
      .choices = {
          "Notenbahn-Größe", "Die Größe der Notenbahn, wenn du allein spielst.",
          "Notenbahn-Länge", "Noten früher sehen, von weiter her.",
          "Notenbahn-Perspektive", "Wie geneigt die Bahn wirkt: flacher oder steiler.",
          "Notenbahn-Stil", "Das Aussehen der Bahn hinter den Noten.",
          "Deckkraft der Notenbahn", "Wie viel von der Location durch die Bahn scheint.",
          "Instrument-Lautstärke bei Fehlern", "Wie laut deine Spur bleibt, wenn du eine Note verpasst.",
      },
      .choice_help = {
          "Macht die Notenbahn kleiner oder größer, wenn du allein spielst, verankert an der Schlaglinie: Noten, Seitenleisten, Rock-Meter und Star-Power-Anzeige mit ihr. Bei 80 % ist mehr von der Bühne zu sehen. 100 % ist die Größe der Konsole; mit zwei oder mehr Spielern bleiben die Größen des Spiels.",
          "Zeigt jede Note entsprechend früher, wie in Clone Hero: Die Notenbahn der Konsole setzt sich über ihr fernes Ende hinaus in ihrer eigenen Perspektive fort, wird dadurch etwas höher, und die zusätzlichen Noten kommen von weiter her, während jede Note die Schlaglinie mit derselben Geschwindigkeit erreicht. Auf Experte erscheint eine Note normalerweise 1,5 s, bevor du sie spielst; bei 150 % 2,25 s. Das Timing zum Treffen der Noten ändert sich nicht. Gilt für alle Spieler. Mit einer flacheren Perspektive nimmt der längere Teil mehr vom Bildschirm ein. 100 % ist die Notenbahn der Konsole.",
          "Die Perspektive der Notenbahn, als blicke die Kamera von höher (unter 100 %: das ferne Ende breiter, seine Noten größer, mehr von der Bahn gut lesbar) oder von tiefer (über 100 %: das ferne Ende schmaler und gestauchter). Die Noten brauchen gleich lang über die Bahn, daher bewegen sie sich unter 100 % nahe der Schlaglinie langsamer (bei 75 % etwa ein Fünftel) und über 100 % schneller: Der Schalter darunter behält das Tempo der Konsole, oder Hyperspeed im Spiel gleicht das aus. Die Länge oben setzt dieselbe Perspektive fort. 100 % ist die der Konsole.",
          "Dunkel ist die schwarze Notenbahn der Community: Das Bild der Bahn wird schwarz, sodass Noten, gehaltene Noten und Linien hervorstechen.",
          "Unter 100 % scheint die Location durch die Notenbahn: Noten, Linien und Seitenleisten bleiben deckend. 100 % ist die deckende Bahn der Konsole.",
          "Verpasst du eine Note oder schlägst ohne eine an, schaltet das Spiel die Spur deines Instruments stumm, bis du wieder eine Note triffst. Hier wählst du, wie viel davon bleibt: 0 % schaltet sie stumm wie auf der Konsole, 50 % halbiert sie, 100 % lässt sie, wie sie war. Gitarre und Bass; spielen zwei Spieler denselben Part (sie teilen sich seine Spur), ändert nur 100 % etwas. Das Schlagzeug ändert sich nicht.",
      },
      .choice_values = {{{}, {}, {}, {"Original", "Dunkel"}, {}, {"Stumm"}}},
      .done_title = "Die Bühne gehört dir!",
      .done_text = "Das Spiel wurde installiert. Leg gleich los oder sieh dir zuerst die Anzeige-, Grafik- und "
                   "Instrumenteinstellungen an; du kannst sie jederzeit im Launcher ändern.",
      .dlc_where = "Das Spiel zeigt die Songs in der Songliste des Schnellspiels.",
      .extracting_title = "Die Bühne wird gestimmt",
      .extracting_text = "Das Spiel wird entpackt. Das kann ein paar Minuten dauern; schließe den Launcher nicht.",
      .keyboard_tap_title = "Anschlag beim Drücken eines Bunds",
      .keyboard_tap_hint = "Jede Bundtaste schlägt an, wenn du sie drückst; Bünde, die du innerhalb von 20 ms drückst, sind ein Akkord, ein Anschlag.",
      .part_instrument = "Gitarre",
      .part_vocals = "Gesang",
  };
  static const GameTexts kFrench{
      .welcome_title = "Bienvenue en coulisses",
      .welcome_text = "Avant de monter sur scène, il faut installer le jeu. Le launcher extrait ses fichiers de votre ISO "
                      "Xbox 360 ; vous n'aurez à le faire qu'une fois.",
      .toggles = {
          "Bloom", "La lueur autour des lumières et des surfaces claires.",
          "Profondeur de champ", "L'arrière-plan flou derrière le sujet net.",
          "Correction des couleurs", "L'ambiance de chaque salle ; désactivée, le grain l'est aussi.",
          "Grain", "Le grain sur l'image. Nécessite la correction des couleurs.",
          "Vignettage", "Des coins plus sombres, là où une salle l'active.",
          "Scène à pleine fréquence", "La salle et le groupe dessinés à chaque image, pas une sur deux.",
          "Passer la vidéo d'intro", "L'intro s'arrête dès que le jeu est chargé.",
          "Menu de débogage du jeu", "Le menu de débogage de Neversoft dans le menu principal et le menu pause.",
          "Effet sonore du vibrato", "La barre de vibrato fait varier la hauteur des notes tenues.",
          "Son du Star Power de World Tour", "Le Star Power avec l'écho et la réverbération de World Tour.",
          "Baisser la piste sur une erreur", "Votre partie moins forte après une erreur, comme sur la console.",
          "Notes à la vitesse de la console", "Les notes arrivent à la ligne de frappe aussi vite qu'à 100 %.",
      },
      .toggle_help = {
          "Le halo autour des lumières de la scène, de la pyrotechnie et des surfaces les plus claires. Désactivé, l'image est plus nette et plus plate.",
          "Floute l'arrière-plan dans certains plans de caméra et derrière les menus et le menu pause, comme un objectif. Désactivée, tout reste net.",
          "La luminosité, le contraste et la saturation de chaque salle, les fondus au noir et l'image grise tant que la jauge rock est dans le rouge. Désactivée, les couleurs sont celles de la scène et le grain s'éteint avec elle : sans cette passe, il est bien plus fort.",
          "Un grain fin et animé sur l'image, comme sur pellicule. Il n'apparaît qu'avec la correction des couleurs activée.",
          "Assombrit les coins de l'image dans les salles et les moments qui l'utilisent.",
          "Le jeu dessine sa scène 3D sur deux images : la salle et le public dans l'une, le groupe et les effets d'image dans la suivante. La scène bouge donc à la moitié de la fréquence d'images (30 fois par seconde sur la console), tandis que la piste de notes et l'interface utilisent la fréquence complète. Activée, toute la scène est dessinée à chaque image : la scène aussi fluide que la piste, pour environ deux fois plus de travail par image pour le processeur et la carte graphique : à 160 FPS, même un PC rapide peut descendre vers 130 dans les chansons chargées, alors qu'à 120 FPS ou moins il garde de la marge. Si la fréquence d'images baisse, réduisez-la ou désactivez cette option.",
          "À chaque démarrage, le jeu passe sa vidéo d'intro pendant qu'il charge. Activée, la vidéo s'arrête dès que le jeu est chargé, comme si vous appuyiez alors sur Start, et le jeu commence aussitôt.",
          "Ajoute l'entrée « (debug menu) » au menu principal et l'entrée Debug Menu au menu pause : le menu des développeurs du jeu (Unlock All, Skip Into Song, Change Venue, Select Slomo, Show Song Time, Toggle GPU Time, ...). Ses textes sont en anglais, certaines entrées ne font rien sans les outils des développeurs, et Unlock All modifie votre sauvegarde comme le jeu.",
          "Activé, comme sur la console : bouger la barre de vibrato sur une note tenue fait varier la hauteur de votre instrument. Désactivé, la note garde sa hauteur ; la barre de vibrato fait toujours onduler la note sur la piste et remplit toujours le Star Power sur les notes Star Power.",
          "Dans Guitar Hero World Tour, le Star Power faisait passer votre instrument par un écho calé sur le tempo de la chanson, une réverbération et un passe-haut, réglés pour chaque instrument et pour un ou plusieurs joueurs, jusqu'à la fin du Star Power. Band Hero a gardé ces réglages mais ne fait que monter la réverbération de la salle. Activé, le Star Power sonne comme dans World Tour ; le groupe, le public et le chant ne changent pas.",
          "Activé, comme sur la console : une note ratée ou un grattement sans note baisse la piste de votre instrument jusqu'à la prochaine note réussie ; le volume dessous dit ce qu'il en reste (0 % la coupe). Désactivé, votre piste ne change jamais.",
          "Avec une perspective autre que 100 %, les notes mettent le même temps à parcourir la piste : elles arrivent donc à la ligne de frappe plus lentement sous 100 % (environ un cinquième de moins à 75 %) et plus vite au-dessus. Activé, le temps sur la piste change avec la perspective pour qu'elles y arrivent à la vitesse de la console : sous 100 % chaque note reste visible moins longtemps (en Expert 1,18 s au lieu de 1,5 à 75 % ; une piste plus longue le rend), au-dessus de 100 % plus longtemps. L'Hyperspeed du jeu s'ajoute comme d'habitude. À deux joueurs ou plus, les perspectives propres de leurs pistes rendent les notes un peu plus rapides que sur la console (4 à 7 % à 75 %). Désactivé, les temps de la console.",
      },
      .choices = {
          "Taille de la piste", "La taille de la piste de notes quand vous jouez seul.",
          "Longueur de la piste", "Les notes vues plus tôt, venant de plus loin.",
          "Perspective de la piste", "L'inclinaison de la piste : plus plate ou plus raide.",
          "Style de la piste", "L'aspect de la piste derrière les notes.",
          "Opacité de la piste", "Ce qu'on voit de la salle à travers la piste.",
          "Volume de l'instrument sur une erreur", "Le volume de votre partie quand vous ratez une note.",
      },
      .choice_help = {
          "Réduit ou agrandit la piste de notes quand vous jouez seul, ancrée à la ligne de frappe : les notes, les bords, le rock-mètre et la jauge de Star Power avec elle. À 80 %, on voit davantage la scène autour. 100 % est la taille de la console ; à deux joueurs ou plus, les tailles du jeu restent.",
          "Montre chaque note d'autant plus tôt, comme dans Clone Hero : la piste de la console se prolonge au-delà de son bout lointain dans sa propre perspective, elle devient donc un peu plus haute et les notes en plus viennent de plus loin, tandis que chaque note arrive à la ligne de frappe à la même vitesse. En Expert, une note apparaît normalement 1,5 s avant que vous la jouiez ; à 150 %, 2,25 s. Le timing pour toucher les notes ne change pas. S'applique à tous les joueurs. Avec une perspective plus plate, la partie en plus prend davantage d'écran. 100 % est la piste de la console.",
          "La perspective de la piste, comme si la caméra regardait de plus haut (sous 100 % : le bout lointain plus large et ses notes plus grandes, plus de piste facile à lire) ou de plus bas (au-dessus de 100 % : le bout lointain plus étroit et plus tassé). Les notes mettent le même temps à parcourir la piste : sous 100 % elles avancent donc plus lentement près de la ligne de frappe (environ un cinquième de moins à 75 %), et plus vite au-dessus de 100 % ; l'interrupteur dessous garde la vitesse de la console, ou l'Hyperspeed du jeu le compense. La longueur ci-dessus prolonge la même perspective. 100 % est celle de la console.",
          "Sombre est la piste noire de la communauté : l'image de la piste devient noire, pour que les notes, les notes tenues et les lignes ressortent.",
          "Sous 100 %, la salle se voit à travers la piste de notes : les notes, les lignes et les bords restent opaques. 100 % est la piste opaque de la console.",
          "Quand vous ratez une note ou grattez sans note, le jeu coupe la piste de votre instrument jusqu'à la prochaine note réussie. Ici vous choisissez ce qu'il en reste : 0 % la coupe comme sur la console, 50 % la réduit de moitié, 100 % la laisse telle quelle. Guitare et basse ; avec deux joueurs sur la même partie (ils partagent sa piste), seul 100 % la change. La batterie ne change pas.",
      },
      .choice_values = {{{}, {}, {}, {"Originale", "Sombre"}, {}, {"Coupé"}}},
      .done_title = "La scène est à vous !",
      .done_text = "Le jeu a été installé. Allez jouer tout de suite, ou vérifiez d'abord les réglages d'affichage, "
                   "graphiques et de vos instruments ; vous pourrez toujours les changer dans le launcher.",
      .dlc_where = "Le jeu affiche les chansons dans la liste de la Partie rapide.",
      .extracting_title = "On accorde la scène",
      .extracting_text = "Le jeu est en cours d'extraction. Cela peut prendre quelques minutes ; ne fermez pas le "
                         "launcher.",
      .keyboard_tap_title = "Gratter en appuyant sur une frette",
      .keyboard_tap_hint = "Chaque touche de frette gratte quand vous l'appuyez ; les frettes appuyées à moins de 20 ms d'intervalle forment un accord, un seul grattage.",
      .part_instrument = "Guitare",
      .part_vocals = "Chant",
  };
  static const GameTexts kSpanish{
      .welcome_title = "Bienvenido al backstage",
      .welcome_text = "Antes de subir al escenario hay que instalar el juego. El launcher extrae los archivos desde tu "
                      "ISO de Xbox 360; solo tendrás que hacerlo una vez.",
      .toggles = {
          "Bloom", "El resplandor alrededor de las luces y superficies claras.",
          "Profundidad de campo", "El fondo desenfocado detrás de lo enfocado.",
          "Corrección de color", "El aspecto de cada escenario; al desactivarla se apaga el grano.",
          "Grano de película", "El grano sobre la imagen. Necesita la corrección de color.",
          "Viñeta", "Esquinas más oscuras, donde un escenario la activa.",
          "Escenario a cuadros completos", "El escenario y la banda en cada cuadro, no en uno de cada dos.",
          "Saltar el video de introducción", "La intro termina en cuanto el juego cargó.",
          "Menú de depuración del juego", "El menú de depuración de Neversoft en el menú principal y en el de pausa.",
          "Efecto de sonido del whammy", "La palanca de whammy dobla el tono de tus notas largas.",
          "Sonido del Star Power de World Tour", "El Star Power con el eco y la reverberación de World Tour.",
          "Bajar la pista al fallar", "Tu parte más baja después de fallar, como en la consola.",
          "Notas a la velocidad de la consola", "Las notas llegan a la línea de golpe tan rápido como al 100 %.",
      },
      .toggle_help = {
          "El halo de luz alrededor de las luces del escenario, la pirotecnia y las superficies más brillantes. Desactivado, la imagen se ve más nítida y plana.",
          "Desenfoca el fondo en algunas tomas de cámara y detrás de los menús y la pausa, como el lente de una cámara. Desactivada, todo se ve nítido.",
          "El brillo, el contraste y la saturación de cada escenario, los fundidos a negro y la imagen gris mientras el medidor de rock está en rojo. Desactivada, se ven los colores propios de la escena y el grano de película se apaga con ella: sin esta pasada el grano es mucho más fuerte.",
          "Un grano fino y en movimiento sobre la imagen, como en una película. Solo se ve con la corrección de color activada.",
          "Oscurece las esquinas de la imagen en los escenarios y momentos que la usan.",
          "El juego dibuja su escena 3D en dos cuadros: el escenario y el público en uno, la banda y los efectos de imagen en el siguiente. Así el escenario se mueve a la mitad de los cuadros por segundo (30 veces por segundo en la consola), mientras el highway de notas y el HUD van a la tasa completa. Activado, toda la escena se dibuja en cada cuadro: el escenario tan fluido como el highway, a cambio de cerca del doble de trabajo por cuadro para el procesador y la tarjeta gráfica: a 160 FPS incluso un PC rápido puede bajar a unos 130 en canciones cargadas, mientras que a 120 FPS o menos le queda margen. Si bajan los cuadros por segundo, bájalos o desactívalo.",
          "En cada inicio el juego muestra su video de introducción mientras carga. Activado, el video termina en cuanto el juego cargó, como si pulsaras Start en ese momento, y el juego empieza enseguida.",
          "Añade la opción «(debug menu)» al menú principal y Debug Menu al de pausa: el menú que usaban los desarrolladores del juego (Unlock All, Skip Into Song, Change Venue, Select Slomo, Show Song Time, Toggle GPU Time, ...). Sus textos están en inglés, algunas opciones no hacen nada sin las herramientas de los desarrolladores, y Unlock All cambia tu partida guardada como jugar.",
          "Activado, como en la consola, mover la palanca de whammy en una nota larga dobla el tono de tu instrumento. Desactivado, la nota mantiene su tono; la palanca sigue haciendo ondular la nota en el highway y sigue llenando el Star Power en las notas de Star Power.",
          "En Guitar Hero World Tour, el Star Power pasaba tu instrumento por un eco al tempo de la canción, una reverberación y un filtro pasa altos, ajustados para cada instrumento y para uno o varios jugadores, hasta que terminaba el Star Power. Band Hero conservó esos ajustes pero solo sube la reverberación del escenario. Activado, el Star Power suena como en World Tour; la banda, el público y la voz no cambian.",
          "Activado, como en la consola, una nota fallada o un rasgueo sin nota bajan la pista de tu instrumento hasta que aciertes otra; el volumen de abajo dice cuánto queda (0 % la silencia). Desactivado, tu pista nunca cambia.",
          "Con una perspectiva distinta de 100 % las notas tardan lo mismo en recorrer el highway, así que llegan a la línea de golpe más lento bajo 100 % (cerca de una quinta parte menos al 75 %) y más rápido sobre 100 %. Activado, el tiempo en el highway cambia con la perspectiva para que lleguen a la velocidad de la consola: bajo 100 % cada nota se ve menos tiempo (en Experto 1,18 s en vez de 1,5 al 75 %; un highway más largo lo devuelve), sobre 100 % más tiempo. El Hyperspeed del juego funciona encima como siempre. Con dos o más jugadores, las perspectivas propias de sus highways hacen las notas algo más rápidas que en la consola (4-7 % al 75 %). Desactivado, los tiempos de la consola.",
      },
      .choices = {
          "Escala del highway", "El tamaño del highway de notas cuando tocas solo.",
          "Largo del highway", "Las notas se ven antes, desde más lejos.",
          "Perspectiva del highway", "Qué tan inclinado se ve el highway: más plano o más empinado.",
          "Estilo del highway", "El aspecto del highway detrás de las notas.",
          "Opacidad del highway", "Cuánto del escenario se ve a través del highway.",
          "Volumen del instrumento al fallar", "Qué tan fuerte queda tu parte cuando fallas una nota.",
      },
      .choice_help = {
          "Hace el highway de notas más pequeño o más grande cuando tocas solo, anclado en la línea de golpe: las notas, los bordes, el medidor de rock y el de Star Power con él. Al 80 % se ve más del escenario alrededor. 100 % es el tamaño de la consola; con dos o más jugadores se quedan los tamaños del juego.",
          "Muestra cada nota así de antes, como en Clone Hero: el highway de la consola continúa más allá de su extremo lejano en su propia perspectiva, así que se hace un poco más alto y las notas de más vienen desde más lejos, mientras cada nota llega a la línea de golpe a la misma velocidad. En Experto una nota aparece normalmente 1,5 s antes de tocarla; al 150 %, 2,25 s. El momento para acertar las notas no cambia. Vale para todos los jugadores. Con una perspectiva más plana, la parte de más ocupa más pantalla. 100 % es el highway de la consola.",
          "La perspectiva del highway, como si la cámara mirara desde más arriba (bajo 100 %: el extremo lejano más ancho y sus notas más grandes, se lee más del highway) o desde más abajo (sobre 100 %: el extremo lejano más angosto y más comprimido). Las notas tardan lo mismo en recorrer el highway, así que bajo 100 % se mueven más lento cerca de la línea de golpe (cerca de una quinta parte menos al 75 %) y sobre 100 % más rápido: el interruptor de abajo mantiene la velocidad de la consola, o el Hyperspeed del juego lo compensa. El largo de arriba continúa la misma perspectiva. 100 % es la de la consola.",
          "Oscuro es el highway negro de la comunidad: la imagen del highway se vuelve negra, así las notas, las notas largas y las líneas resaltan.",
          "Bajo 100 % el escenario se ve a través del highway de notas: las notas, las líneas y los bordes siguen opacos. 100 % es el highway opaco de la consola.",
          "Cuando fallas una nota o rasgueas sin nota, el juego silencia la pista de tu instrumento hasta que aciertes otra. Aquí eliges cuánto queda: 0 % la silencia como en la consola, 50 % la deja a la mitad, 100 % la deja como estaba. Guitarra y bajo; con dos jugadores en la misma parte (comparten su pista) solo 100 % la cambia. La batería no cambia.",
      },
      .choice_values = {{{}, {}, {}, {"Original", "Oscuro"}, {}, {"Silenciado"}}},
      .done_title = "¡El escenario es tuyo!",
      .done_text = "El juego se instaló correctamente. Puedes salir a tocar ya o revisar primero los ajustes de pantalla, "
                   "gráficos y tus instrumentos; siempre podrás cambiarlos desde el launcher.",
      .dlc_where = "El juego muestra las canciones en la lista de canciones de Partida rápida.",
      .extracting_title = "Afinando el escenario",
      .extracting_text = "Estamos extrayendo el juego. Puede tardar unos minutos; no cierres el launcher.",
      .keyboard_tap_title = "Activar rasgueo al presionar botón",
      .keyboard_tap_hint = "Cada tecla de traste rasguea al presionarla; los trastes que presiones con menos de 20 ms de diferencia cuentan como un acorde: un solo rasgueo.",
      .part_instrument = "Guitarra",
      .part_vocals = "Voz",
  };
  static const GameTexts kItalian{
      .welcome_title = "Benvenuto nel backstage",
      .welcome_text = "Prima di salire sul palco bisogna installare il gioco. Il launcher estrae i file dalla tua ISO "
                      "Xbox 360; dovrai farlo una sola volta.",
      .toggles = {
          "Bloom", "Il bagliore attorno alle luci e alle superfici chiare.",
          "Profondità di campo", "Lo sfondo sfocato dietro ciò che è a fuoco.",
          "Correzione del colore", "L'aspetto di ogni locale; disattivata spegne anche la grana.",
          "Grana", "La grana sull'immagine. Richiede la correzione del colore.",
          "Vignettatura", "Angoli più scuri, dove un locale la attiva.",
          "Palco a frequenza piena", "Il locale e la band disegnati in ogni fotogramma, non uno sì e uno no.",
          "Salta il video introduttivo", "L'intro finisce appena il gioco è caricato.",
          "Menu di debug del gioco", "Il menu di debug di Neversoft nel menu principale e nel menu di pausa.",
          "Effetto sonoro della leva whammy", "La leva whammy piega l'intonazione delle note lunghe.",
          "Suono dello Star Power di World Tour", "Lo Star Power con l'eco e il riverbero di World Tour.",
          "Abbassa la traccia sugli errori", "La tua parte più bassa dopo un errore, come sulla console.",
          "Note alla velocità della console", "Le note arrivano alla linea di battuta veloci come al 100 %.",
      },
      .toggle_help = {
          "L'alone di luce attorno alle luci del palco, alla pirotecnica e alle superfici più luminose. Disattivato, l'immagine è più nitida e piatta.",
          "Sfoca lo sfondo in alcune inquadrature e dietro i menu e il menu di pausa, come l'obiettivo di una telecamera. Disattivata, tutto resta nitido.",
          "La luminosità, il contrasto e la saturazione di ogni locale, le dissolvenze al nero e l'immagine grigia mentre l'indicatore rock è nel rosso. Disattivata, si vedono i colori propri della scena e la grana si spegne con essa: senza questo passaggio è molto più forte.",
          "Una grana fine e in movimento sull'immagine, come su pellicola. Si vede solo con la correzione del colore attiva.",
          "Scurisce gli angoli dell'immagine nei locali e nei momenti che la usano.",
          "Il gioco disegna la sua scena 3D su due fotogrammi: il locale e il pubblico in uno, la band e gli effetti d'immagine nel successivo. Così il palco si muove alla metà della frequenza dei fotogrammi (30 volte al secondo sulla console), mentre la pista delle note e l'HUD usano la frequenza piena. Attivato, tutta la scena viene disegnata in ogni fotogramma: il palco fluido come la pista, per circa il doppio del lavoro per fotogramma per il processore e la scheda grafica: a 160 FPS anche un PC veloce può scendere a circa 130 nelle canzoni più cariche, mentre a 120 FPS o meno ha margine. Se la frequenza cala, abbassala o disattivalo.",
          "A ogni avvio il gioco mostra il suo video introduttivo mentre carica. Attivato, il video finisce appena il gioco è caricato, come se premessi Start in quel momento, e il gioco inizia subito.",
          "Aggiunge la voce «(debug menu)» al menu principale e Debug Menu al menu di pausa: il menu degli sviluppatori del gioco (Unlock All, Skip Into Song, Change Venue, Select Slomo, Show Song Time, Toggle GPU Time, ...). I suoi testi sono in inglese, alcune voci non fanno nulla senza gli strumenti degli sviluppatori, e Unlock All cambia il tuo salvataggio come giocare.",
          "Attivato, come sulla console, muovere la leva whammy su una nota lunga piega l'intonazione del tuo strumento. Disattivato, la nota mantiene la sua intonazione; la leva fa ancora ondeggiare la nota sulla pista e riempie ancora lo Star Power sulle note Star Power.",
          "In Guitar Hero World Tour lo Star Power faceva passare il tuo strumento in un'eco a tempo con il brano, un riverbero e un filtro passa alto, regolati per ogni strumento e per uno o più giocatori, fino alla fine dello Star Power. Band Hero ha conservato quelle regolazioni ma alza solo il riverbero del locale. Attivato, lo Star Power suona come in World Tour; la band, il pubblico e la voce non cambiano.",
          "Attivato, come sulla console, una nota sbagliata o una pennata senza nota abbassano la traccia del tuo strumento finché non colpisci di nuovo una nota; il volume sotto dice quanto ne resta (0 % la silenzia). Disattivato, la tua traccia non cambia mai.",
          "Con una prospettiva diversa dal 100 % le note impiegano lo stesso tempo a percorrere la pista, quindi arrivano alla linea di battuta più lente sotto il 100 % (circa un quinto in meno al 75 %) e più veloci sopra. Attivato, il tempo sulla pista cambia con la prospettiva perché vi arrivino alla velocità della console: sotto il 100 % ogni nota resta visibile meno a lungo (in Esperto 1,18 s invece di 1,5 al 75 %; una pista più lunga lo restituisce), sopra il 100 % più a lungo. L'Hyperspeed del gioco si aggiunge come sempre. Con due o più giocatori le prospettive proprie delle loro piste rendono le note un po' più veloci che sulla console (4-7 % al 75 %). Disattivato, i tempi della console.",
      },
      .choices = {
          "Dimensione della pista", "La dimensione della pista delle note quando suoni da solo.",
          "Lunghezza della pista", "Le note viste prima, da più lontano.",
          "Prospettiva della pista", "Quanto è inclinata la pista: più piatta o più ripida.",
          "Stile della pista", "L'aspetto della pista dietro le note.",
          "Opacità della pista", "Quanto del locale si vede attraverso la pista.",
          "Volume dello strumento sugli errori", "Quanto resta forte la tua parte quando sbagli una nota.",
      },
      .choice_help = {
          "Rende la pista delle note più piccola o più grande quando suoni da solo, ancorata alla linea di battuta: le note, i bordi, il rock meter e l'indicatore di Star Power con essa. All'80 % si vede più palco intorno. 100 % è la dimensione della console; con due o più giocatori restano le dimensioni del gioco.",
          "Mostra ogni nota altrettanto prima, come in Clone Hero: la pista della console prosegue oltre la sua estremità lontana nella sua stessa prospettiva, quindi diventa un po' più alta e le note in più arrivano da più lontano, mentre ogni nota arriva alla linea di battuta alla stessa velocità. In Esperto una nota appare normalmente 1,5 s prima di suonarla; al 150 %, 2,25 s. Il tempo per colpire le note non cambia. Vale per tutti i giocatori. Con una prospettiva più piatta la parte in più occupa più schermo. 100 % è la pista della console.",
          "La prospettiva della pista, come se la telecamera guardasse da più in alto (sotto il 100 %: l'estremità lontana più larga e le sue note più grandi, si legge più pista) o da più in basso (sopra il 100 %: l'estremità lontana più stretta e più compressa). Le note impiegano lo stesso tempo a percorrere la pista, quindi sotto il 100 % si muovono più lente vicino alla linea di battuta (circa un quinto in meno al 75 %) e sopra il 100 % più veloci: l'interruttore sotto mantiene la velocità della console, oppure l'Hyperspeed del gioco lo compensa. La lunghezza sopra prosegue la stessa prospettiva. 100 % è quella della console.",
          "Scuro è la pista nera della community: l'immagine della pista diventa nera, così le note, le note lunghe e le linee risaltano.",
          "Sotto il 100 % il locale si vede attraverso la pista delle note: le note, le linee e i bordi restano opachi. 100 % è la pista opaca della console.",
          "Quando sbagli una nota o fai una pennata senza nota, il gioco silenzia la traccia del tuo strumento finché non colpisci di nuovo una nota. Qui scegli quanto ne resta: 0 % la silenzia come sulla console, 50 % la dimezza, 100 % la lascia com'era. Chitarra e basso; con due giocatori sulla stessa parte (ne condividono la traccia) cambia solo con 100 %. La batteria non cambia.",
      },
      .choice_values = {{{}, {}, {}, {"Originale", "Scuro"}, {}, {"Muto"}}},
      .done_title = "Il palco è tuo!",
      .done_text = "Il gioco è stato installato. Vai a suonare subito o controlla prima le impostazioni dello schermo, grafiche e "
                   "dei tuoi strumenti; potrai sempre cambiarle nel launcher.",
      .dlc_where = "Il gioco mostra i brani nella scaletta di Partita veloce.",
      .extracting_title = "Accordiamo il palco",
      .extracting_text = "Stiamo estraendo il gioco. Può richiedere qualche minuto; non chiudere il launcher.",
      .keyboard_tap_title = "Plettrata alla pressione di un tasto",
      .keyboard_tap_hint = "Ogni tasto della chitarra plettra quando lo premi; i tasti premuti a meno di 20 ms l'uno dall'altro sono un accordo, una sola plettrata.",
      .part_instrument = "Chitarra",
      .part_vocals = "Voce",
  };
  switch (language) {
    case 3:
      return kGerman;
    case 4:
      return kFrench;
    case 5:
      return kSpanish;
    case 6:
      return kItalian;
    default:
      return kEnglish;
  }
}


namespace {

// The frets' colors (the mockup's) and the chips' text colors.
constexpr uint32_t kGreen = 0x3FAE5F;
constexpr uint32_t kRed = 0xD9443A;
constexpr uint32_t kYellow = 0xE8C547;
constexpr uint32_t kBlue = 0x3E82D6;
constexpr uint32_t kOrange = 0xEB8A35;
constexpr uint32_t kGrey = 0x2A2A30;
constexpr uint32_t kOnFret = 0x0E0E10;
constexpr uint32_t kOnGrey = 0xC9C9D0;
constexpr uint32_t kOnStarPower = 0xC5DC3A;

// One language's words for the guitar's functions.
struct ControlWords {
  const char* frets;
  const char* strum_group;
  const char* menus;
  const char* green;
  const char* red;
  const char* yellow;
  const char* blue;
  const char* orange;
  const char* strum_up;
  const char* strum_down;
  const char* whammy;
  const char* whammy_chip;
  const char* dpad_up;
  const char* dpad_down;
  const char* right_stick;
};

// The game's guitar on the Xbox 360 controller (what the game reads from a guitar, and from a controller or the
// keyboard reported as one, src/startup.cpp's gamepad_subtype): the frets on A, B, Y, X and LB, strumming on the
// D-pad, the whammy bar on the right stick, Star Power on Back. Back is Star Power in the menus too, so it is listed
// once.
std::vector<GameControlGroup> Guitar(const ControlWords& w) {
  return {
      {w.frets,
       {
           {int(Bind::kA), "", kGreen, kOnFret, w.green},
           {int(Bind::kB), "", kRed, kOnFret, w.red},
           {int(Bind::kY), "", kYellow, kOnFret, w.yellow},
           {int(Bind::kX), "", kBlue, kOnFret, w.blue},
           {int(Bind::kLeftShoulder), "", kOrange, kOnFret, w.orange},
       }},
      {w.strum_group,
       {
           {int(Bind::kDpadUp), "\xE2\x86\x91", kGrey, kOnGrey, w.strum_up, w.dpad_up},
           {int(Bind::kDpadDown), "\xE2\x86\x93", kGrey, kOnGrey, w.strum_down, w.dpad_down},
           {int(Bind::kRStickRight), w.whammy_chip, kGrey, kOnGrey, w.whammy, w.right_stick},
           {int(Bind::kBack), "SP", kGrey, kOnStarPower, "Star Power"},
       }},
      {w.menus,
       {
           {int(Bind::kStart), "START", kGrey, kOnGrey, "Start"},
       }},
  };
}

// One language's words for the drums' functions.
struct DrumWords {
  const char* pads;
  const char* kick_group;
  const char* menus;
  const char* red;
  const char* yellow_cymbal;
  const char* yellow_pad;
  const char* blue;
  const char* orange;
  const char* green;
  const char* kick;
  const char* kick_chip;
  const char* dpad_up;
  const char* dpad_down;
};

// The game's drums on the Xbox 360 controller, in the highway's order (GH Metallica's): a Guitar Hero kit's pads on B,
// Y, X and A, its orange cymbal on RB and its kick pedal on LB; a Rock Band kit (four lanes) has no orange. Star Power on
// Back; the menus on the D-pad and Start.
std::vector<GameControlGroup> Drums(const DrumWords& w, bool four) {
  std::vector<GameControl> pads = {
      {int(Bind::kB), "", kRed, kOnFret, w.red},
      {int(Bind::kY), "", kYellow, kOnFret, four ? w.yellow_pad : w.yellow_cymbal},
      {int(Bind::kX), "", kBlue, kOnFret, w.blue},
  };
  if (!four) pads.push_back({int(Bind::kRightShoulder), "", kOrange, kOnFret, w.orange});
  pads.push_back({int(Bind::kA), "", kGreen, kOnFret, w.green});
  return {
      {w.pads, pads},
      {w.kick_group,
       {
           {int(Bind::kLeftShoulder), w.kick_chip, kGrey, kOnGrey, w.kick},
           {int(Bind::kBack), "SP", kGrey, kOnStarPower, "Star Power"},
       }},
      {w.menus,
       {
           {int(Bind::kDpadUp), "\xE2\x86\x91", kGrey, kOnGrey, w.dpad_up, w.dpad_up},
           {int(Bind::kDpadDown), "\xE2\x86\x93", kGrey, kOnGrey, w.dpad_down, w.dpad_down},
           {int(Bind::kStart), "START", kGrey, kOnGrey, "Start"},
       }},
  };
}

}  // namespace

const std::vector<GameControlGroup>& GameDrumControlsFor(uint32_t language, bool four_lanes) {
  static const DrumWords kEnglish{"Drums \xC2\xB7 Pads", "Drums \xC2\xB7 Kick and Star Power", "Menus", "Red pad",
                                  "Yellow cymbal", "Yellow pad", "Blue pad", "Orange cymbal", "Green pad",
                                  "Kick pedal", "KICK", "D-pad up", "D-pad down"};
  static const DrumWords kGerman{"Schlagzeug \xC2\xB7 Pads", "Schlagzeug \xC2\xB7 Bass und Star Power", "Menüs",
                                 "Rotes Pad", "Gelbes Becken", "Gelbes Pad", "Blaues Pad", "Oranges Becken",
                                 "Grünes Pad", "Bassdrum-Pedal", "BASS", "Steuerkreuz hoch", "Steuerkreuz runter"};
  static const DrumWords kFrench{"Batterie \xC2\xB7 Pads", "Batterie \xC2\xB7 Grosse caisse et Star Power", "Menus",
                                 "Pad rouge", "Cymbale jaune", "Pad jaune", "Pad bleu", "Cymbale orange", "Pad vert",
                                 "Pédale de grosse caisse", "GC", "Croix haut", "Croix bas"};
  static const DrumWords kSpanish{"Batería \xC2\xB7 Pads", "Batería \xC2\xB7 Bombo y Star Power", "Menús",
                                  "Pad rojo", "Platillo amarillo", "Pad amarillo", "Pad azul", "Platillo naranja",
                                  "Pad verde", "Pedal del bombo", "BOMBO", "Cruceta arriba", "Cruceta abajo"};
  static const DrumWords kItalian{"Batteria \xC2\xB7 Pad", "Batteria \xC2\xB7 Cassa e Star Power", "Menu",
                                  "Pad rosso", "Piatto giallo", "Pad giallo", "Pad blu", "Piatto arancione",
                                  "Pad verde", "Pedale della cassa", "CASSA", "Croce su", "Croce giù"};
  const DrumWords& words = language == 3   ? kGerman
                           : language == 4 ? kFrench
                           : language == 5 ? kSpanish
                           : language == 6 ? kItalian
                                           : kEnglish;
  // Built once per language and kind of kit.
  static std::vector<GameControlGroup> built[7][2];
  static bool done[7][2] = {};
  const uint32_t index = language < 7 ? language : 1;
  if (!done[index][four_lanes]) {
    built[index][four_lanes] = Drums(words, four_lanes);
    done[index][four_lanes] = true;
  }
  return built[index][four_lanes];
}

const std::vector<GameControlGroup>& GameControlsFor(uint32_t language) {
  static const std::vector<GameControlGroup> kEnglish = Guitar({
      "Guitar \xC2\xB7 Frets", "Guitar \xC2\xB7 Strum and effects", "Menus", "Green fret", "Red fret", "Yellow fret",
      "Blue fret", "Orange fret", "Strum up", "Strum down", "Whammy bar", "WHAM", "D-pad up", "D-pad down",
      "Right stick"});
  static const std::vector<GameControlGroup> kGerman = Guitar({
      "Gitarre \xC2\xB7 Bünde", "Gitarre \xC2\xB7 Anschlag und Effekte", "Menüs", "Grüner Bund", "Roter Bund",
      "Gelber Bund", "Blauer Bund", "Oranger Bund", "Anschlag hoch", "Anschlag runter", "Whammy-Hebel", "WHAM",
      "Steuerkreuz hoch", "Steuerkreuz runter", "Rechter Stick"});
  static const std::vector<GameControlGroup> kFrench = Guitar({
      "Guitare \xC2\xB7 Frettes", "Guitare \xC2\xB7 Grattage et effets", "Menus", "Frette verte", "Frette rouge",
      "Frette jaune", "Frette bleue", "Frette orange", "Grattage haut", "Grattage bas", "Levier de vibrato", "VIBR",
      "Croix haut", "Croix bas", "Stick droit"});
  static const std::vector<GameControlGroup> kSpanish = Guitar({
      "Guitarra \xC2\xB7 Trastes", "Guitarra \xC2\xB7 Rasgueo y efectos", "Menús", "Traste verde", "Traste rojo",
      "Traste amarillo", "Traste azul", "Traste naranja", "Rasgueo arriba", "Rasgueo abajo", "Palanca de trémolo",
      "TRÉM", "Cruceta arriba", "Cruceta abajo", "Stick derecho"});
  static const std::vector<GameControlGroup> kItalian = Guitar({
      "Chitarra \xC2\xB7 Tasti", "Chitarra \xC2\xB7 Plettro ed effetti", "Menu", "Tasto verde", "Tasto rosso",
      "Tasto giallo", "Tasto blu", "Tasto arancione", "Plettrata su", "Plettrata giù", "Leva del vibrato", "VIBR",
      "Croce su", "Croce giù", "Levetta destra"});
  switch (language) {
    case 3:
      return kGerman;
    case 4:
      return kFrench;
    case 5:
      return kSpanish;
    case 6:
      return kItalian;
    default:
      return kEnglish;
  }
}

}  // namespace launcher::game
