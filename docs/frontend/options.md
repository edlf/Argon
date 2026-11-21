# Front end: options and custom music

## Options: "FE OPTIONS" (vtable 0x19da00), Game / Sound Settings
- FE OPTIONS: build 0x2c740, enter 0x2d9d0, input 0x2cef0, update 0x2d830, ticker 0x2e000. Title fe 0x280; hints
  Back / Select. Rows (font 1 x 0.75, panels as the main menu's) at (62, 100 + 30 i), sliding in from x 640 in 10
  frames: Game Settings (0x281) -> FE OPTIONS MAIN, Sound Settings (0x282) -> FE OPTIONS SOUND, Track Records (0x283)
  -> FE FASTESTLAP, High Scores (0x3c0) -> FE HIGHSCORES, Ghost Data (0x287) -> FE GHOSTMANAGE, Credits (0x284) ->
  FE OPTIONS CREDITS, Exit (fe 8; with the dashboard enabled a message box, else back), Easter Eggs (0x286, shown once
  one is unlocked, 0x1bbc08..) -> FE EASTEREGGS, Cheats (0x285, shown with 0x1bbc26) -> FE CHEATMENU. Ticker per row:
  0x370 / 0x371 / 0x372 / 0x377 / 0x378 / 0x373 / 0x374 / 0x376 / 0x375. B: back to FE MAIN. The page draws the main
  menu's Options model (shell +0x1ff8) at (-4, -1, 0) with camera 1.
- Settings pages FE OPTIONS MAIN (Game Settings; vtable 0x19d990, build 0x2f000, enter 0x2f1a0, input 0x2f0b0 (+ the
  gamma ramp 0x2f100)) and FE OPTIONS SOUND (0x19dc70: 0x2e970 / 0x2ef90 / 0x2ea20) run a shared engine over a list of
  entries (0x260 bytes, made by 0x81e00, options added by 0x81ee0): +0 type, +4 label, +0x104 description, +0x204
  option strings, +0x240 option widths (text width + 10), +0x24f choice, +0x250 count, +0x251 slot width, +0x252
  default, +0x254 action (A, no options), +0x258 change callback, +0x25c next. Rows (0xabca0): label at
  (62, 100 + 20 x (count - i)) with a panel (the cursor's), options from x 300; the choice 0xff85dde5, the rest
  0xff555555. Input (0xac040): up = next entry (the row above, wrapping to the first), down = previous, left / right
  step the choice, A on Exit back, on Reset to Default asks fe 0x341 (0xacb00), else the action; B back. Hints
  Back / Select or fe 0x48 Change Settings; ticker = the entry's description. Cursor starts on the last entry.
- Lists: every one starts with Exit (type 0, fe 8 / 0x33b) and Reset to Default (1, 0x332 / 0x333). Game (0x81fd0):
  Controller 4 / 3 / 2 / 1 Vibration (types 5..2, Off / On, default On), Speed Units (6, km/h / mph, mph for region
  1), Brightness (8, 0..7, default 4), HUD (0xf, Off / Map Off / Full, default Full), Multiplayer Laps (0x10,
  Default / 3 / 5 / 7 / 9). Sound (0x824c0): Music Options (9, action 0x2ebd0 -> FE MUSICSELECT), Engine Volume (0xb),
  SFX Volume (0xc), Music Volume (0xd) (0..7, default 4), Headphones (0xe, No / Yes). A value is read by type with
  0x81fb0; the in-race pause menu (0x898e0) uses the sound entries too.
- Argon (ui\options.cpp, the entries app_settings.cpp): pages 9 / 10 / 11; the choices are kept in the profile's JSON ("options"). Applied
  (see "Settings effects"): the volumes, Brightness, Speed Units (the HUD speed in mph, race_strings 624), HUD Off
  (no HUD) / Map Off (no minimap or its icons), Controller 1..4 Vibration. Kept only: laps, headphones.

## Settings effects
- Brightness (0x2f100 on change): ramp[i] = clamp(i + (step x 3 - 12) x 4, 0, 255) for R, G and B, then
  D3DDevice_SetGammaRamp; step 4 = the identity, 7 = +36, 0 = -48. Argon: the swapchain is UNORM, so after ImGui a
  full-screen constant |offset| is added (blend ONE / ONE) or subtracted (REVERSE_SUBTRACT) over the frame, clamped
  by the framebuffer the same way (App::drawBrightness).
- Volumes: 0x1a8564 holds DirectSound millibels per step 0..7: -10000, -3200, -1600, -800, -400, -200, -100, 0
  (0x1a8560 = -1 before it). Engine: 0x2eb80 -> IDirectSoundBuffer_SetVolume(table[step]); music: 0xdee90 returns
  table[step] of type 0xd. Engine Volume's preview (settings input 0xac040 after each cursor move): on the Engine
  Volume row (type 0xb) 0x2ea60 keeps the SFX choice, sets the music to -10000 mB, the SFX choice to the engine's
  and plays UI sound 0xa (SF_UI_ENGINETEST audio\frontend\enginetest, LOOPS 1; ids 6..9 are ui_amb3's four files);
  left / right on it set the sound's volume (0x2eb80); on any other row, B or leaving 0x2eb00 restores the music and
  SFX and stops it with a 0.5 s fade (0xcaea0). Argon: Audio::playEngineTest / stopEngineTest at the engine volume
  (no fade). (An earlier note had this on the SFX row.) Argon volumes: its own levels stay at the default step 4 and the others scale by
  10^((mB + 400) / 2000) (step 0 silent).
- HUD: 0 Off, 1 Map Off, 2 Full (default). Map Off hides the minimap (0x4f090) and its craft icons.
- Vibration: 0x78260 sets the pad object's +0x3510 + port to 1 when Controller n Vibration (types 2..5) is Off. The
  motors (player_pad_input 0x70ac0 -> 0xb46d0 -> pad +0x194 / +0x196 -> XInputSetState in 0xb4530, only on change):
  first = craft +0x9ea + +0x1138 (Rumble.wLeftMotorSpeed, the low-frequency motor), second = +0x9e8 + +0x1134
  (wRightMotorSpeed); 0xb46d0 sends the first only above 0x4000, the second above 0x2000. Off while the port's flag
  0x52c8c0 (= pad object 0x5293b0 + 0x3510 + port: the Off setting and the pause share it) or craft +0x11e is set.
  Once the craft is knocked out (+0x1145 with its time +0x1148, both set by 0x58d20; not the finish) both fade as
  round((3 - t) x 33) x 300 for 3 s, then 0. The values: [physics/rumble.md](../physics/rumble.md) "Rumble". Argon (App::updateVibration,
  logic\rumble_rules.cpp): each player's craft (split screen: player n's slot) to that player's pad, with Controller
  (port + 1) Vibration (setting 2 + port, the port the player joined with in Multiplayer, else player n), not while
  paused or in a menu, the knocked-out fade from Weapons::knockOut's clock, the pad's thresholds. The pads:
  platform\pad_rumble (the n-th connected XInput / force-feedback pad; a player's GLFW gamepad is matched by its
  place among the connected gamepads, so it is the right pad when only XInput pads are connected: not checked with
  several pads).

## Custom music
- On the Xbox, Custom plays songs ripped to the console's soundtracks: 0x440e70 (0x440e74 entries of 0x4c bytes:
  name wchar[32], +0x40 song count, +0x44 the game's own flag, +0x48 the soundtrack id; entry 0 is the game's own
  music, so Custom needs a second). The source is 0x1bbc27 (0 Custom, 1 Default; set to Default when the saved
  playlist finds no songs, 0xbf710). The playlist: 0x4406fc, 32 x (soundtrack, song), -1 empty, saved with the
  profile (0xbf350: soundtrack id, song, its name).
- FE MUSICSELECT with ripped music: Custom shown at x 362; left / right on Music Source switches (the choice 0xff85dde5,
  the other grey; Edit Custom Playlist bright with Custom); A on Edit (Custom only) -> FE LEVELMUSIC. Ticker 0x3e8 /
  0x3e9 by the source, 0x3eb Edit, 0x3ec Exit; right hint 0x48 "Change Settings" on Music Source.
- FE LEVELMUSIC (vtable 0x19dee8: build 0x26af0, enter 0x27930, input 0x27380, update 0x27650, lists 0x27b80 /
  0x27e10, move 0x27fe0, preview 0x28550). Title fe 0x21c "Edit Playlist". Headers "Songs in Playlist" (0x26c) at
  (62, 130) and "Songs not in Playlist" (0x26d) at (318, 130) (630 - 62 - 250, 0x1a36c8 = 630); two 250 x 200 areas
  at (62, 160) / (318, 160), 10 rows each at y 160 + 20 i, "soundtrack\song" (the soundtrack name at most 31
  characters). The areas' look (0x27190, vtable +0x2c: drawn under the elements): a 1 px black outline (0xab110,
  colour 0xff000000, thickness 1) on (x, 160)-(x + 250, 360), filled 1 px inside, (x + 1, 161)-(x + 249, 359), with
  0xa0204060 (0xaad40: translucent dark blue). Hints Back / Select Song
  (0x45), Select Column (0x47) / Preview Song (0x46). The cursor starts on the songs not in the playlist (or the
  playlist if every song is in it); up / down scroll (the top row follows once the cursor is 9 below it), left /
  right change column (only to one with songs), A moves the song to the other list (the first free of 32 slots),
  Y plays it, B back to FE MUSICSELECT and the front-end music again.
