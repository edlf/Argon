# Original saves: the game's profiles on the Xbox HDD

How Quantum Redshift (USA XBE) stores a profile, and `tools\saves\profiles.py` / `tools\saves\profile_editor.py`, which read,
edit, sign and convert them to and from argon's JSON profiles (`save_data\profiles\*.json`, [frontend/README.md](../frontend/README.md)
"Profiles"). Decoded from the disassembly only (profile_write 0xbfac0 / 0xbf090, the load 0xbfa40 / 0xbf1a0 / 0xbf8c0,
XAPI 0xedb00..0xee700). **Nothing here has been checked against a real save from a console yet.**

| file | what |
|---|---|
| [tools.md](tools.md) | copying, editing and converting |

## Folders and files
- Title ID 0x4D530015. XAPI maps `U:` to `E:\UDATA\4d530015\` (lower-case hex).
- A profile is one save folder `E:\UDATA\4d530015\<ID>\` holding `SaveMeta.xbx` and `savegame.sav` (no save image: the
  dashboard shows the title's).
- The save's name is not the profile's: FE NAME ENTRY's Done (0x3a5b1) builds it as
  `swprintf("%s %s", infotext 238, the profile name)` into 0x459a90 (the save list record +8), so `Profile: PLAYER1`
  in English. Infotext 238 is the dashboard language's: English `Profile:`, Japanese `プレイヤー:`, the other languages
  an untranslated test string; a profile saved in one language is looked for under another name in another. The
  tools use the English one. savegame.sav keeps the bare profile name.
- `<ID>` (XCreateSaveGame 0xedaa6) = 12 upper-case hex digits hashed from the save name's UTF-16 code units:
  `h = (h * 0x10000 + c) % 0xFFFFFFFFFFC5`, `"%012X" % h`. `Profile: PLAYER1` -> `B1CD4EB5DDBF` and `Profile: ET` ->
  `FF5B38DCE470` (both real Xbox saves), `Fastest Lap Data` -> `9618834AAC57`, `_SAVE` -> `0041163B1366`. If the
  folder exists, its SaveMeta Name must be the save name.
- `SaveMeta.xbx`: UTF-16LE with no terminator: U+FEFF, `Name=<save name>\r\n`, e.g. `Name=Profile: ET\r\n`
  (`NoCopy=1\r\n` follows for the lap records' save, not for profiles).
- The other saves: `Fastest Lap Data` (laptimes.lap, 0x884 bytes: 90 x (f32 time, wchar[10] name) + the signature,
  NoCopy) and the ghosts (`Ghost: <character>-<track>`, ghost.gst). The tool handles only profiles.

## savegame.sav (exactly 0x16E9 = 5865 bytes)

| File offset | Size | Content |
|---|---|---|
| 0x0000 | 0x100 | Profile name, wchar[128], NUL-terminated (at most 9 characters). The bytes after the NUL are uninitialised stack data: the tool keeps them on an unedited save and zeroes them on a rename |
| 0x0100 | 0xCC4 | Raw copy of g_profile 0x1baf74..0x1bbc37 (below) |
| 0x0DC4 | 7 | Sound Settings, one byte per entry (the chosen option), in list order: Exit, Reset to Default, Music Options, Engine Volume, SFX Volume, Music Volume, Headphones |
| 0x0DCB | 10 | Game Settings: Exit, Reset, Controller 4 / 3 / 2 / 1 Vibration, Speed Units, Brightness, HUD, Multiplayer Laps |
| 0x0DD5 | 32 x 0x48 | Playlist slots: u32 song index, u32 soundtrack index, wchar[32] song name; empty slot = -1, -1 (stale name bytes) |
| 0x16D5 | 0x14 | Signature: HMAC-SHA1 over bytes 0x0000..0x16D4 |

There is no magic, version or size field. The load reads every block in full and then needs end of file right after
the signature, so the size must be exact. A settings byte above the entry's option count gets the default on load.

### The profile block (file offset = 0x100 + profile offset)

| Profile off | Type | Meaning | Reset (0x83b60) | Argon JSON |
|---|---|---|---|---|
| +0x000 | f32[90] | The profile's best laps, index track x 5 + speed; 999999.0 = none | 999999.0 | `bestLaps` (90 x `"%.3f,"`) |
| +0x168 | i32 | Race camera view | 1 | none (`original.camera`) |
| +0x16c | u16 | Unlocked characters, bit per character | 0x007f | `characters` |
| +0x170 | i32 | Highest open speed class 0..4 | 1 | `speeds` |
| +0x174 + c x 0xb0 | record[16] | Per character (below) | | |
| +0xc74 | i32 | Game speed last chosen | 0 | none (`original.gameSpeed`) |
| +0xc78 | i32 | Tournament level last started | 0 | `tourneyLevel` |
| +0xc7c / +0xc7d | u8 / u8 | Infinite Shield / Infinite Turbo switched on | 0 | `cheatsOn` bits 5 / 6 |
| +0xc80 / +0xc84 | i32 / i32 | unused (cleared by the reset) | 0 | none (`original.unusedC80/C84`) |
| +0xc88 | u32 | Unlocked tracks: bit i track i, bit i + 8 its second route | 0x7f | `tracks` |
| +0xc8c | u8 | Mars unlocked | 0 | `mars` |
| +0xc90 | i32 | Highest open tournament level (always set with +0x170) | 1 | `speeds` |
| +0xc94 | u8[8] | Easter egg i unlocked | 0 | `eggs` (bit i) |
| +0xc9c | u8[8] | Easter egg i switched on | 0 | `eggsOn` |
| +0xca4 | u8[7] | Cheat i unlocked by its code | 0 | `cheats` |
| +0xcab | u8[7] | Cheat i activated | 0 | `cheatsOn` |
| +0xcb2 | u8 | Cheat menu shown (the name started "CHEAT") | 0 | `cheatMenu` |
| +0xcb3 | u8 | Music Source: 1 Default, 0 Custom | 1 | `musicCustom` = !value |
| +0xcb4 / +0xcb8 / +0xcbc | i32 x 3 | Loading hint index: Time Attack, Quick Race, Tournament | 0 | none (`original.hint*`) |
| +0xcc0 / +0xcc1 / +0xcc2 | u8 x 3 | Redshift win on Mars (Martian route), Mars (Terraformed route), with easter egg 0 on | 0 | `redshiftWins` bits 0 / 1 / 2 |

Padding: +0x16e (2 bytes), +0xc7e (2), +0xc8d (3), +0xcc3 (1).

Per character c (0xb0 bytes at profile +0x174 + c x 0xb0):

| Record off | Type | Meaning | Reset | Argon JSON |
|---|---|---|---|---|
| +0x00 | i32 | Cash | 0 | `cash[c]` |
| +0x04 | u8[3] | Upgrade levels 0..7: Weapon Strength, Turbo, Shield | 0 | `upgrades[c*3 + k]` |
| +0x07 | u8[5] | Trophy held per tournament level (Novice, Amateur, Expert, Master, Redshift) | 0 | `trophies` bit c*5 + l (24 hex digits, the high word first) |
| +0x0c | i32[5] | Tournament progress per level (races won); -1 = no tournament running | -1 | `tourney[c*5 + l]` |
| +0x20 | i32[18] | Tournament high score per track | 0 | `hiTourney[c*18 + t]` |
| +0x68 | i32[18] | Quick Race high score per track | 0 | `hiQuick[c*18 + t]` |

Characters 0..15: Tara, Yuri, Kumiko, Darius, Jordan, Marcus, Leah, Ka Hekili, Alicia, Vladimir, Abi, Zafir, Kaitlyn,
Jaya, Lukas, Kutoo. Tracks 0..17: Silver, Kalau, Fuusoku, Flames, Solar, Bracken, Mount, Draco, their second routes
(8..15), Mars (16), Mars Terraformed (17).

Percentage Complete (0xc03a0) is not stored: per character the highest trophy level held + 1, +1 per unlocked
character 8..15, +1 per unlocked egg, +1 / +1 / +2 for the three Redshift wins (100 in all).

## The signature
Every profile save uses `XCalculateSignatureBegin(0)`: a roamable signature, the same on every console:

```
signing key = HMAC-SHA1(key = XboxCERTKey, message = the XBE certificate's signature key (16 bytes at +0xC0))[:16]
signature   = HMAC-SHA1(key = signing key, message = savegame.sav[0 : 0x16D5])
```

The certificate signature key is read from the game's `default.xbe`. **The XboxCERTKey is a kernel system key, which
you supply**: nothing in this repository holds it or tells you where to get it. Put it in a text file outside the
repository as 32 hex digits (whitespace is allowed) and pass the file's path. The tools never print, log or copy the
key. A wrong signature makes the game list the profile as "Invalid profile", and loading it resets the profile.

The kernel works the signing key out itself when it starts the game and keeps it as its export `XboxSignatureKey`
(ordinal 325). The game imports it: its kernel thunk entry (0x18EDDC; the thunk table 0x18ECC0 is the header's +0x158
XORed with the retail key 0x5B6D40B6) points at the 16 bytes once the game runs. So the signing key can be read from
a running xemu instead, and no XboxCERTKey is needed:

```
python -I tools\saves\signature_key.py --check <saves>\11C90F515484
```

With xemu running the game (tools\xemu\run_xemu.ps1, any page past the boot), it checks that the running title is 4d530015,
reads the key through the thunk entry over the GDB stub and never prints it. With no path it sets `XboxSignatureKey`
in keys.json (below); given a key file path it writes that instead (32 hex digits; refused inside the repository, and
an existing file without `--force`). `--check` verifies a save with the key: "valid" on a save copied out untouched
confirms the key and the signing scheme. The game's key is used as it is, without default.xbe.

### keys.json
`keys.json` in the repository root holds the keys by name, each 32 hex digits (an empty string = not set). It is in
`.gitignore`: never commit it.

```
{
  "XbeSignature": "<the XBE certificate's signature key>",
  "XboxCERTKey": "<the XboxCERTKey>",
  "XboxSignatureKey": "<the game's signing key, from signature_key.py>"
}
```

The profile tools use it whenever no key file is given (`--keys <file>` points them at another one). The key, first
found: `--signature-key`, `--cert-key`, keys.json's `XboxSignatureKey`, then keys.json's `XboxCERTKey` (with `--xbe`'s
certificate key if given, else `XbeSignature`, else default.xbe's).
