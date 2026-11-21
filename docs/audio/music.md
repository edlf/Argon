# Audio: music

## Music — `media\music\<t>_music\`
`intro.wav`, `on_start.wav`, `on_start_second_lap.wav` (alt_kalau only) and the segments `<t>_musicN.wav`: bar-aligned
parts of one piece (PCM16 stereo 44.1 kHz, lengths in whole multiples: alt_bracken_music1 14.22 s, 2..4 and on_start
28.44 s), so they must follow each other without a gap. Front end: `frontend_music1..5` (no intro / on_start).

**Music modes** (the music object 0x4406f0; one object for the race and the front end). Its mode +0x6ac picks the
file (0xdf350): 0 intro, 1 on_start, 2 on_start_second_lap, 3..7 segments 1..5, 8 `%s%d` with the folder's segment
count. A per-folder table (0xded40, entry = track id + 1, entry 0 the front end; 19 entries) holds whether intro /
on_start / the second-lap stinger exist and the segment count (4, 5 or 6) — it matches the folders' files exactly,
so argon reads it from them. Mars and Terraformed Mars have no folder: the folder names (0x1a8454, same index)
give both `mount_music`, and their 0xded40 entries keep the default 5 segments, so `mount_music6` plays only on Mount
(argon: track\track_source.cpp musicFolderOf, limitMarsMusic). With n segments, modes 3..7 stop at segment n - 1 (n = 4: 6 and 7 -> 5; n = 5:
7 -> 6), so the last segment plays only in mode 8.
- `set` 0xdec40(mode): 0 -> intro (else on_start), return mode +0x6b4 = 3; 1 -> on_start (else mode 3), return 3;
  2 -> only if the folder has the stinger: return = the current mode, mode 2; 8 -> 8; any other -> mode and return.
- File end (the stream's packet fill 0xdfad0, race / front-end music, not the custom soundtrack): when the mode is
  the one whose file started last (+0x6b0) and is 3 or more, the mode steps up (0xded20: 3, 4, 5, 6, then 3) and the
  same file plays again (the XMO flushed); otherwise the mode's file opens (+0x6b0 = mode) and a stinger (1 / 2)
  hands over to its return mode at once. So **each segment plays twice**, then the next: 1 1 2 2 3 3 4 4 1 1 ...
  (n = 4: 1 1 2 2 3 3 3 3 1 1). Changes take effect at the end of the file playing.
- Callers: race load 0xc8e10 (loader 0x805d9; View Replay 0x3ea0f, then mode 1 at 0x3ea1e before the music's tick, so
  the replay starts on on_start.wav, with 0x4407fc set only state 2 at 0x3e9fe; the attract demo 0x405e4, mode 0 only:
  intro.wav; [replays/users.md](../replays/users.md) "Users") mode 0; the countdown (0x44259, the
  loader 0x8065f / 0x806fc, 0x3ea1e) mode 1; the player starting a lap (0x63140): the final lap ([0x52cdd8]) one up
  (while 3..6), else lap 2 mode 2; the player's place (0x635c0): reaching 1st one up, leaving 1st one down (while
  over 3); Shield Low (0x4eb20): dropping under 33 % one up (0x4ecc6), back to 33 % one down (0x4eda1); POSTABLE ON
  (0x8eb10) writes mode 8 (the track's last segment for the results); front-end pages (0x27764, 0x28640, 0x29e10,
  0x3500c, ...) mode 3.