- A song moved out of the playlist (0x27fe0 -> 0x28210) closes the gap: the slots after it move up one, so the
  playlist is the filled slots before the first empty one.
- In a race Custom plays "random music from your ripped tracks" (fe 0x3e9): CSoundtrack::OpenSong 0xdfe20 (at the
  race's load 0xc8e7f and, through the stream's packet fill 0xdfad0, each time a custom song ends) picks the song
  with 0xdf150: n = the slots before the first empty one, the song = slot rand() % n (0xf96ea): each pick on its own,
  so a song can play twice in a row and the playlist never ends. With Default, or no playlist, 0xdf150 gives the
  game's own music (soundtrack 0, the track's folder + 1). The songs are WMA (WMADECXM, 0x159417, opened by
  open_wma_file): the Xbox's ripped soundtracks are WMA only.
- Argon (ui\playlist.cpp, the music app\app_music.cpp): page 19. <game>\custom_music stands for the ripped soundtracks: each folder is one, its .wav
  files (PCM or Xbox ADPCM, as the game's own), .flac (dr_flac), .ogg (Ogg Vorbis, stb_vorbis) and .mp3 (dr_mp3; the encoder's start / end padding is not trimmed) the songs (more than 2 channels: the first two); files directly in custom_music form a soundtrack called
  "custom_music". The profile JSON keeps "musicCustom" and "playlist" ("soundtrack\\song.wav|..."). In a race with
  Custom and a playlist the track's music is replaced by random picks as the game's (audio\play_order.hpp
  RandomPickOrder, the C runtime's LCG seeded at random). The areas as decoded (LevelMusicPage::drawArea). Not
  ported: WMA (the game's format) and mp3 / Opus need decoders argon does not have (WMA: e.g. FFmpeg; mp3:
  dr_mp3 from the same dr_libs): not added. The decoders are downloaded by CMake into build\_deps (pinned commits: dr_libs f12e197, stb 78770d8).

## Options sub-pages
- FE MUSICSELECT (Music Options; vtable 0x19e160: build 0x2b920, enter 0x2c240, input 0x2bd80, update 0x2c430, ticker
  0x2c660). Title fe 0x258 "Music". Rows Music Source (0x259 at (62, 120); Default 0x25a at x 262 / Custom 0x25b at
  x 362), Edit Custom Playlist (0x25c, (62, 140): A -> FE LEVELMUSIC "Edit Playlist", vtable 0x19dee8, with Custom
  on), Exit (8, (62, 160)) -> back to Sound Settings. Custom needs 2+ ripped soundtracks (0x440e74); without them
  Custom is hidden, Default and Edit Custom Playlist grey, the ticker fe 0x3ea ("You have no ripped music..."),
  Exit 0x3ec; with them 0x3e8 / 0x3e9 by the source, 0x3eb Edit. With ripped music: see "Custom music".
- FE OPTIONS CREDITS (vtable 0x19e1c8: enter 0x2e170, input 0x2e110, player 0x2e390): no menu bars. The text: fe
  0xb68 + n (n 0..0x61) with "[C:aarrggbb]" / "[S:x]" tags, centred on 320 at 0.65 (0xb11f0), pages stacked by their
  height from y 50, scrolling up 1 px a frame from 2 s after entering; a page above the top is dropped; past the
  last back to FE OPTIONS; A / B leave. Pictures: two slots, each moving and scaling over 8 s by one of four motions
  (0x1b99a0: (0, 350) -> (200, 500) x 1.5 -> 2, (500, 250) -> (450, 250) x 0.9 -> 1, (640, 350) -> (440, 500) x 1.5
  -> 2, (140, 250) -> (190, 250) x 0.9 -> 1), then the next motion of its pair and the next of 16 pictures (slot 0
  starts on picture 8 with motion 0 at + 1 s, slot 1 on picture 0, motion 1). The pictures are the shell's 16 image
  elements +0x765c (0x59eee8 + 0x604 + n x 0x20c), loaded by main_menu 0x30af0 from data\charbitmaps\<character>.dds
  for the characters 0..15 (0x78b30 the folder name; 256 x 512, kukai / lukas 512 x 512), drawn at the texture's
  size x the scale, centred on the motion's point (0xcc050), alpha blended. After them a black rectangle over the
  screen (0x2e7ab -> 0xab290 (0, 0, 640, 480)), alpha 255 - 255 x clamp(now - the start (enter + 2 s), 0, 1): from
  the start the pictures and background come out of black over 1 s. The text's "[QRLOGO]" is an icon tag (0x77272:
  data\qrlogo.dds, registered with the tag table's +0x44 set, 0x39bda4): drawn at the texture's own size (256 x 128)
  whatever the text scale (0xb1ce7). The shell's menu bars are skipped on FE TITLE and FE OPTIONS CREDITS (0x30493).
  Enter also sets 0x5a1e79 = 1, 0x388f50 = 1 (0x388f54 the clock, 0x388f58 1.0; the pause menu's flag too) and sizes
  each picture element from its +0x1e8 / +0x1ec (0x2e300; not decoded). Argon: as decoded (ui\credits.cpp; the
  ImGui descriptor pool grew to 128 for the pictures), over black (seen on the Xbox; the XBE's reason not found).
- Track Records = FE FASTESTLAP (vtable 0x19e028: build 0x1f9f0, enter 0x20fa0, input 0x20a60, update 0x206c0, rows
  0x1fef0, time 0x1fe10, frames 0x205f0). Title 0x208; ticker 0x209. The speed class (infotext 0xfa + speed) right at
  (568, 106) orange; headers Track 0x3c1 / Fastest Time 0x20a / Profile 0x20b centred at 165 / 350 / 510, y 160,
  orange; frames header y 155 x 25 (swapped) and body y 180 x 160 at x 50 (230), 280 (150), 430 (160); rows y 180 +
  16 i: the track (infotext 300 + i, + 8 for the second routes; Mars 0x13c / 0x13d or "????" 0x17f) at x 55, the
  time (record table 0x1bbc34: 0x18 bytes, +4 time, +8 name; index speed + 5 x track) at 350, the profile at 508.
  Left / right the speed, up / down the routes, B back; hints Back, More Tracks (0x4a), More Speeds (0x49).
- Save-new-records mode (enter 0x20fa0: +0x4150 when any 0x52ac03[track] is set): B does nothing; hints More Speeds
  (0x49) left and Continue (0x40) right in the header, More Tracks (0x4a) right in the footer; A (input 0x20a60)
  clears every flag and, with a profile (0x459a8c clear) and no new high score (0x52ac15[track], FE HIGHSCORES'
  flags), opens FE MESSAGEBOX kind 4 "Saving" (+0x414d: the next enter leaves), else leaves at once; either way to FE
  MAIN 0.1 s later (state 7). Coming back from a race (+0x4151) the speed is the game speed and the page shows the
  second routes when no first route (0..7) nor Mars (0x10) has a new record but a second route or Terraformed Mars
  has. The update (0x206c0) flashes the new records' rows (all three columns) on the shown speed when it is the game
  speed: grey 255 down 16 a frame, back to 255 below 64.
- The lap records (g_track_records 0x1bbc34, 18 tracks x 5 speeds x 0x18 bytes: +4 time, +8 the profile's name): the
  game's "laptimes" save (0xc0f90 writes the 0x870 bytes on U:\). Reset (0x83cc0): every time 999999, every name fe
  0x20c "--------" (the profile reset 0x83b60 also sets the profile's own 90 best laps, profile +0, to 999999). In
  every mode the race's best (+0xd338) starts at the track's record (pickups_init 0x3e4a0) and the player's laps
  (race_craft_lap 0x3d5a0; Time Attack also 0x429c0) beat it: the new best shows "New lap record" (+0xd32c), and a
  lap under the table's time replaces the record with the profile's name (0x52ac03[track] = 1). Time Attack also
  compares the profile's own best (g_profile + (speed + track x 5) x 4): Time Attack's lap end (0x429c0) takes the
  table's record only for a lap that beats the profile's best too, and on any profile best it sets the shell's
  pending page (0x5994c1) to "FE FASTESTLAP", which FE MAIN's enter (0x29f49; FE AWARDS 0x12780 too) opens instead of
  itself. The other modes' lap end (race_craft_lap 0x3d5a0) never touches the profile's bests.
- Argon (app_laprecords.cpp): the table in <saves>\records\laptimes.json ({"lapRecords": [{"track", "speed", "time", "name"}]}),
  read when the menu opens and written at once when a record falls (no FE FASTESTLAP save step); menu races only
  (not the editor). Track Records shows each record's time (the HUD's lap format) and name. "New lap record" (3 s,
  argon's time) now follows the race's best. The profile's own bests ("bestLaps" in the profile JSON), Time
  Attack's way back through FE FASTESTLAP and its save-new-records mode are ported (2026-11-14,
  logic\lap_record_rules.cpp; Continue writes the table again: argon already wrote it when the record fell).
  `--new-record <track>` (tests) flags a track's record as new.
- FE HIGHSCORES (vtable 0x19e230: build 0x24fb0, enter 0x26490, input 0x25f20, update 0x25b20, rows / frames
  0x254f0). Title 0x3c0. The character (infotext 200 + id) right at (568, 96), font 1 (xemu); headers Track (55), Tournament (0xf0,
  360), Quick Race (0x7b, 515) at y 160; frames as Track Records but the second column 160 wide and the third at 440, 150 wide (xemu: they touch); no
  ticker panel (as Character Data, Ship Data, Track Records, Ghosts); scores from tables
  0x1bb108 / 0x1bb150 (Mars 0x1bb190 / 0x1bb194), "n/a" 0x3c3. Left / right the unlocked characters, up / down the
  routes; hints Back, More Tracks, More Characters (0x4b).
- Ghost Data = FE GHOSTMANAGE (vtable 0x19dad0: build 0x215f0, enter 0x21c80, input 0x22000, frames 0x21f10, list
  0x223f0, leave 0x21b30). Title 0x3fc "Ghosts"; frames (52, 100) 536 x 40 (swapped) and (52, 140) 536 x 160. No
  ghosts: fe 0x3fd at (62, 150) 516 wide, hint Back only. With ghosts: one track at a time (0x52ccf8; enter moves to
  the first of 0..0x11 with ghosts): its name (infotext 300 + track, in element +0x1440; its place and font are not
  decoded) and five rows font 0 at (62, 170 + 20 i), each a ghost of that track named by its character (infotext
  200 + id; a bad one "Invalid ghost." 0x1e0), the arrows as FE GHOSTSELECT's ((82, 148) / (82, 279)). Left / right:
  the previous / next track with ghosts (wrapping), the cursor on top; up / down scroll; X asks fe 0x3fe + the row's
  text (kind 5) and deletes it; B back (after the timed leave). Hints Back, footers "[BUTLFIX][BUTRFIX] - More
  Tracks" (0x4f) / "Delete Ghost Data - [BUTX]" (0x4c), ticker 0x3ff "Select ghost to delete.". Argon
  (ui\records.cpp): as decoded, the track's name font 1 at 0.6 centred in the top frame (a guess); unreadable ghost
  files are left out rather than listed as invalid.
- Argon (ui\options.cpp, ui\credits.cpp, ui\records.cpp): pages 12 Credits, 13 Music, 14 Track Records, 15 High
  Scores, 16 Ghosts. Records: see the lap records above (Ghosts: see "Time Attack"); with none, Track Records shows "--:--.--" / "--------" (0x20c), High
  Scores "n/a", Ghosts the empty message (the rows' values with data, the records' time format and the High Scores
  row spacing are not checked). Easter Eggs and Cheats: see "Easter Eggs and Cheats"; Edit Playlist: "Custom
  music".
