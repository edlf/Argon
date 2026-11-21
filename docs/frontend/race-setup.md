# Front end: character, track and speed select

## Character select, Character Data, Ship Data
Ported in `src\ui\charsel.cpp`; labels `tools\cmm\ghidra\label_charselect.py`.

- Flow: FE MAIN Quick Race -> FE CHARSELECT; A (unlocked) -> FE COURSE SELECT (quick race) / FE TOURNEYTYPE
  (tournament); Y -> FE SHIP INFO, X -> FE CHAR INFO, both back to FE CHARSELECT with B. Argon: A starts the race
  on the editor's current track with that pilot (FE COURSE SELECT and FE SPEEDSELECT not ported).
- Cameras: the shell has two (0x2f4b0): camera 1 (+0x60c0) eye (0, 0.01, 0) looking at (0, 0, 1), fov pi x 0.2857
  (51.4 deg; the main menu item models); camera 2 (+0x64c0) eye (-0.79, 4.68, 7.57) looking at (-0.8, 4.79, 6.58),
  fov 20 deg (0x2f9a9 / 0x2f9c0), used by the character and ship models (render list +0x690c). 4:3.
- FE CHARSELECT (vtable 0x19e298): 16 characters in 2 rows of 8 (0x163b0: left / right wrap within the row, up /
  down swap rows). Draw 0x178a0: frame panel (84, 240) 472 x 125 glow, fill 0x80404080 top / 0x80804040 bottom,
  scale 0.5; tiles 54 x 54 at x 90 + 58 i, y 246 / 305, glow, the selected one highlighted and filled 0xffa08040
  (bottom corners 0); unlocked: the pilot icon (0x528ea8 + id x 0x50 = data\hud\tex<0x1b93f8[id]>.dds), 2 px bigger
  when selected; locked: "?". Portrait: the icon 128 x 128 at (88, 110) until the 3D character has loaded. Name (infotext 200 + id, "?" if locked) at
  (560, 110), 0.75, right-aligned, yellow (xemu). Info (0x15550 / 0x19ba0, right at 568, font 0; xemu: the labels
  orange, the values 0xff85dde5; the five rows slide in from x 1280 to 560 over 10 frames on entering and back out
  while leaving, 0x19990 from the enter 0x196f2 and the leaves 0x16b9f / 0x16f57 / 0x1738a): Trophy Level (infotext 250 +
  the highest trophy level held, or fe 0xcc None: 0x19c48 scans 0x1bb0ef + character x 0xb0, levels 0..4; ported
  2026-11-14), Home Circuit (infotext 300 + id), its difficulty "(strings 1700 + id % 8)".
  Ticker (0x19760): locked -> fe 0xaf + id (7: 0xb6), an unlocked original with its nemesis locked -> 0xb4 / 0xb5.
  Hints: Back / Select (Select hidden when locked), footers 0x37 "[BUTY] - Ship Data" / 0x38 "Character Data -
  [BUTX]" (y 445; hidden when locked). Title fe 0xa0.
