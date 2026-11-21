# Front end: intro screens, title, attract demo

## Intro screens "FE INTROSCREENS" (vtable 0x19e368)
The shell's home page at boot (+0x1dc = 0x531530 until fe_title_update sets FE MAIN). Ctor 0x748d0, message handler
(enter) 0x12770, update 0x12050, input 0x12630, draw 0x898a0 (FE TITLE's). Labels: `tools\cmm\ghidra\label_introscreens.py`.
- The videos are Bink 1 ("BIKi", 640 x 480): `media\videos\ms_game_studios.bik` (308 frames at 30 fps),
  `media\videos\curly_seq.bik` (300 at 30) and `Locale\<language>\intro.bik` (2491 at 29.97), each opened as
  `%s...` with the disc root (0x52afcc). CMoviePlayer (wmvmovie.cpp, Open 0xe03e0); the main loop runs at 30 Hz while one
  plays.
- Enter (0xfe00 / 0xfe01): back from the attract demo with a saved page (0x5994c1, not "" or "FE MAIN") the shell goes
  to that page. Else the language folder goes to +0x1e0 by the language 0x5293b0 (XGetLanguage; 2 forced in the
  Japanese region, neon_start 0xb2722): 2 `Locale\Japanese\`, 3 German, 4 French, 6 Italian, anything else
  `Locale\English\` (so Spanish, 5, plays the English intro: the Spanish intro.bik on the disc is unused); the movie
  textures (0xe0480), video state +0x1d8 = 0, shell state 1.
- Update, shell sequence 1: when another title launched the game (0x52c8bf: neon_start 0xb26fb, XGetLaunchInfo found
  launch data of type 0) the title texts are laid out and sequence 7 follows at once (no videos); else sequence 5.
  Sequence 5 by video state: 0 ms_game_studios.bik opened (+0x1d4 the start time, +0x1dc = 3 frames, counted down in
  state 1 and not read elsewhere), 1 at its last frame (bink_is_last_frame 0x32530) curly_seq.bik (state 2), 2 at its
  last frame intro.bik (state 3), 3 at its last frame state 4, 4 the title texts laid out, sequence 7. Every state draws
  the current frame (bink_draw_frame 0x32410, a full-screen quad). Sequence 4 / 7: the last frame drawn once more
  (unless launched by a title), shell state 0, FE TITLE.
- Input: A (+0x88 & 0x10) or START (& 0x100) skips the playing video: state 1 -> curly_seq.bik, 2 -> intro.bik,
  3 -> state 4 (FE TITLE on the next update). No other button acts.
- The string tables (fe_strings, strings, infotext: neon_app_init_load 0x772d2; string_table_open 0x79030) are read
  from `%sLocale\<language>\%s` for 1 English, 2 Japanese, 3 German, 4 French, 6 Italian; Spanish (5) has no case
  (the path is left unset), so this release cannot run in Spanish. On the USA disc French, German, Italian and Spanish
  hold the same placeholder text ("xSélé¢t Profìlêöäß..."); English and Japanese are real. The intro videos are
  translated (French, German, Italian, Japanese); Spanish's intro.bik is a copy of the English one.
- The placeholder text is pseudo-localisation, the developers' test of a translation before it exists: the four
  languages' tables are byte for byte the same in all five files (fe_strings, strings, race_strings, infotext,
  cutscenes). Each string is the English one with accented look-alike letters ("Are you sure" -> "Åre ÿóú surë"),
  an `x` at each end (to see cut-off text) and `öäßü` padding, about 30-50% longer (to see whether translated text
  fits): Exit (fe 8) is "xExìtöäßüöäßüöäx". The tables are read right: of the header's four block offsets only the
  first is set (the others 0), and it holds this text; English's blocks 1-3 are set but the strings are in block 0
  too. In play: garbled text, rows too long for their panels, and symbols the game's fonts may lack (©, £, §, @).
  The EU (PAL) release should have the real translations in its `Locale` folders; copying them over the USA ones
  needs no code change ([argon/todo.md](../argon/todo.md) "Real translations").
- Argon (ui\intro_screens.cpp, movie\movie_player.cpp): the same order and skipping, with the .ogv next to each .bik
  (Theora, converted by `tools\assets\convert_assets.py` with ffmpeg and decoded by libtheora; the soundtrack, the movie's
  clock, is the Vorbis stream muxed in the same .ogv, its pages copied out for stb_vorbis by movie\vorbis_track.cpp);
  a missing .ogv is skipped.
  The language (text and intro) comes from `<saves>\settings.json` (viewer_settings.hpp; the game's five, offered
  only with real tables: App::findTranslatedLanguages; Japanese text is drawn in the MSPGoth font). The soundtrack plays through miniaudio and is the video's
  clock. Page 25 (`--menu-page 25`); `--no-intro` or settings.json's `skipIntroVideos` (as a launch by another title)
  and screenshot runs start on FE TITLE.

## Title screen "FE TITLE" (vtable 0x19dba0)
Build 0x34460 (fe_title_screen_init), enter 0x34cd0, update 0x34660, input 0x34970, draw 0x898a0.
- Texts (centred, font 0): fe 1 "Press START to begin" at (320, 320), hidden until sequence 8; fe 0xc
  "Developed by Curly Monsters Ltd" at (320, 421 - 30) and fe 0xd "(c) 2002 Microsoft Corporation..." at
  (320, 445 - 30) (0x195150 / 0x195154). No menu bars, logo, title or ticker (0x30303 skips FE TITLE); the
  background is the usual swirl. Colours (text element +0x1a0): "Press START" 0xffff8000 orange (0x34511 writes
  shell +0x374 = element +0x1d4 + 0x1a0), the other two keep the default 0xff85dde5; +0x204 = 2 on all three.
  Argon: ui\title_texts.hpp.
- frontend\titlelogo (bones face3, obj1, QRLOGO, skin1, spline320_bevel3; every mesh setup 2): mesh 0 face3
  "Specular Bump RGBA" (the cube reflection x its vertex colour 0x02141c: nearly black), mesh 1 obj1 the letters'
  faces, flag 0x23 = **unlit** (0x20), so white vertex colour x tex1 (white at the top, blue below), mesh 2 QRLOGO
  the black outline (flags 7: own ambient 0x2a2e36, black vertex colour, stage 0 env map tex2 ADDSMOOTH), mesh 3
  skin1 the additive blue glow, mesh 4 spline320_bevel3 the bevel ("Diffuse Specular Bump Map", tex0 = the normal
  map on both stages). frontend\dodechstart: dodeca2 "Specular Bump" (default.dds of data\defaultcubemap, slot 0:
  grey clouds, a sun and an orange horizon, so the frames turn silver, blue and gold as it spins), dodeca3 the
  additive shell (own ambient 0x0c1c3c), grid2 the light streak (16 vertices, ONE / ONE, tex1, camera-facing and
  occlusion-tested, flags 0x72887). The emblem keeps turning (3 tracks, its clock loops 10 .. 20 s), so its pose and
  the streak's place depend on when the screen is looked at.
- Argon fix (2026-11-16): the front end lit every prelit mesh (mesh_shading.glsl's front-end branch ignored the surface's
  lit bit), so the unlit letter faces came out at the ambient 0x70 x the texture, dark grey. Now the front end
  follows the surface setup as the race does (render\d3d_lighting.hpp surfaceLighting); of the front end's drawn
  models only titlelogo has an unlit, non-additive mesh (frontend\background, the bars and cursors are not 3D-drawn
  here).
- The user's real-Xbox capture of this screen is the 640 x 480 buffer with the dashboard's widescreen flag on
  (everything x 0.75 across, "Widescreen layout"): stretched to 16:9 it matches a 16:9 argon shot in the logo's
  place and size and in the text. Still different (2026-11-16): the background is lighter and more purple in the
  capture, most in the lower half (e.g. (42, 29, 44) against (13, 11, 21) at the bottom centre; the swirl's feedback
  is not checked against the hardware, its R5G6B5 rounding included), and the emblem's gold looks tan / greyer there
  (argon reads the cube at mip 0, mesh_shading.glsl cubeArray; the game's mip filter for that stage is not traced).
- Models (render list +0x6958 = camera 1, as the main menu items; matrix 0x50a180, identity): frontend\dodechstart
  (shell +0x3e48; 3 tracks, 299 keys at 10 / s) and frontend\titlelogo (+0x4458; 1 track, 50 keys at 10 / s), both
  loaded with the shell (0x30d0b) and flagged animated. Their clocks (model +0x5e4): the dodecahedron = seconds
  since enter, kept in 10 .. 20 once past 20 (the base moves on by 10); the logo: 0 in sequence 1, then 0 .. 4.9
  (sequence 8), 4.9 (5), and 4.9 - 4 t after START (7); the dodecahedron's drops to 0 once the logo's is < 0.7.
- Sequences (the shell's page state current_sequence): 1 (2 s) -> 8 ("Press START" shown, 4.9 s) -> 5. START / A
  (0x34970): in 5 -> 7, the logo flies back out, after 1.225 s the page leaves; in 1 or 8 fe_shell_set_state(4),
  and state 4 leaves at once (no fly-out). Leaving (0x34660 cases 4 / 7): UI sound 3 (SF_UI_SCREENFORWARD
  ui_xition1_LR), the page off, FE CHOOSESAVE.
- Sound: the front-end music only; the ui_amb3 loop, which other pages play when Music Volume is 0, never here
  (0x32550 skips FE TITLE, [audio/README.md](../audio/README.md) "Front end").
- Argon (ui\title.cpp): page 6, opened at start-up (`--no-title` or `--menu-sel n` start on FE MAIN).

## Attract demo (gl_attractdemo.cpp)
The "movie" that starts when the front end is left alone is not a video or an NMS sequence: it is an in-engine
replay of a recorded 6-craft race, played back through the race code's event buffer (CEventBuffer,
eventbuffer.cpp), with the loading screen in front of it as for any race. FE TITLE has no timeout of its own.
- Trigger (fe_shell_update 0x306b0, at 0x3075b): two idle clocks 0x439db8 / 0x439dbc += the frame time
  0x50e46c; once 0x439dbc > 75 s (0x1a382c) and the page (shell +0x1d8 -> +0x48) is not FE INTROSCREENS, FE
  MESSAGEBOX or FE OPTIONS CREDITS, fe_shell_start_attract 0x31c10 runs. So it starts from any other page (FE
  TITLE, FE MAIN, the options, ...), not only "Press START". The pad poll 0xb4bfc zeroes both clocks while any
  pad has a button held (+0x86), the d-pad (+0xda) or +0x12e non-zero; 0xa6c7d zeroes them too, and main_menu
  0x30c01 zeroes 0x439db8.
- fe_shell_start_attract 0x31c10 (once: shell +0x1630 = 0x5994c0 set, cleared by main_menu 0x30be2): the page
  gets message 0xfe02 (vtable +0x60); the return page +0x1631 (0x5994c1) is moved to +0x16b1 and the current
  page's name written to +0x1631; the shell's widgets hidden, the FE music stopped (0x440db0). Then
  d:\replays\replay<n>.info (n = 0x52c8b4 + 1) is read as text pairs: `track <t>` -> 0x52ccf8 (silver 0, kalau
  1, fuusoku 2, flames 3, solar 4, bracken 5, mount 6, draco 7, alt_silver .. alt_draco 8..0xf, mars 0x10,
  alt_mars 0x11) and `craft<i> <pilot>` -> 0x52cdc0[i] (tara 0, yuri 1, kumiko 2, darius 3, jordan 4, marcus 5,
  leah 6, kukai 7, alicia 8, vladimir 9, abi 10, zafir 11, kaitlyn 12, jaya 13, lukas 14, kutoo 15). Game mode
  0x52ccfc = 2, players 0x52cd8c = 1, craft 0x52cdbc = 6, drones 0x52cda0 = 5, 0x529fdc = 0, then LOADER.
- The files (on the disc, `replays\`): replay1 bracken (marcus, jaya, kumiko, kukai, jordan, zafir), replay2
  silver (tara, alicia, lukas, jaya, zafir, kukai), replay3 flames (darius, zafir, tara, abi, kumiko, jordan).
  Each is `.info` (the text above), `.index` (18432 x {float time, u32 offset into .events}, 16-byte steps; the
  first time -1 marks the end: 214.7 s, 223.1 s and 218.1 s) and `.events` (the event records; craft events
  are 0x58 bytes; the format is decoded in [replays/README.md](../replays/README.md)).
- The race (0x7f222: the game-logic class by game mode via the table 0x7f384; mode 2 = CAttractDemo, vtable
  0x1a1a0c: init 0x40440, update 0x40af0, shutdown 0x40d50, crafts 0x40310). attract_create_crafts 0x40310:
  craft 0 a player craft (vtable 0x19f3e8, unless 0x52abe0), the others drones (vtable 0x1a1c10). attract_init
  0x40440: hud0 message 3 "craft0 huddata" and message 1 "set none" (no HUD); z:\replay<n>.index / .events
  copied from d:\replays (CASyncFile::Copy 0xd8d00) when not cached yet; a CReplay "demo" (0xd9b70, +0xd2d4);
  0x52c8b4 moves on (0 -> 1 -> 2 -> 0: the three replays in turn); the crafts' matrices reset; camera0 message 2
  "replay on" / "replay0output" (the replay cameras, as View Replay); the crafts registered with the event
  buffer ("craft%d") and "WeaponEvents".
- attract_update 0x40af0 each frame: the usual game logic (0x3b920), eventbuffer_advance 0xdb260 to the FE
  clock, and while it returns 0 craft_replay_inputs 0x61c60 for each craft (the ships are simulated from their
  recorded inputs). It ends (GAMELOGIC message 7, vtable 0x1a1a3c -> 0x402b0 = 7, the same quit-to-front-end
  message as the pause menu's) when the replay runs out, or on a press (+0x88) of START or BACK (0x100 / 0x200,
  pads 1..3; 0x300 on pad 4), White, Black, A, B, X, Y or a stick click (bits 2..7, 10, 11, any pad:
  0x88450), an analog value +0x7c / +0x80 above 0 on any pad (0x88380), or a newly inserted pad (0x885a0). The
  d-pad and the triggers' bits 0 / 1 are not checked.
- Coming back: main_menu 0x30af0 (shell vtable 0x1a2890 [6]) reloads the front end on the shell's home page
  +0x1dc (0x59806c: FE INTROSCREENS 0x531530 at boot, set to FE MAIN 0x591418 by fe_title_update from sequence
  1 on), clearing +0x1630. fe_main_enter (0x29f44) then goes to the saved page 0x5994c1 unless it is empty or
  "FE MAIN" (fe_title_enter 0x34d2d and the page at 0x12770 do the same), so the front end comes back on the
  page that was left idle: from "Press START" back to FE TITLE (confirmed in the game: it comes back to the
  page it left). The loading screen does not wait for A before the demo (confirmed in the game).
- Ghidra labels: `tools\cmm\ghidra\label_attract.py`.
- The files hold each craft's position, velocity and orientation ([replays/README.md](../replays/README.md)), so the game places the crafts
  from them rather than re-simulating inputs; argon does the same.
- Argon (app_replays.cpp, logic\attract_demo.hpp, logic\replay_race.cpp): the idle clock runs in the front end's
  update (75 s, zeroed while any key, mouse button or pad input is held; not on FE INTROSCREENS, FE MESSAGEBOX, FE
  OPTIONS CREDITS or the loading screen), then replay1, 2, 3 in turn, each as a Quick Race with the .info track and
  pilots behind the loading screen (no wait for A), no HUD, no pause, the replay cameras (the director, [replays/users.md](../replays/users.md) "Replay cameras") in place of the TV
  cameras. START / A / B / X / Y or the replay's end go back to the front end on the page that was left. Tests:
  `--menu --menu-page 6 --attract-after 1` (the idle time shortened), `--menu --replay n --race-go`. To compare: the
  replay cameras' cuts (mode 0 not decoded), the 3 s easing on the grid, the flaps / jets from the control bits.
