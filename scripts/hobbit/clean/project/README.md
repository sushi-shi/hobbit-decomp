# Hobbit reconstructed source subset

Generated from the evidence-backed source manifest. This is an incomplete
reconstruction, not a playable game. The export strips address annotations;
source behavior and the original compiler flag profiles are preserved.

Run `nix develop -c python3 build.py --exe /path/to/Meridian.exe` to compile every exported unit and the
resource script with the configured VC6 toolchain. This never launches the game.
`--jobs N` controls compiler concurrency. Objects are cached against sources,
headers, compiler settings, and build-script inputs.

`--link` requires a complete source reconstruction and an explicit `link`
object in `build.json`, containing `output`, `flags`, and `libraries`.
No full link profile has been admitted yet. The export contains no reference
executable, extracted code objects, donor game imports, runtime DLLs, or launcher.
Game resources retain their original ownership.

Supply your own matching executable; `config/retail/targets.json` records its
full SHA-256 and size. `HOBBIT_EXE` can also select this local file. Icon and
default bitmap bytes are extracted only after verifying this identity, into
`build/`. They are not included in this export and no acquisition is automated.
