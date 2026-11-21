# Physics: the in-race HUD

## HUD (gamehud.cpp; labels from tools\cmm\ghidra\label_hud.py)
Functions: CGameHud_ctor 0x448b0, gamehud_setup 0x46040, gamehud_layout 0x45270, gamehud_update 0x496f0,
gamehud_render 0x4a100, hud_minimap_draw 0x4f090, hud_draw_rotated_quad 0xcbe50, hud_draw_bar 0xc24d0,
font_draw_text 0xb1660, hud_time_format 0x1f910. Ported in src\hud as the game's element tree: `GameHud`
(game_hud.cpp: the ctor's elements, `layout` = 0x45270, `update` = 0x496f0 with Time Attack's hiding 0x49440, `render`
= 0x4a100 drawing the elements, the shield bar, the turbo fan and box and the needles, then the markers 0x4d9d0 in
markers.cpp), its elements `HudText`, `HudSprite`, `HudPowerUpPanel` (+0x3950), `HudPointsPopup` (+0x9380)
(hud_elements.cpp) and `HudMinimap` (0x1a0df8: map, icons, position list; hud_minimap.cpp), all drawn through a
`HudCanvas` (hud_canvas.hpp). The game's group element (0x19aec0) has no class: GameHud holds the elements itself.

- Elements: group (vtable 0x19aec0), text (0x19afc0), sprite (0x19b0d0), minimap (0x1a0df8). Slots +0x2c draw,
  +0x40 pos (+0x24/+0x28), +0x44 size (+0x3c/+0x40), +0x48 scale (+0x30/+0x34, UV extent for sprites),
  +0x4c visible (bit 8 of +0x1c4), text +0x64 set string. Text: colour ARGB +0x1a0, font +0x1d4 (0 = 18 px,
  1 = 34 px), flags +0x204 (&0xf 0 left / 1 right / 2 centre, &0xf0 0x10 vcentre / 0x20 bottom, &0xf000 per line).
- Fonts `fonts\Square721 BdEx BT 18|34` (.dds 512x512 + .map: u16 first=32, u16, u32 count 344, f32 line height
  19/35, then 5 x 1024 floats by char-32: u0, v0, u1, v1, advance). Glyph quad = (u1-u0, v1-v0)*512 at the pen,
  pen += advance*512 (glyph boxes overlap their padding). `[F:n]` inside a string switches font.
- Strings `Locale\<lang>\race_strings.str` (u32 x 8 header, block at h[3]: count at +4, (u16 id, u32 off) from +8,
  UTF-16 at +8+count*6+6+off*2): 600 Turbo, 601 Shield, 605 Lap, 613-615 time:/prev:/best:, 616 RESET,
  617 WRONG WAY, 623 km/h, 624 mph, 800 New lap record.
- Layout on 640x480, single-player safe area L/R = 52, T/B = 36 in from the edges:
  | element | pos | font / flags | colour |
  |---|---|---|---|
  | "Lap" | R-4, T | 0, right | 0xff80d0ff |
  | lap "%d[F:0] / %d" (min(lap, laps) / laps, "--" for 0) | R-4, T+10 | 1, 0x1001 | white |
  | time: / prev: / best: | R-114, T+52/66/80 | 0, right | 0xff80d0ff |
  | values MM:SS.hh (current "00:00.00", unset "--:--.--") | R-4, same | 0, right | white / 0xffffc864 x2 |
  | speed "%d" (huddata +0x14) | R-55, B-40 | 1, right | white |
  | "km/h" | R-3, B-28 | 0, right | 0xff80d0ff |
  | "Shield" / bar / "%d" | L, T+42 / L, T+61 (length below) / L+5, T+63 | 0 | fill 0xffffd000, red blink < 0.33 |
  | speedo overlay | R-167, B-166, 192x96 | | |
  | "Turbo: %.1fs" box | R-162, B-57, 162x20; text R-146, B-56 | 0 | |
  | minimap | L+4, B-232, 80x160, +0x214 = 68, x +50 | | |
