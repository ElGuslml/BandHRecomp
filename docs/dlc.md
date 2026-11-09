# Downloadable content (DLC)

**Status: not tested (2026-11-06).** GH5's page, unchanged. Band Hero's code for it is GH5's, where a package made for
that game installs and its songs show, but no package has been tried in this game yet.

Band Hero Launcher's DLC page installs Xbox 360 content packages (STFS: LIVE, PIRS or CON) with the game itself:
`BandHeroRecomp.exe --install_content=<package>` extracts one into

    userdata\0000000000000000\4156085C\00000002\<package name>\

## Turning it off

Remove the package from the DLC page, or delete its folder. `kHasDlcPage` in `launcher/game.h` hides the page.
