# Saves: copying, editing and converting

## Copying saves in and out of xemu
The tools work on save folders; they never open the xemu HDD image. Copy the folder yourself:
- **FATXplorer** on a copy of the xemu HDD image (or the image with xemu closed): `Partition 1 (E:)\UDATA\4d530015\`,
  copy a `<ID>` folder out, or one in.
- **FTP** from a dashboard that runs in xemu with networking (for example an FTP server dashboard): the same path,
  `E:\UDATA\4d530015\`.

Copy whole folders (SaveMeta.xbx and savegame.sav). A folder written by the tools is named by the game's hash, so it
can go straight into `4d530015\`. Keep a backup of the original folder.

Every write keeps SaveMeta.xbx in step with the profile: `from-json`, `new`, `resign`, `win-all` and the editor all
write it with `Name=Profile: <the profile's name>` and read it back (a missing or stale one in a save folder is
written again; a savegame.sav given on its own, outside a save folder, is written alone). Writing into a folder whose
SaveMeta names another profile is refused. The game opens a profile by the folder it hashes from the save name, so
`info` and every write warn when the folder or SaveMeta do not match it. A profile renamed by hand in SaveMeta.xbx
alone is not found: the name inside savegame.sav, SaveMeta's Name and the folder must all change together (the
editor's rename does that).

## tools\saves\profiles.py (command line)
Python 3, standard library only. Global options before the command: `--signature-key <file>` (the game's signing key,
from tools\saves\signature_key.py), or `--cert-key <file>` (the XboxCERTKey file) with `--xbe <file>` (default
`game_data\default.xbe` of this checkout). The game's key is
used when both are given; with neither, keys.json (`--keys <file>`: another one).

| Command | What it does |
|---|---|
| `info <save>` | name, save folder ID, percent complete, unlocks, eggs / cheats, settings, playlist, per-character cash / upgrades / trophies / progress, signature status; warns when SaveMeta or the folder name do not match |
| `verify <save>` | signature status; exit code 0 valid, 1 invalid, 2 not checked (no key) |
| `resign <save>` | signs savegame.sav again and rewrites SaveMeta.xbx, in place (needs the key) |
| `win-all <save>` | every tournament won, in place and signed (`--unsigned` without a key): each character's five trophies and what those wins open in profile_tournament_won 0x84160: all 16 characters, both routes of all 8 courses, Mars, the Redshift class, all 8 easter eggs; no tournament left running. Cash, upgrades and the Redshift Quick Race wins stay, so it reads 96 % |
| `to-json <save> <out.json>` | argon profile JSON |
| `from-json <profile.json> <out>` | savegame.sav + SaveMeta.xbx in `<out>\<ID>\` (`--exact-folder`: in `<out>` itself) |
| `new <name> <out>` | a fresh profile as the game's reset makes it (a name starting "CHEAT" opens the cheat menu), same output options |

`<save>` is a save folder or its savegame.sav. Without any key (no key file, none in keys.json), `info` / `verify` / `to-json` work and report
"signature not checked"; `from-json` and `new` refuse to write unless `--unsigned` is given, and then warn (the new file
has a zero signature, which the game rejects); `resign` needs the key.

```
python tools\saves\profiles.py info <saves>\11C90F515484
python tools\saves\profiles.py to-json <saves>\11C90F515484 save_data\profiles\PLAYER1.json
python tools\saves\profiles.py from-json save_data\profiles\PLAYER1.json <saves>
python tools\saves\profiles.py --signature-key <game key file> verify <saves>\11C90F515484
```

## tools\saves\profile_editor.py (editor)
`python tools\saves\profile_editor.py [save folder or .json]`. Opens a save folder or argon JSON; tabs for the profile
(name, open speed class, last tournament level, music source, Redshift wins), the unlocks (characters, tracks, Mars),
the characters (cash, upgrade levels, trophies; "All tournaments won" does what `win-all` does, on top of the
edits so far, until you save), the easter eggs and cheats (unlocked / on, the cheat menu) and the
settings; the percent complete follows the edits. Save as a save folder (signed with a key file, else keys.json's
keys; with neither, after a warning, the file keeps its old signature) or as argon JSON. "keys.json..." picks the
keys.json to use (default: the repo's) and forgets the key files chosen before; "Game key file..." (from
signature_key.py), "XboxCERTKey file..." and "default.xbe..." pick single files; a .json picked there is used as a
keys.json. All remember paths only (never a key) in `%APPDATA%\qr_profile\settings.json`; the status line names the
key in use (the file, and keys.json's entry). Only the fields you change are written back, so an unedited save is
saved byte for byte. "Save as save folder..." writes SaveMeta.xbx with the profile's save name into the folder named
by its hash: choosing the profile's own folder after renaming it saves into the new name's folder beside it, then asks
whether to remove the old folder (its two save files; the folder too when nothing else is in it).

## Converting
- The JSON has no field for the camera, the game speed, the hint counters, +0xc80 / +0xc84, the exact lap times and
  the console's playlist slots. `to-json` keeps them in an `"original"` object at the end (and the song names in
  `original.songs`); argon's loader looks keys up by name and ignores it, but argon drops it when it saves
  the profile. `from-json` uses it when present, else the reset values (camera 1, game speed 0, hints 0, playlist
  32 empty slots).
- `speeds` goes to +0x170 and +0xc90. `cheatsOn` bits 5 / 6 go to the activated bytes and to +0xc7c / +0xc7d (from
  the game: either sets the bit). An empty `tourney` (a new argon profile) becomes -1 (no tournament running).
- `playlist` ("soundtrack\song") cannot be turned into console soundtrack / song indices: without kept slots the
  save gets 32 empty slots and Music Source Default (the game and argon both fall back to Default with no songs).
- `bestLaps` keeps 1 ms; the exact f32 values come back from `original.bestLapsExact` where they still print as the
  JSON's.
- `options` maps by setting type; a value outside an entry's choices gets the default, as in argon.

What round-trips exactly:
- savegame.sav read and written unedited: byte for byte (name padding, struct padding and stale playlist names kept).
- save -> JSON -> save: byte for byte except the name padding and the struct padding (zeroed), flag bytes other than
  0 / 1 (written as 1), a +0xc90 that differs from +0x170, Exit / Reset / Music Options bytes that are not 0, Music
  Source Custom without any playlist slot, and the signature (signed again).
- argon JSON -> save -> JSON: the same values; `playlist` comes back empty, `musicCustom` false without songs, an
  empty `options` / `bestLaps` as the full strings argon reads them as.