- huddata (craft +0xa30): +0xc turbo secs, +0x14 km/h = sum of last 4 forward speeds * 0.9, +0x18 =
  (class+1) * throttle / 6 * 0.5 (+ turbo), +0x38 best, +0x3c prev, +0x44 shield %, +0x48 shield value, +0x70 shield
  flash (weapon object +0x94, "Shield line" below).
- Speedo: centre (R-71, B-70). Turbo fan (speedo_bars, 0xe040ff40): centre uv (0.5, 1) + 6 points at
  225 - k*f deg (k = 0..1 by 0.2), p = c + 96 (cos+sin, cos-sin), uv = (0.5 + 0.5 (cos+sin), 1 + cos - sin);
  f = turbo T: <5 T*14, <10 70+(T-5)*7, <15 105+(T-10)*3.5, <20 122.5+(T-15)*2.5, <25 135+(T-20)*2.22, else 144.1.
  Needles (hud_draw_rotated_quad, pivot R-69.5, B-70, angle 270 - a deg, quad along (sin, cos), V 0 at the tip):
  len 96 w 24 a = clamp(km/h * 0.12008, 0, 144.1); len 64 w 16 a -> clamp(+0x18 * 1.441 + jitter) smoothed x0.2/frame.
  Needle 2 (0x4ad71..0x4ae5e, single player only): g = huddata +0x18; target = g x 1.441 + (r1 / 255) g x 0.1 - (r2 /
  255) g x 0.1, r1 / r2 = rand() & 0xff (the CRT LCG 0xf96ea: seed x 0x343fd + 0x269ec3, bits 16..30; shared with
  the rest of the game, so its sequence is not reproducible), two fresh draws every time the HUD is drawn; clamped to
  0..144.1, then +0x8940 += (target - +0x8940) x 0.2. So the wobble is a triangular noise of at most +-0.1 g degrees
  (+-6.9 % of the needle's angle), redrawn each frame and low-passed by the 0.2 step; it grows with the throttle gauge
  (class and throttle, + turbo) and is 0 at idle. Ported: hud\speedometer_rules.hpp (argon draws from its own
  generator, std::minstd_rand; its smoothing is the same 0.2 per 60 Hz frame scaled by dt).
- Minimap (data\hud\minimap\<t>.dds): UV extent su = 0.125, sv = 0.125 * 228/96; UV centre = (pos.x / 9.9 + 256) / 512,
  (pos.z / 9.9 + 256) / 512 (11.88 on Fuusoku and alt_fuusoku, track ids 2 / 0xa at 0x52ccf8, 0x4f145; checked: AI
  nodes land on the drawn track). The map is picked by track id, not by folder (gamehud_setup 0x4725e, only with one
  player, 0x52cd8c == 1; jump table 0x49318, hud\minimap_rules.hpp): only alt_silver, alt_bracken and alt_mount have
  their own map; alt_kalau, alt_fuusoku, alt_flames, alt_solar and alt_draco show their first route's, and every id
  past 0xf (Mars, alt_mars) shows mars.dds (0x47362). A lookup by folder name left those six second routes without a
  map: the nemesis races (the home track's other route) and most of a nemesis character's tournament. heading h = 180 +
  atan2(fwd.x, fwd.z), a0 = -atan2(su, sv); corner angles h - a0 + 90 - a0, h + 90, h + 270, h + 270 - 2 a0 with
  uv = (cos*hv + sin*hu + uc, cos*hu - sin*hv + vc); screen quad x L+54..L+134, y B-164..B-4, centre (L+94, B-84);
  colours top 0xb0ffffff, bottom 0x00ffffff, centre 0xffffffff, tris (0,1,2)(2,1,3)(0,1,4) (the last gives the
  bright view cone). Other crafts: 12 px icons (0xcbc40) clipped 12 px inside; position list with 36x36 portraits.
- Panels (hud_draw_panel 0xc1b40, used by hud_draw_bar 0xc24d0 and the turbo box): (x, y, w, h, highlight, glow,
  line colour, fill, fill colour, scale) in 640 x 480 units. Hexagon with the top-left and bottom-right corners cut by 8
  (8 - w if narrower): (x, y+8) (x+8, y) (x+w, y) (x+w, y+h-8) (x+w-8, y+h) (x, y+h), fan (0,1,2)(0,2,3)(0,3,5)(5,3,4).
  1. fill: untextured, alpha blend, corners 0..2 = the fill colour argument (EBX), corners 3..5 = the caller's EAX
     (EDI, 0xc1fb0; the decompiler shows only corner 5);
  2. glow: ring 6 out, inner = line colour RGB / 2 (highlight) or / 8, outer black (vertex data from 0xc1600);
  3. overlay: textures data\hud\feplasma0 / 1 / 2 / 0 on stages 0..3 (loaded 0xc1600), colour ops MOD2X / MODULATE /
     MODULATE / MOD2X, alpha MODULATE x4; corner colours 0x8080ff, (x, y+8) and (x+8, y) white, the two bottom-right
     corners 0xff8000, alpha 0x70 (0xb0 highlighted); uv (0xc16f0, the animation): s = screen pos x 0.005 x scale,
     t = (global_frame_counter_2 % 1000) / 1000 (the vertical blank callback 0xb2670 counts it, 60 a second):
     stage 0 (sx, sy + t), 1 (sx + t, sy), 2 (-t, sy), 3 (sx + t, sy + t);
  4. outline: antialiased 1 px lines, line colour if highlighted else black.
  Shield bar (hud_draw_bar 0xc24d0, called at 0x4b231: (+0x44, x [HUD +0x193a4], y, len, 20, reverse 0), CL the
  highlight, EBX the fill colour, EAX 0): 0xc1b40(x, y, len, 20, CL, CL, 0xffffff00, fill 1, EAX = 0, 1.0), the back
  panel; then 0xc1b40(x, y, +0x44 x 0.01 x len, 20, 0, 0, 0xffffff00, 1, EBX, 1.0) with EAX = EBX (0xc2556), the fill:
  all six corners the fill colour, a solid bar (argon faded corners 3..5 to clear before), never highlighted (the
  reverse flag, 0 here, would right-align it). CL = craft_shield_weapon_up 0x6aba0 (0x4b182..0x4b1b0, on huddata +0x22c's
  craft in 0x596dfc, its weapon object +0x854): one of the object's live weapons (count [+0x90], entries 0x1730 apart
  from +0xc4, live byte at entry - 4) is a class 2 record (+8 == 2: the shields, TSHIELD / 1TSHIELD / 2TSHIELD) whose
  shield bonus channel 0 (record +0x90, the shield-damage resistance it adds) is above 0.0 (0x1a3440): the same test
  that blocks hits ([weapons/README.md](../weapons/README.md) "Damage"); argon: Weapons::shieldGuard(slot).blocked. With CL set the back panel takes
  the front end's selected-row look (0xc1b40 has no timing of its own for it): the glow ring (only drawn with glow on)
  at the line colour / 2 (0x7f7f00) instead of / 8, the plasma overlay at alpha 0xb0 instead of 0x70, and a yellow
  (0xffffff00) outline instead of black; the overlay's scroll (0xc16f0) runs all the time. So while a shield is up the
  whole bar frame glows yellow and brightens, as a selected menu row (which also fills 0xffa08040), and drops back when
  its life ends. Ported: argon's App::drawPanel, shared with the front end. Fill: EBX 0xffffd000, red blink
  < 0.33 %. Length (0x4a100 at 0x4a9xx): S = upgrade (0x52cd24[slot]) x 20 + 100 / ((100 - pilot armour) x 0.01) - 100,
  i.e. the shield size (App::shieldSize) - 100; single player len = 90 + S x 100 / 180 (Tara 105: 92.8), split screen
  70 + S x 70 / 180; y = T + 61 (language 2: T + 79, the bar +3 more; "Japanese layout" below). Turbo box: a panel
  162 x 20 at (R-162, B-57), no fill.
  Shield line (0x4b0ce): single player T + 61, split screen B - 32; render draws "Shield" (601) itself at line - 19,
  the bar (0x4b1f7) at the line, the value "%d" (0x4b268) at (L + 5, line + 2).
  Fill colour (0x4b1b4..0x4b1f4, EDX; esi = huddata = the HUD's +0x5588): 0xffffd000; red 0xffff0000 when huddata +0x44
  < 0.33 (0x1a3640) and (vblanks & 0xf) >= 8; then white 0xffffffff when huddata +0x70 > 0 and (vblanks & 0xf) < 8.
  vblanks = 0x439df0, +1 in the vertical blank callback 0xb2670 (60 a second), so both blinks have a 16-blank period
  (0.267 s, 3.75 Hz) and use opposite halves: a flash alone alternates white / yellow, a flash on a low shield white /
  red. Note: +0x44 is the shield in % (0x6c049: weapon object +0), so the game's red test reads under 0.33 %, not 33 %
  (Shield Low scales by 0.01 first). Compared on a real Xbox: the bar does not blink red on a low shield (only "Shield
  Low" shows, under 33 %: logic/race_warnings.hpp), so argon compares with 0.33 as the game does.
  huddata +0x70 = the craft weapon object's (+0x854) shield flash +0x94, copied each frame by 0x6bd70 (0x6bdd8, called
  by craft_race_info 0x5ce70 at 0x5d51c with edx = craft +0xa30; it fills +0x44..+0x108 from the weapon object). The
  +0x94 writers ([weapons/hud.md](../weapons/hud.md) "Flashes"): 0x6d400(amount) += amount capped at 1 (0x1a3454), every craft; 0x6d430 the same
  plus +0x98, human slots; weapon_damage 0x6b19a calls one of them with 0.8 when the hit's shield damage is > 0 (0x6b1a1
  human / 0x6b1a8 AI); the corrode adds 0.2 (0x6d332 / 0x6d33b) each frame its time is running, the race is on
  (0x52af9f clear) and the shield is above 0; craft_weapon_update fades it by dt to 0 (0x6d372..0x6d3a1, after the
  corrode and the knocked-out early-out); the weapon object reset 0x6a862 zeroes it; 0x6b999 (the SHIELD bone draw)
  zeroes it once at or below 0. So a hit blinks the fill for 0.8 s (two quick hits up to 1 s), a corrode for its whole
  time plus up to 1 s. Ported: hud\shield_bar_rules.hpp (tests\shield_bar_rules_tests.cpp), fed by `CraftWeapon::shieldFlashLevel`
  (raised in Weapons::flashShieldHit and CraftWeapon::corrode, faded in Weapons::updateCraftEffects); argon counts the
  blanks from ImGui::GetTime() x 60 (it used the render frame count, too fast above 60 Hz).
- Japanese layout: the language 0x5293b0 (XGetLanguage, 2 = Japanese) moves these (every other language the
  English values; ported as hud\hud_language_layout.hpp, from argon's language setting):
  | what | English | Japanese | where |
  |---|---|---|---|
  | time / prev / best rows | T + 52, 66, 80 | T + 52, 71, 90 | layout 0x4578a |
  | score value | T + 13 | T + 15 | layout 0x456f5 |
  | speed value | (R - 55, B - 40) | (R - 58, B - 36) | layout 0x455f0 |
  | km/h / mph | B - 28 | B - 26 | layout 0x454aa |
  | shield line (single player) | T + 61 | T + 79 | render 0x4b0ce |
  | shield bar below the line | 0 | 3 (split screen too) | render 0x4b1f7 |
  | shield value below the line | 2 | 3.5 (split screen too) | render 0x4b268 |
  | "Turbo: %.1fs" below its box's top | 1 | 0.5 | render 0x4aa1e |
  | split "%.1fs" below its box's top | 1 | 3.5 | render 0x4ab6e / 0x4ac84 |
  The split "Turbo" label (R - 4, box - 19) and the boxes themselves do not move. Argon's split "%.1fs" now sits
  at the game's box + 1 (it was + 2).
  Ported with a custom ImGui pipeline (shaders\hudpanel.frag rebuilds the four uv sets from the screen position).
- Score (single player only): "Score" (race string 606) font 0 0xff80d0ff at (L, T), value "%d" font 1 white at
  (L, T + 13) (layout 0x456ba; +2 for language 2, ported). The HUD keeps shown (+0x5598) and target (+0x559c) points: an event of
  type 4 (class 0x1a1d64, posted by award_points 0x83f00 with kind / points) adds its points to the target and shows a
  popup (0x4cb00, not ported); every frame shown += max((target - shown) / 4, 100), capped at the target (0x496f0).
  Point values from config\cashmodifiers.txt into 0x1bc4c0.. in file order: pickup_bonus 250 (power-up, kind 9,
  0x83e70), megaup_bonus 1000 (kind 10, 0x83ed0), powerchip_bonus 5000 (kind 8, 0x83ea0), weaponhit_bonus 75 per shield
  point a hit takes (kind 0, 0x83f90; [weapons/README.md](../weapons/README.md) "Hits"), kills_bonus 25000 (kind 2, doubled kind 11, 0x83e00),
  lap_time_bonus 10: a lap under the trackgoals.txt <track>_lap_times goal for the speed class earns 10 x (goal - lap) x 100 (kind 4, 0x83d80), air_time_bonus 20. AI crafts score nothing.
- Air time (craft_hover end, 0x5f717, player only, on landing): t = craft +0x938 - 1 s; t > 0 -> int(t x 100) x
  air_time_bonus points, kind 6; +0x938 reset. (Argon counts +0x938 as the time without hover contacts or water.)
- Points popup (element +0x9380 at (centre, T + 120), scale 0.5625; add 0x4cb00, update 0x4cc30): queue of up to 32
  (kind, points, extra); 0 points only for kinds 12..15; the same kind adds to its entry (if it is showing and older
  than 0.4 s it restarts at 0.4); the first entry shows for 0.8 s, then the next. a = age x 1.25: points "%d" (blank
  for 0) 0xffff00 at y - 150 a, scale (7, 6) a s; label (race string 0x1b9748[kind]: 1230 Attack, 1231 AWESOME ATTACK,
  1232 Killed, 1233 AWESOME KILL, 1234 Great Lap, 1235 AWESOME LAP, 1236 Airtime, 1237 AWESOME AIRTIME, 1238 Points
  Bonus, 1239 Power-up Bonus, 1240 MEGA-UP BONUS, 1241 Nemesis Kill, 1317 Turbo Up, 1318 Shield Low, 1332 Opponent
  Shielded, 1242 Hit by; kinds 2 / 15 add the pilot name, infotext 400 + pilot, "%s %s": "<pilot> Killed", "Hit by
  <pilot>" (0x4cd62, ported: GameHud::popupLabel); kinds 1, 3, 5 and 7, the AWESOME ones, are never posted:
  see "Bonus kinds") 0x00ff00 at y + 30 s - 50 a, scale (3.5, 3) a s; both alpha
  255 (1 - a); for a > 0.5 a copy: points at y - 75 scale (3.5, 3) s, label at y + 30 s - 25 scale (1.75, 1.5) s, alpha
  255 (1 - clamp(4 a - 3)). Centred, font 0.
- Needles (0x4ad6c / 0x4ae5e, colour in EAX): the speed needle white, the second (throttle) needle 0xffff0000.
- Glyph boxes in the .map are 4 taller than the font line (23 / 19, 39 / 35; ink in box rows 3..17 / 7..31): argon
  centres each box on the line (2 up), which puts the panel texts where the game shows them (centred in the 20-high
  shield / turbo panels at y + 2 / y + 1). Not confirmed in 0xb1660 (the decompiler times out there).
- Text flags & 0xf000 (font_draw_text 0xb1660 at 0xb2371): 0x1000 centres each glyph's font height in the line's
  tallest font, 0x2000 aligns to its bottom; the lap counter "%d[F:0] / %d" uses 0x1001 (right, centred).
- Other crafts on the minimap (hud_minimap_draw 0x4f090): data\hud\arrow.dds (a "V") via hud_draw_icon 0xcbc40
  (x, y, angle, 12): corners at angle + 225 / 135 / 315 / 45 deg, 12 from the centre, uv (0,0) (1,0) (0,1) (1,1);
  angle = atan2(fwd.x, fwd.z) - map heading; offset = (other - player) rotated by heading - 90 deg x (1/9.9) / 512 x
  map size / UV extent; drawn only 12 px inside the map. Colour (EAX): player white, others 0xffffff00, the player's
  nemesis (pilot id +-8: Tara/Alicia, Yuri/Vladimir, Kumiko/Abi, Darius/Pharaoh, Jordan/Kaitlyn, Marcus/Jaya,
  Leah/Lukas, Kukailimoku/Kutoo; ids from the replay loader 0x31c10) 0xffff8000. Skips crafts with +0x1145 / +0x1129 set.
- Position list (same function, left of the map, element x = L+4): one row per craft in place order (index list
  at the race object +0x244, player index +0x22c), row top r = y + 160 + 68 - crafts x 38 + place x 38:
  black (x-3, r-2)-(x+43, r+39); (x-2, r-1)-(x+42, r+37) yellow 0xffffff00 for the player else 0xff808080;
  (x+9, r)-(x+40, r+35) 0xffb07000 / 0xff303030; the hit's red flash (x+7, r)-(x+43, r+35), red = craft weapon
  +0x98 x 254, additive, drawn twice ([weapons/hud.md](../weapons/hud.md) "Flashes"); portrait (x+7, r)
  36 x 36 from data\hud\tex<table 0x1b93f8[pilot]>.dds (0x793a0), red tint if +0x1145; energy bar: grey
  (x-1, r+1)-(x+6, r+36), red (x+0.5, r+2)-(x+5, r+34-32e), green (x+0.5, r+34-32e)-(x+5, r+34), e = craft
  weapon +0 / 100. Player row: place word (race_strings 607 + place, font 1, white) at (x+56, y+32) and a
  yellow-on-black connector from the row (x+39, r+17) to (x+50, y+48).
- Power-up panel (element +0x3950 at ((L + R) / 2 - 64, T), draw 0x4eb20; huddata +0x74 / +0x78 / +0x7c held levels
  red / blue / yellow, +0x8c special ready): weaponbase (blend mode 1), then in mode 2 (additive) each colour's cap
  (0x52cd0c / 12 / 18[slot], the most this race allows) as weapon<colour><cap> in 0xff404040, then, unless the special
  is ready, each held level as weapon<colour><level> in grey g = min(lround(155 + 100 sin(15 t)), 255) (55..255, period
  0.42 s; t = the clock 0x50e430 + 0x50e434 in s): that is the panel's glow / blink. A ready special draws
  weaponspecial (mode 1, white) instead of all three held levels. Split screen: x + 32, scale 0.5. Ported
  (HudPowerUpPanel; argon's clock is App::simClock).
- Shield Low (also in 0x4eb20): +0x1a7c keeps the shield fraction (huddata +0x44 x 0.01). Dropping under 0.33: the
  music mode one up (0xdec40, modes 3..6; [audio/music.md](../audio/music.md) "Music") and +0x1a80 = now + 1; while under, each time now passes +0x1a80
  (the player's slot not finished 0x597362, no post-race state, 0x52af9f clear) event kind 13 "Shield Low" with 0 points
  (0x83f00), UI sound 0x23 (not ported: which list 0xc3ca0's id 0x23 plays from is not checked) and +0x1a80 = now + 1.
  Back to 0.33 or over: the music mode one down. Ported as argon::ShieldLowWarning (logic\race_warnings.cpp), the music
  steps too (wentLow / recovered). 0xc3ca0 indexes a table of 0x100-byte sound slots (sound object +0x1590 + id x
  0x100) filled in the order the lists load, so 0x23 is not placed yet.
- Messages: RESET (616) and WRONG WAY (617) are text elements +0xa5c4 / +0xa7d0 at (centre, (B - T) / 2 -+ 40) =
  (320, 200) / (320, 280), font 1, centred, 0xffff8000. New lap record (800) is element +0x74c8 at (320, 230), font 1,
  centred, white (gamehud_setup 0x461ee). Their triggers (craft_race_info 0x5ce70, the craft's vtable 0x74 after the
  object update; huddata = craft +0xa30):
  - RESET: respawn 0x5cb30 (when craft +5 is set: back to the restart point 0x597330 + slot x 0x88) sets craft +8 =
    0.5 s (0x1950a0), taking dt off it each step down to 0; 0x5ce70 copies it to huddata +0x60 (0 while 0x52af9f /
    0x52af9c). 0x4c060 shows RESET while +0x60 > 0 with the static sprite +0xa1ac (data\hud\static.dds over the whole
    screen, colour alpha clamp(lround(+0x60 x 255)) so at most 128, uv offset (+0x1fc, +0x200) = (rand & 0xfff) /
    4095 each frame over an extent of 1, draw 0xab9b0).
  - WRONG WAY: while +0xa90 (the reset time) is 0 and the craft not dead (+0x1145): d = normalize(next node - current
    node) (race slot +0 at 0x5972f0, nodes 0x1b9bc8, next 0xebc11); forward (+0x250) . d <= -0.4 adds dt to +0x8e0,
    above clears it; +0x8e0 > 1 s sets +0xa73 (huddata +0x43), else 0. No speed condition. gamehud_update 0x49f54 shows
    it steady (no blink) while +0x43 is set, the race is not over (0x52af9f), no controller message (+0x70ac) and in
    split screen the craft is in play (+0x11e).
  - New lap record: only CGameHudTimeAttack (vtable 0x1a0ba8, ctor 0x4c7d0, update 0x4c850) shows +0x74c8: huddata
    +0x42 (Time Attack's lap end 0x429c0 copies +0xd32c: this lap beat the race's best +0xd338) and the next lap's time
    +0x34 x 4 < 10 (its first 2.5 s) and frac(+0x34 x 4) > 0.4 (on 60 % of each quarter second), the craft not dead,
    0x52afac / 0x52af9f / the post-race state clear; then 0xc3400 plays sound 10 at most every 5 s
    (SF_GENERIC_FASTESTLAP `newlap` by its name; App::playLapRecordSound). The
    plain CGameHud never shows it, so a Quick Race / Tournament lap record is silent on the HUD.
  Ported: argon::RaceWarnings (logic\race_warnings.cpp, stepped with the player's craft at 60 Hz), the static overlay
  (HudStaticOverlay), GameHud::lapRecordShown (RaceLapRecords::recordOnLap). The respawns follow the game's reset request
  (Physics, "Reset request"); the craft never dies (+0x1145), so the dead conditions are the energy > 0.
- Bonus kinds (who posts which): award_points 0x83f00 takes the kind from the stack and every one of its 11 callers
  pushes an immediate: 0 Attack (weapon hit 0x83f90 at 0x83ff0; its "awesome" flag only doubles the points, the kind
  stays 0), 2 Killed (0x83e58), 4 Great Lap (0x83de4), 6 Airtime (0x5f7a1), 8 / 9 / 10 pickups (0x83ec4 / 0x83e94 /
  0x83ef4), 11 Nemesis Kill (0x83e36, double points), 13 Shield Low (0x4ed69), 14 Opponent Shielded (0x84000), 15 Hit
  by (0x84051). The event class 0x1a1d64 is built in only one other place, 0x3d666, which writes kind 12 (Turbo Up).
  The HUD's handler (0x497a4) and the popup add 0x4cb00 pass the kind through unchanged (0x4cb49 only looks up its
  label). So nothing gives kinds 1 (AWESOME ATTACK), 3 (AWESOME KILL), 5 (AWESOME LAP) or 7 (AWESOME AIRTIME): their
  labels (race strings 1231 / 1233 / 1235 / 1237) are left over, and config\cashmodifiers.txt has no awesome bonus. The
  argon's App::awardPoints callers post the same kinds (none of 1 / 3 / 5 / 7); nothing to port.
