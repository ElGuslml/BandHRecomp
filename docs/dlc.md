# Downloadable content (DLC)

**Status: the packages are read, their songs not tried yet (2026-11-10).** The game reads the player's DLC folder in
place; with a collection of 191 packages it takes 65 (Guitar Hero 5's and Death Magnetic) and resolves them all, but no
song of them has been played in this game yet.

## Your DLC folder

Band Hero Launcher's DLC page takes the folder where the player keeps their packages (STFS files: LIVE, PIRS or CON, in
it and its subfolders), as many games' as they like; the other ElGus Recomps launchers can use the same folder. The
launcher reads each package's header and the names at its root, checks it against this game's rule (below), and writes
the game's list, `userdata\0000000000000000\4156085C\content_packages.txt`, with the packages on. The game (the SDK's
change 80) lists those packages as its downloadable content and reads them from their files: nothing is copied,
written or deleted. The page groups the packages by the game they were made for, each with a switch (a group's switch
turns all of it); those the game cannot resolve come turned off. The folder and the player's choices are in
`dlc_library.txt` beside the list; the folder is read again each time the launcher opens (`kDlcMode = kFolder` in
`launcher/game.h`, launcher 3.17).

Packages installed by an earlier launcher (extracted into `userdata\0000000000000000\4156085C\00000002\<package>\` by
`BandHeroRecomp.exe --install_content=<package>`) keep working and show as "Installed copies". One that is also in the
DLC folder (the same file name, or the same name and size) shows once, as the copy the game reads; "Remove copies"
deletes those copies after a confirmation, and the folder's packages take their place.

## Which packages the game reads

Guitar Hero 5's rule (GH5Recomp's `docs/dlc.md`). The game's DLC script (`5A4B6A0C`) is GH5's, with one more call at
its end (`boot_download_scan`): it knows a package by the QBKey of its display name (lower case, any other character
than a letter or digit an underscore, its first 42 characters) and reads it if the package carries
`bmanifest_<key>.pak.xen` (Guitar Hero 5's packs do), if another package's manifest names it, or if the content index
on its disc names it (`qb.pab.xen`, key `0x65735DEB`: one entry, Death Magnetic, the same as Guitar Hero 5's). A
folder it cannot resolve makes it report a damaged package and remove every content file, all DLC off, as Guitar Hero
5 does. `kDlcRule` in `launcher/game.h` checks the packages the same way, and those that do not resolve come turned
off.

Checked with a collection of 191 packages on a hard disk: the launcher takes 65; the game lists them, mounts 16 for
their manifests, finds every manifest it asks for and logs no error in 60 s at the title screen.

## Turning it off

Turn the package (or its game's group) off on the DLC page. Without a DLC folder chosen, the game's list is not
written and the game reads only what is extracted into `00000002`. `kHasDlcPage` in `launcher/game.h` hides the page;
`kDlcMode = kInstall` brings back the page that extracts the packages.