- Argon (audio\music_modes.cpp: MusicModes, MusicFolder; audio\music_sequence.cpp + song_stream.cpp): one stream per
  music, miniaudio's audio thread moving from a song to the next queued one in the middle of a read (sample-exact).
  Songs are decoded as they play (audio\song_feed.cpp: ~12 s ahead in a 2 MB ring, kept filled by a thread; FLAC / Ogg /
  MP3 from the file's bytes, audio\song_sources.cpp; a WAV decoded whole). The next song is opened on a worker; a mode
  change before the junction swaps the queued song (SongStream::requeue).
  Events: App::startCountdown, cueLapMusic, cueLeadMusic, stepRaceWarnings, openResultsTable (logic\music_cues.hpp);
  View Replay: Audio::restartRaceMusic(OnStart) (MusicModes::startOver: the stream stopped, mode 0, then 1).
  levelload_music keeps the folder order; Custom music (FE MUSICSELECT) picks each song at random as the game's
  0xdf150 ([frontend/options.md](../frontend/options.md) "Custom music"; play_order.hpp RandomPickOrder). stderr logs
  `music: <file> (mode n)` as each song starts.

**Music fades** (the race end). The music's volume is the music object's +4 (0x4406f4, DirectSound mB); its tick
0xdf4c0 calls IDirectSoundStream_SetVolume when it differs from +8 (the last set). A song started from stopped
(0xdf260, state +0x6c0 0 -> 2) sets +4 = music_volume_mb 0xdee90 (the Music Volume step's 0x1a8564 entry). The
race-end pages write +4 every frame of a one-second leave (the clock 0x292d58, seconds; "Saving Race" 0x520 shown in
the hint's place, the hints hidden, messages dropped), **linear in millibels**: ramp = -(t - t0) x 10000 (0x1a35a8),
then under -9600 (0x1a3968) -> -9600, else over the setting -> the setting (so at the default -400 mB the volume holds
for 0.04 s, then falls 100 dB/s).
- Out — Yes on WINMENU (0xa1420 cursor 6: +0x123d, +0x1240 = t0; fade 0xa0be6) / LOSEMENU (0x85660 cursor 5: +0x615,
  +0x618; fade 0x85007): the ramp for the whole second (-9600 from 0.96 s at -400 mB); at 0.5 s (0x1a3504) the save
  (0xa1950 / 0xbfac0), past 1 s (0x1a3454) the item runs with the music still at -9600: Race Again (0xa0cf0 / 0x85140)
  stops it at once ([0x440db0] = 0, 0xdf4c0) and the race's reload plays it again from the setting; Track Select /
  Main Menu (LOSEMENU 0x852a0: GAMELOGIC 0) do not touch it; something on the way out stops it (0x99fb0, entry 12 of
  vtable 0x19d880, stops every sound and the music: the caller is not traced; WINMENU's 0xa0e40 / 0xa0f20 / 0xa1010
  not read) and the front end's pages start their music (mode 3) at the setting.
- Dip — A on SCOREMENU (0x950be: +0x609, +0x61c = t0; fade 0x953ef), A on POINTSMENU (0x8d32d: +0x609, +0x60c; 0x8d46b),
  Continue on UPGRADEMENU (0x9e8f0: +0x60e, +0x610; 0x9ed9a): the ramp up to 0.5 s (-5000 there), then (t - t0) x 10000
  - 9000 (0x1a3964) with the same clamps (from -4000 back to the setting by 0.86 s at -400 mB); at 0.5 s the tournament
  progress 0x95d80 (UPGRADEMENU 0x9fef0); past 1 s the next page (0x95650 / 0x8d610 WINMENU / LOSEMENU, 0x9ffc0) with
  +4 = the setting (0x954e4 / 0x8d54f / 0x9eebb). The music's mode is not touched: mode 8 (POSTABLE ON) plays on.
- POSTABLE's A (0x8fa90), X on SCOREMENU (POINTSMENU), View Replay and Upgrade Menu (0x85380 / 0x85460) do not fade.
  The pause menu writes +4 only for its Music Volume row (0x89a13 / 0x89b3c, at once).
- Argon (audio\music_fade.cpp: musicFadeMb, musicFadeGain, MusicFade; ui\race_menus.cpp RaceMenu::startLeaving /
  stepLeaving; App::fadeRaceMusic): the pages' second of "Saving Race" and the curve as above on glfwGetTime, applied
  as a gain 10^((mB - setting) / 2000) on the race music (Audio::setRaceMusicFade, back to 1 when the race music stops
  or another starts). stderr logs `music volume: <mB> mB at <t> s (setting <mB> mB)` each frame of it; test
  `--finish 1..3 [--upgrade-menu] --finish-leave <s>` (A twice at that glfwGetTime). Tests: tests\music_modes_tests.cpp.
  Not compared by ear against xemu.