- 3D character (0x15db0): frontend\characters\<pilot> (skinned), at table 0x1b9a88[id] (e.g. Abi (-3, -21, -80)),
  rotation 0 (0x5a2160..68 never written), lights: light 0xcccccc along (-0.5, -1, -1), ambient 0x707070; a locked
  character with the lights off (0xaf3d0): a black silhouette. The shell draw (0x30303) checks shell +0xa23c
  (0x5a20cc, set to 1 by 0x162da for a locked character): its branch 0x304fb turns light 1 off (LightEnable(1, 0),
  0xa7ea0 black along (0, 1, 0)), calls 0xaf3d0, draws the model list, restores the lights and fades the echo over it
  at max(+0xa238, 0.2). What makes it a silhouette: mesh_apply_material_stages (0xba660) ends (0xbad5b) by setting
  every stage's colour op to code 0 (MODULATE) when light 0's colour (0x50e2f8, from 0xa82d0) is black, unless the
  mesh's surface has flag 0x2000; skinned meshes (mesh flag 0x40000, 0xa3df0 with the char*bone.xvu vertex shaders)
  do the same again when the shell light colour 0x391100..08 is black. So texture-only and added layers (Kutoo's
  body: code 0x17 then an added reflection; its head plate) take the black lit colour; only unlit glows stay (Kutoo's
  eye quads grid1 / grid1_1, additive). Checked live in xemu (tools\xemu\tsslog.py: 23 / 13 from 0xba6ec / 0xba782,
  then code 0 on all four stages from 0xbad8c..0xbadbf, for every Kutoo mesh). Argon: mesh_shading.glsl (material bits 8
  skinned, 16 surface 0x2000). Its idle animation is the model's own bone tracks
  ([models/meshes.md](../models/meshes.md) "Animation"), clock = seconds since the character first showed (0x15db0 -> model +0x5e4); argon
  plays it the same way. Both cameras use near 0.001 / far 5000; argon draws the character, ship and globe
  pages with near 1 (with 0.001 its depth buffer resolves only ~0.4 units at the character, so the moving fingers
  and hair cards flickered).
  Translucent meshes (the hair cards, blend 0x00100023: alpha = stage 2's texture alpha, SELECTARG1) come out with
  hard edges in xemu; argon cuts them at alpha 0.5 (Track::translucentAlphaRef, the character and ship pages),
  with ref 0 the cards' half-transparent black edges left a dark fringe. The game's reference there (render list
  bucket 4, 0xbbf60, alpha test on) is not decoded. A new model fades in over 1 s (the model fade below).
  Lighting (shell draw 0x2ffa0 before the render lists +0x690c): light 0 0xcc along (-0.5, -1, -1) (0xa80d0 /
  0xa7f60, D3DLIGHT8 at shell +0x60b4, SetLight 0x10ccc0), ambient 0x707070 (0x3033e); the D3D material (0xa6ad7)
  diffuse / ambient / specular 1, emissive 0, with COLORVERTEX on and the diffuse from the vertex colour, the
  ambient from the material (render states 0x1188c4..0x1188e4, read live): lit = ambient + vertex colour x light x
  N.L. Leah measures within a few percent of xemu. The background (the darkening panel's plasma, 0xc1b40: its own
  vertex colours 0xffffff / 0x8080ff / 0xff8000 at alpha 0x70, 0xb0 selected, stages colour MOD2X / MODULATE /
  MODULATE / MOD2X, alpha MODULATE, blend preset 1) builds up through the feedback from black: in argon it
  reaches the game's level (about (35..50, 30..45, 60..80)) some 15 s after start.
- Unlocks (0x1bb0e0, 16 bits): a new profile has the seven originals (0..6); winning a tournament with character
  i < 7 unlocks i + 8, with the seven originals character 7 (0x84160); 0x19f10 sets all. Argon: the profile's (app_tourney.cpp unlocks as 0x84160 does); no profile: all unlocked,
  `--new-profile` = 0x007f.
- FE CHAR INFO (vtable 0x19d7f0, build 0x14370, enter 0x14bf0): title fe 0x168; name (568, 86) font 1 x 0.75
  orange (0xffffbe64); Age / Origin / Quote labels right at 568 (y 131 / 167 / 207), values orange (149 / 187;
  strings 1501 / 1502 + 4 id), the quote (strings 1720 + id) at (62, 225), 516 wide; biography (strings 1500 + 4 id)
  in a gradient frame (0xc2310) (52, 265) 536 x 125, text from (62, 275), 5 lines shown (xemu; 0x14a50 scrolls
  by 4), arrows
  icons\up.dds / down.dds 40 x 20 at (300, 268 / 368), footer 0x39 "Scroll Biography" when it scrolls.
- FE SHIP INFO (vtable 0x19dd48, build 0x32a30, enter 0x32ee0, draw 0x326a0): title fe 0xdc; ship
  frontend\shipmodel\<pilot>, YPR (0, 25 deg, 0) after a turn about y of -0.5 deg a frame, at (-2, 6.95, -12)
  (0x33230); 7 stat bars (0xc2420: black panel, fill 0xffff8040 of stat x 10 % from the right) at x 368, 200 x 24,
  y 100 then +28 (+44 after Turn and Aerodynamics), labels fe 0xe0..0xe6 right at 560 (0.8); name (62, 100) shrunk
  to 300, yellow (xemu); portrait 64 x 64 in a gradient frame at (62, 130); Ship Name / Weapons / Weight (fe 0xdd..0xdf) panels
  (62, 309 + 20 i) 120 x 17 fill 0x60404070, labels (70, ..) 0.8, values orange (190, ..) 0.5 x 0.55: ship name string
  1564 + id, weapons 1503 + 4 id (xemu: Zaltys / Corrosives for Yuri), weight table 0x1b9798 "%d kg" (or lbs x 2.2 by a setting). Table 0x1b9798 (8 ints per pilot):
  weight, Acceleration, Power, Turn, Aerodynamics, Track, Off Road, Water.
- Strings: fe_strings (0x52c8a0), infotext (0x52c8b0: names 200+, trophies 250+, tracks 300+), strings (0x52c8ac:
  biographies 1500+, difficulty 1700+, quotes 1720+).

## Track select "FE COURSE SELECT" (vtable 0x19de18)
Quick Race goes FE CHARSELECT -> FE COURSE SELECT -> FE SPEEDSELECT (or FE TRACK INFO with Y) -> the race.
- Picking a character in Quick Race puts the cursor on its home track's row (character id & 7).
- Build 0x1cfb0, enter 0x1dfd0, input 0x1da10, update 0x1d860, draw 0x1d360 (ctor 0x75400). Title fe 0x17c
  "Select Track".
- Rows (0x20c-byte text elements at +0x1d4): 9, at (62, 100 + 30 i), scale 0.6, with panels; they slide between
  x 640 and 62 in 10 frames (0x1e6a0). Rows 0..7 = the tracks (infotext 300 + i: Silver Rock Canyon, Kalau
  Wastelands, Fuusoku City, Flames of Ra, Solar City, Brackenstowe Vale, Mount Grauen, Draco Island), row 8 = Mars
  (fe 0x182, locked 0x17f "????..."). Colour 0xff85dde5, locked 0xff555555.
- Unlocks: 0x1bbbfc (bit i track i, bit i + 8 its second route), 0x1bbc00 Mars; 0x19f20 sets all. Argon
  has everything unlocked (`GameProgress::tracks`, `mars`); the new-profile values are not known.
- A (0x1e5e0): locked -> buzz; the second route unlocked (Mars: always) -> the route choice (0x1e4d0): two
  buttons right-aligned at 568, y 300 / 330, fe 900 + i "1: Tara's Route" (the track, id i) / 0x38c + i
  "2: Alicia's Route" (its alt_ version, id i + 8); Mars 0x394 "1: Martian Route" (0x10) / 0x395 "2: Terraformed
  Route" (0x11); else the track. B closes the choice, else back to FE CHARSELECT. Y: FE TRACK INFO (not ported).
- Track ids -> folders (0x1b9538): silver, kalau, fuusoku, flames, solar, bracken, mount, draco, then alt_ of
  each (8..15), mars (0x10), alt_mars (0x11).
- Ticker (0x1e380): empty; locked Draco fe 0x183, locked Mars fe 0x184. Hints Back / Select, footer 0x3b
  "[BUTY] - Track Data".
- Globe (0x1d360): frontend\earth (shell +0x4a68 = 0x59c8f8; only lotex%d.dds) for rows 0..7: rotation Y(turn y)
  then X(turn x) (row-vector), x 0.9, at (-8, 0, 40), drawn with the main menu camera; the turn eases 0.2 of the
  way to table 0x1b9a18[row] (degrees: (-33, -2), (-55, -188), (-35, -238), (-30, -137), (27, -254), (-56, -104),
  (-47, -117), (-21, 53), Mars (0, -106)) each frame, set at once on enter. Corner brackets mark the track at the
  screen centre of the globe: yellow 0xd0ffff00 (413..433, 228..248, 5 px arms), dark 0xd0202000 (411..435,
  226..250, 6 px). Row 8: frontend\mars (+0x5078 = 0x59cf08) tilted -25 deg about X after a turn about Y of the
  clock in seconds (1 rad / s), unscaled, no brackets. Light 0 along (-1, -1, 1).
- Argon: picking a route starts the race on that track (FE SPEEDSELECT not ported: argon's speed class).
  The route buttons (build 0x1cfb0: elements +0x162c / +0x1838): font 0 at 1.0, right-aligned at 568 (y 300 / 330),
  each with a panel; the cursor starts on the first (0x1e544). `--route-choice` (with `--menu-page 4`) opens them.

## Select Speed and Track Data
- FE COURSE SELECT leaves (0x1d860) to FE SPEEDSELECT after A, or to FE TRACK INFO after Y (+0x1a48 set by 0x1da10:
  Y works on an open track, not on Mars; the track id 0x52ccf8 = the first route).
- FE SPEEDSELECT (vtable 0x19d720: build 0x33560, enter 0x34200, input 0x33ac0, update 0x33fa0, ticker 0x343e0).
  Title fe 0x2bc "Select Speed". Rows infotext 0xfa + i (Novice, Amateur, Expert, Master, Redshift) at
  (62, 100 + 40 i), font 1 x 0.75; the highest open speed is 0x1bb0e4 (outside Tournament at least 1), higher rows
  0xff555555. Enter: the cursor on the game speed (global_game_speed), lowered to the highest open one;
  hints Back / Select (0x3d; Time Attack 0x32). Up / down wrap 0..4; A on an open speed sets global_game_speed and
  0x1bbbe8 and goes on (Time Attack FE GHOSTSELECT, Tournament FE GOALDISPLAY, else LOADER); on a locked one an error
  sound; B back to FE COURSE SELECT. Ticker: a locked speed's hint fe 0x116 + speed (0x118 "Win the Amateur
  tournament to unlock." ...), else none.
- The speedometer (update 0x33fa0, as FE TOURNEYTYPE's 0x35e50): main_menu loads frontend\speedselect into shell
  +0x5aa0 (0x30ea9); +0x60b4 = its node "SPEEDO_NEEDLE" (0xb9c40), +0x60b0 (0x59df40) its root (0xb96e0). Each frame
  0xb99f0 sets the needle node's rotation to (0, 0, 35 deg x the cursor's speed) (0x1a37c0 / 0x1a36f4 / 0x1a389c /
  0x1a393c for 1..4, else 0; 0x5a211c is pi / 180): its local 3 x 3 = YawPitchRoll(0, 0, a) (0xf3117) with the
  node's scale, the translation kept. The model is drawn unmoved (identity from 0xf3117 with 0 x pi / 180) into the
  camera 1 render list (0x298ee0 = shell +0x6958) with the menu camera (0x5a1df8). Its two quads (SPEEDO_DIAL,
  SPEEDO_NEEDLE, 12.4 wide x 0.83) sit at (-4.55, -1.19, 20): right of centre, below the middle.
- FE TRACK INFO (vtable 0x19e300: build 0x36a60, enter 0x37370, input 0x371e0, update 0x37800, draw 0x36840). Title fe
  0x190 "Track Data". Right-aligned at 568: the track (infotext 300 + id, orange 0xffffbe64) y 90, "Location:" (fe
  0x17e, orange) 120, the location 140, "Length:" (0x17d, orange) 160, the length 180, "Home Track Of:" (0x180,
  orange) 200, the character (infotext 200 + t) 220, the nemesis (infotext 208 + t, only once unlocked) 240 (the last
  four hidden for Mars). strings.str per track t: 0x640 + 10 t length km, 0x643 + 10 t miles (Speed Units), 0x641 +
  10 t location, 0x642 + 10 t description; Mars 0x690..0x693. The description at (62, 280), 516 wide, 4 lines
  (+0x1430), scrolled 4 at a time (+0x1434), arrows icons\up.dds / down.dds at (300, 268 / 368) 40 x 20, footer
  "Scroll Track Data" (0x3a) when longer. Draw (0x36840): the track's picture, then the frame (52, 265) 536 x 125.
  B back to FE COURSE SELECT. The picture: enter 0x37370 loads data\frontend\track_<t>\track_<t>.dds into shell
  +0x5a68 (0x59d8f8; second routes as their first, Terraformed Mars as Mars); the draw sets a viewport (52, 76) 536 x
  188, blend ONE / ONE add (0x40304 / 0x40344 / 0x40348 / 0x40350), no culling, and puts it through
  hud_draw_rotated_quad (320, 150, 0, -128, 128, 640, white): 640 x 256 centred on (320, 150), clipped to the
  viewport.
- hud_draw_rotated_quad (0xcbe50; x, y, angle, from, to, width, colour in eax, texture in esi): a = angle + 180 deg
  (0x5a211c x 0x1a36fc); along = (sin a, cos a), half = (cos a, -sin a) x width / 2; corners to-end - half, to-end +
  half, from-end - half, from-end + half with uv (1, 0), (0, 0), (1, 1), (0, 1). FE CHOOSESAVE's card uses it too.
- Argon (ui\speed.cpp): pages 20 / 21; the profile's "speeds" is the highest open speed (`--unlock-all` 4,
  `--new-profile` 1); the chosen speed is the race's speed class. The speedometer (FeModels::speedometerScene, the
  needle turned by FeScene::turnedBone) and the track's picture (ui\picture_quad.cpp) are ported (2026-11-14). Not
  ported (2026-11-14). Track Data's texts (build 0x36a60): the track's name font 1 at 1.0, orange; every other
  label and value font 0 at 1.0, right-aligned at 568; the description font 0 at (62, 280).
