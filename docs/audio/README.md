# Audio

All sounds are RIFF WAV. Sound effects: **Xbox ADPCM** (format tag `0x69`,
mono 22050 Hz, 36-byte blocks per channel = 4-byte header [int16 sample,
u8 step index, u8 0] + 32 bytes of IMA nibbles, low nibble first. A block is
**64 samples: the header sample + the first 63 nibbles** (the 64th nibble is
padding) — verified: blocks join seamlessly and totals match the `fact` chunk;
skipping the header sample (a common mistake, and my first version) puts a step
at every block = audible 345 Hz buzz. Stereo interleaves 4-byte groups per channel.

**Loops** wrap on whole 64-sample blocks: keep the full decode for looping sounds
(trimming to `fact` made the wrap click, up to ~3× a normal step) and use `fact`
only for one-shots. 150 files carry an `smpl` chunk; nearly all loop the whole file.
Some shipped loops still aren't seamless (Silver's ocean, Fuusoku's city), so the
argon crossfades the last ~12 ms into the start and loops from there — every loop
then wraps within the sound's own sample-to-sample range.
Music: PCM16 stereo 44.1 kHz (front-end music: stereo ADPCM). Decoder:
`src\audio\wav.cpp` (`argon --decode in.wav out.wav` writes PCM and prints the format, `playFrames` and the `smpl` loop).

**Output** (`audio_output.cpp`): the mix runs at the Xbox's 48 kHz, which the HRTF and crosstalk filters are designed
for, except on a 44.1 kHz device, where it runs at 44.1 kHz so the songs play unresampled instead of going
44.1 -> 48 -> 44.1 kHz (the HRTF delays are scaled to the rate; the filters' taps are not). The songs stay 44.1 kHz in
their feeds and the music's one sound resamples them to the mix, so a junction between songs stays seamless. The final mix is limited (`peak_limiter.cpp`) and then dithered at 16-bit
resolution (`output_dither.cpp`): Windows rounds the float mix for a 16-bit device without dither, which left a faint
crackle in quiet music at high volume.

**Converted files** (`python tools\assets\convert_assets.py`, next to the originals, which stay as they are; argon loads
`<name>.flac` / `<name>.ogg` in place of a game `.wav` when it is there: `audio.cpp` `loadGameSound`. The folders are
still listed by their `.wav` names, also once the `.wav` is deleted (`game_sound_files.hpp` `listGameSounds`: one
`<name>.wav` per name with a `.wav`, `.flac` or `.ogg`, sorted), so the order and the `_L` / `_R` / `.L` pairs, the
music's segments and the engines' `.ini` layer names don't change):
- Music, 112 files: `<name>.flac`, lossless. Only the `data` chunk's samples: the 106 PCM files end in a LIST chunk
  (Sound Forge's INFO tags, 88 bytes) and 14 of them also in Sound Forge / Vegas blocks (`bext`, `minf`, `elmo`,
  `regn`, `ovwf`, `umid`); two (flames_music4, flames on_start) have an `smpl` chunk with no loop. None of it is sound
  or loop data the game uses. The 6 ADPCM files (frontend_music1..5, levelload_music1) are decoded by argon and cut
  to their `fact` length (what the music plays: `Pcm::playFrames`). Each FLAC decodes bit for bit to the source's samples.
- Sound effects, `audio\**`, 617 files, all ADPCM mono 22050 Hz: `<name>.ogg`, Vorbis at 256 kbit/s, but libvorbis
  takes at most 90 kbit/s for mono 22050 Hz, so they are at 90 kbit/s (the script finds the highest for each format
  and says so). Decoded by argon with every block whole, the padded last block included, as `loadWav` keeps them
  (loops wrap on whole blocks, see above), so the Ogg plays exactly like the `.wav`: the `fact` count is the Vorbis
  comment `PLAYLENGTH`, read into `Pcm::playFrames` where one-shots stop (`song_files.cpp` `playFramesFromComments`),
  and the `smpl` loop (150 files) the comments `LOOPSTART` (first frame) and `LOOPLENGTH` (frames, `end + 1 - start`),
  read into `Pcm::sampleLoopBegin / End` (`sampleLoopFromComments`). The script checks each Ogg on argon's own
  decode: the same length, play length and loop as the `.wav` (`argon --decode` prints them).
- Not converted: the story scenes' speech, `data\movies\speech` (224 ADPCM files, read from the `.wav`).
- Deleting the originals: `python tools\assets\delete_converted_originals.py [--delete] [--ffmpeg <ffmpeg>] [--argon <argon.exe>]
  [--game <folder>]` checks each original the converter handles (the `.bik` videos too) and reports it as verified (the
  converted file is there, not older, and passes the converter's check again: the FLAC's samples, the Ogg's lengths
  and loop on argon's decode, the `.ogv`'s soundtrack and frame count), not converted, out of date or failing.
  Without `--delete` it only reports, with the bytes it would free (1020 MB for the 737 originals: 8 videos, 112 music
  files, 617 sound effects); with it, only the verified originals are deleted. The speech, the saves and the converted
  files are never touched; `convert_assets.py` then converts only the originals left (none: nothing to do).

| file | what |
|---|---|
| [mix-levels.md](mix-levels.md) | mix levels |
| [music.md](music.md) | music |
| [race-sounds.md](race-sounds.md) | race sounds, reverb and filters |
| [ships.md](ships.md) | ships |

## Tracks — `audio\tracks\<t>\` (alt_ tracks use the normal track's folder)
`<t>.txt` lists the track's sounds:
```
SF_BRACKEN_CROWD1	e_crowd1        <- sound id, sub-folder
TRIGGERSTART
TRIGGER
X	-51.599998                     <- game coordinates
Y	13.510000
Z	1.650000
MINDIST 180.000000
MAXDIST 230.000000
TRIGGER ...                        <- more points for the same sound
TRIGGEREND
```
Sounds with no `TRIGGER` are played by code (countdown `three/two/one/go`,
`tunnel`; `thunder` has a folder in bracken but bracken.txt never lists it, so the game
never loads it: [effects/rain.md](../effects/rain.md) "Rain"). The rain loops are generic sounds 3 / 4
(`rain_windshield` / `rain_ground_1`, [effects/rain.md](../effects/rain.md) "Rain"). Each sub-folder has a `.txt` with options: `LOOPS 1`,
`TYPE ENVIRONMENT`, `NOPOSITION`, `NOREVERB`, `NOMAIN`, channel files `LEFT`/
`RIGHT`/`LS`/`RS` (= `*_L.wav`, `*_R.wav`, ...), `MINVOL`/`MAXVOL`, `ROUTETO <ch>`
(thunder only), `GROUP`/`ENDGROUP`.
The parser (0xc6ea6..) reads `LOOPS`, `MINVOL` and `MAXVOL`, the sound record's shorts +0x98 / +0x9a / +0x9c, with
sscanf("%d") (0xdd940) into an int, so each also writes its value's high half over the next short: `LOOPS 1` leaves
MINVOL 0 and a negative `MINVOL` leaves MAXVOL -1 (read live from xemu's records, 2026-11-20: every LOOPS 1 folder at
0 / 0, impact_l_metal2 -1000 / -1, ui_rollover3 LOOPS 0 MINVOL -9600 MAXVOL 0 at -9600 / 0). The scrapes (scrape_m /
scrape_w, LOOPS 1) thus play at full whatever the contact's force: on Silver's loop the craft brushes the track every
few frames (normal . direction 0.11..0.15, velocity change 15..27 against the light force 20), and each brush starts
SCRAPEMETAL. The frame without a contact stops it through 0xc4ba0, which calls 0xcafd0 with 0: the voice is released
(id -2, the old one at +0x18) and IDirectSoundBuffer_Play(0, 0, 0) clears its looping, so the scrape plays on to its
end (1.2 s) while the next brush starts another; the stacked scrapes are the loop's grinding (xemu, 2026-11-20: 11
released scrape voices after one loop). 0xcafd0 with 1 (StopEx with the envelope) is not what 0xc4ba0 does; every
0xc4ba0 stop (scrapes, rain 3 / 4, INCOMING) plays out so. Argon read MINVOL -10000 there (about -90 dB) and cut
a stopped sound at once, so the loop was silent (sound_rules scanLoopsInto / scanMinVolumeInto, RaceSfx::stop ->
Voice::release).
`cut_ambient\Amb_N.{L,R,C,Ls,Rs,LFE}.wav` = the story scenes' 5.1 ambience ("cut" = cutscene): only the scene
page's enter (0x9a810) and the tournament-win scene (0x7fc56) load it (0xca300) and play it (0xca540, its only two
callers, no pointer to it), at the SFX option less 5 dB on the six speaker voices; the scene's leave stops it
(0xca5d0). The race never plays it: Argon did, under every race, until 2026-11-21 (now Audio::playSceneAmbience from
StoryScene::start, mix::kSceneAmbienceMb).

Argon treats a sound's TRIGGER points as zones for **one** looping voice:
the loudest point (full volume inside MINDIST, fading to silence at MAXDIST — my
reading of the two values) sets gain and pan, smoothed over ~80 ms. Playing one
copy per point stacked identical loops wherever zones overlap (180 m radii overlap
a lot), which comb-filtered and overloaded into a distorted sound.

## Front end — `audio\frontend\`
- `frontend.txt` lists the UI sounds by id (FUN_000c3ca0(id)): 0 SF_UI_SELECT1 ui_button1p, 1 SF_UI_BADSELECT1
  ui_rollover3, 2 SF_UI_ROLLOVER1 ui_rollover1, 3 SF_UI_SCREENFORWARD ui_xition1_LR, 4 SF_UI_SCREENBACK ui_xition2, 5
  SF_UI_GOTOGAME ui_xition3_LR, 6 SF_UI_BACKGROUND3 ui_amb3 (loop, 4 files L / R / LS / RS), 7 SF_UI_ENGINETEST
  enginetest. The pages call 0 on a pick, 1 on a refused pick, 2 on a cursor move, 3 / 4 leaving forward / back
  (some on the press, some when the next page opens); each page's calls: [frontend/README.md](../frontend/README.md) "UI sounds". The race's
  generic list (`audio\generic\generic.txt`) has its own menu sounds (SF_GENERIC_MENUNAVIGATE / BADMENUSELECT /
  MENUSELECT, the same files) used by the pause menu (ids 0x17 / 0x18: up / down, a volume step that moved; A on any
  item but a volume row; [frontend/race-end.md](../frontend/race-end.md) "Pause menu").
- Music: `media\music\frontend_music\` on the menus (the game's own soundtrack entry 0 "Frontend Music",
  0x1a8450), `levelload_music\` with the `ui_levelload` loop on the loading screen. That loop (0xca610: ui_load_l / r /
  ls / rs on four 2D voices +0x964, looping) plays at the **Music Volume** (0xdee90), not the SFX one (Argon gave it
  the SFX volume until 2026-11-21); 0xca610 is called after a story scene (its leave 0x99fb0) and on the race's way
  out (0x7fb7b, 0x8019c), 0xca9b0 stops it. Not traced: whether the front end -> race load plays it (Argon does).
- Argon (Audio::loadUi / playUi / setFrontEnd / updateFrontEnd): the UI sounds at the SFX volume, the front-end
  music at the music volume (looping the folder in order), the loops (ui_amb3, UI sound 6, through sound_play_ui: the SFX
  option on top, 0xc7990) at the SFX volume x the editor's ambience slider, as the race's ambience (until
  2026-11-21 the SFX volume was left out: 4 dB too loud at the default step, more below it). The menus' ambience
  stands in for the music when it is off: 0x32550, on each page's enter (fe_shell_set_state 1, 0x31736), stops every
  voice of sound id 8 with its fade (0xcafd0 1), then finds Music Volume (type 0xd) in the settings list (0x1baf64,
  +0x25c next) and starts id 8 again only if its choice (+0x24f) is 0 and the page is not FE TITLE / FE INTROSCREENS.
  At the default music step the menus play the music alone (xemu's Audio Debug on the press-START screen,
  2026-11-21: voice 0x44, the stereo stream on bins 0 / 1 at -5 dB = the option's -400 mB + its 100 mB headroom,
  which DirectSound takes off the volume, dsapi.cpp m_lVolume = lVolume - m_dwHeadroom; 0x40..0x43 are
  DirectSound's own 32-sample loops on every bin). Argon: FeShell::show -> Audio::enterFrontEndPage (stops it at
  once, no fade; until 2026-11-21 it played on every menu page); each front-end page
  plays its own table of UI sounds by input event (ui\fe_sounds.hpp); the loading screen's A plays GOTOGAME. The
  pause holds only the race's voices (the pause menu's 0x17 / 0x18 start through RaceSfx::playOnListener after the hold, so they play).

## Pickups (models, not audio)
`data\objects\powerchip`, `megaup`, `powerup_red|blue|yellow` (+ `_ns`, `_ee`
variants), placed by `config\<t>pup.txt` (`POWERUPRED` -> `powerup_red`).
Drawn at native size. The loader (`pickups_load` 0x00078380) raises each pickup's Y
by a per-type amount: blue 3.84, red 3.08, yellow 3.93, mega-up 5.97, power chip 5.74;
the draw (`pickups_draw` 0x0008fbc0) adds a bob of 0.5 + 0.5·sin(2·time + index).
