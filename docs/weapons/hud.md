# Weapons: flashes, lock-on and warnings

## Flashes (0x6d400 / 0x6d430, decay 0x6d372, draw 0x6b950 / HUD 0x4fafe)
- 0x6d400(amount): weapon object +0x94 += amount, capped at 1; 0x6d430 (human crafts) also +0x98. A hit with shield
  damage adds 0.8 (0x6b1a1 human / 0x6b1a8 AI); the corrode 0.2 a frame (0x6d33b, every craft, 0x6d400 only). Both
  fall by dt to 0 in craft_weapon_update (0x6d372).
- +0x94 (0x6b950, each frame and viewport, also after a pickup 0x6bc78): the craft's "SHIELD" bone (craft loader
  0x5071f, +0x30 of the craft's model record) is shown while +0x94 > 0, its first mesh's surface colour set to grey
  255 x +0x94 (alpha 255, surface flag 0x20); at 0 it is hidden. No craft model has a SHIELD bone (the index is -1 and
  0x6b950 returns at once), so on the craft this flash, the corrode's only one, never shows; it does on the HUD:
  0x6bd70 (0x6bdd8) copies it to huddata +0x70 and the shield fill blinks white while it is above 0 ([physics/README.md](../physics/README.md) HUD,
  "Shield line"). Argon: `CraftWeapon::shieldFlashLevel` (logic\craft_weapon.hpp).
- +0x98 (human only; only hits feed it): the hit's red flash. weapon_damage (0x6b18a..0x6b1a8) calls 0x6d430 when
  the hit's shield damage (out 0, after resistances and absorb / block; the nemesis halving keeps its sign) is > 0 and
  the target's craft index < 0x52cd8c (the human players), else 0x6d400; the Infinite Shield cheat returns before it,
  0x52af9f does not. 0.8 a hit, so one hit lasts 0.8 s and two close hits stack to 1 (1 s). The decay
  (0x6d3a7, -= the frame's dt to 0) sits in craft_weapon_update after its knocked-out early-out (+0x1145, 0x6d030 ->
  0x6d3dc): a knock-out keeps its flash; paused, the update does not run and the flash holds.
- Drawn (hud_minimap_draw 0x4f090, the position list, at 0x4faf8..0x4fb96) on the row of the craft the row shows
  (edi = the row's craft, +0x854 its weapon object), so in split screen every view's list shows each human's flash:
  after the row's frame and panel, before its portrait (0xcc050 at (x+7, r), 36 x 36, whose transparent parts show
  it): blend mode 2 (additive, 0xa9530), colour ARGB 0xffRR0000 with RR = fistp(+0x98 x 254) (0x1a37b8 = 254), the
  rectangle (x+7, r)-(x+43, r+35) (0x1a37b4 = 7, 0x1a37cc = 43, r+35 the panel's bottom) drawn twice (0xaad40 twice,
  the same arguments), then blend mode 1. Not full screen: only the row's portrait area (the frame's edge x+7..x+9,
  the panel and the 3 px right of it). Where the list is not drawn (one racer, Time Attack) nothing shows.
- Argon: logic\hit_flash.hpp (`HitFlash`: raise / fade / the red byte; tests in weapon_effects_tests.cpp), `CraftWeapon::
  flashPlayerHit` from Weapons::applyHit (the red flash for slots < splitPlayers; `flashShieldHit`, +0x94, for every craft), faded in Weapons::updateCraftEffects (not for knocked-out
  crafts, not while paused), HudRacer::hitFlash -> HudMinimap::drawHitFlash. The AI's +0x94 is not kept (no SHIELD
  bone to light). Checked: `--quick-race 6 --race-go --ai-race --autofire --speed 5 --countdown-at 0 --drive-time 9.5
  --shot <png>` (run before 2026-11-22 without `--menu`: the editor's bracken drive, see [argon/editor.md](../argon/editor.md) "Command line") (slot 0 hit at 9.35 s: its row's panel goes from 0xb07000 to 0xff7400, a red edge right of it).

## Lock-on and crosshairs (0x6c420, HUD 0x4e7f0 / 0x4d9d0)
- weapon_lock_targets 0x6c420 (each frame, every craft): per colour, the lock count = the armed record's +0x168 when
  its +0x150 is set (the special overrides all three), 0 while that colour's shot is out (counts red +0x4c, blue +0x50,
  yellow +0x54). Targets: other crafts still racing, closer than 500 and ahead (direction . forward > 0.85), nearest
  first, max 6 (list +0x60); locked crafts get +0xb4. Lock-on records: every pilot's red weapon, the special and the
  retaliator (Tara ROCKET x1 / x2 / x4, Abi SHURIKEN x2 / x4 / x8 ...).
- Firing (0x6c110) gives projectile k the k-th locked target, so it homes on it after +0x194.
- hud_targets_update 0x4e7f0 (player only): entries {state, craft, x, y, z, time}, 6 per colour (+0x1a84 / +0x1b14 /
  +0x1ba4, stride 0x18). A locked craft keeps its entry, or gets a new one in state 1 (adds 0x4e580 / 0x4e650 / 0x4e720,
  at most one per 0.25 s and colour); state 1 -> 2 after 0.5 s; an entry no longer locked -> state 3, freed 0.25 s later.
- hud_draw_targets 0x4d9d0: at the target's projected position, size clamp(2048 / depth, 16, 64) x scale, spinning
  time x -360 deg/s, drawn as hud_draw_icon (radius size x sqrt 2), additive. State 1: a = 0.5 - age, scale 10a + 1,
  alpha 255 - 510a (shrinks in); state 3: scale 1 + 10 age, alpha 255 - 1020 age (grows out). Textures data\hud\crosshair (red) / crosshair_blue /
  crosshair_yellow. The same function first draws the nemesis marker (the player's nemesis,
  pilot id +- 8, while on screen): yellow corner brackets, s = clamp(2048 / depth x 0.3, 8, 64), box x +- 1.5 s,
  y +- s, arms 0.25 s; the marked craft's pilot name (0x78b00) in font 0 centred at y - (s + 18); both with alpha
  255 - clamp(camera distance - 250, 0, 255). While the race start runs (0x52af9c) the marker is on the craft the grid
  intro shows (0x4db84: crafts - 1 - floor(t / 2), t the intro clock 0x50e458); during the grid intro (0x52af9d) once
  fmod(t, 2) > 0.5: "[FCHAR<id>] <name>" (format 0x1a09c4: the portrait icon tag, 64 x 64, then the name) font 1,
  flags 0x1002, white, x 1.5, at y - 104; if that craft is the player's nemesis, "Nemesis" (race 0x421) font 1,
  centred, 0xffffff00, x 1.5, at y + 24, shown while int(t x 8) & 3 != 0. The icon tags (neon_app_init_load):
  "CHAR<n>" 32 x 32 / "FCHAR<n>" 64 x 64 on the Char Icons textures, slot n = data\hud\tex<0x1b93f8[n]>.dds (0x793a0).
  Argon: GameHud::drawNemesisMarker / drawIntroLabels (hud\markers.cpp), the tags in HudPainter::characterTag
  (app_overlay.cpp). Not ported: the marker following the intro's clock on through the countdown.
- Argon: lockCandidates / PlayerLocks (logic\lock_on.cpp), updateLocks (weapons.cpp) + GameHud::drawCrosshairs
  (hud\markers.cpp), player only. `--powerups r b y` now sets the levels at race start (tests).

## Attack warning (weapon object +0xb5, huddata +0x108, power-up panel 0x4d720)
- In danger: the weapon object's (craft +0x854) +0xb5 = 1, set during the weapons' update by: a homing projectile on
  its target past the homing delay (0x67038, with +0xb4 on the target too and INCOMING 0xc2f90) and a beam's target
  (0x68d1a); every craft but the shooter within 300 m of a projectile (0x66e60, distance squared < 90000 0x1a3a08);
  the craft hit test 0x683b0 (projectile steps and beams): a craft within the hit radius of the path's line, ahead of
  its start and less than 200 m (0x1a35a0) past its end (0x68591).
- 0x6bd70 (each frame, from craft_race_info 0x5ce70) builds huddata +0x108: 0x10 frozen (+0x10 > 0), 8 corroding
  (+0xc), 2 control effects (+0x1c / +0x14 / +0x24), 1 locked on (+0xb4), 0x20 locked on in multiplayer or by
  a craft with +0x8ec, or in danger (+0xb5); then clears the craft's +0xb4 / +0xb5.
- The power-up panel (vtable 0x1a0960, ctor 0x4d540: 5 + 5 sprites at +0x1dc / +0xc18, texts +0x1654 "No Weapon" and
  +0x1860) shows +0x1860 = race string 0x51f "ATTACK WARNING" (0x4d630: font 0, centred (+0x204 = 2), 0xffff4040,
  size 128 x 32) at the panel + (64, 90), split screen + (64, 45), while bit 0x20 is set, unless the player's pad is out
  (0x52abfc), Restart / Track Select runs (0x52abe8 / 0x52abe9), the race is over (0x52afa8) or starting (0x52af9c)
  (0x4d97b). Bits 1..0x10 blink the matching sprite pair red (0xff, a, a >> 5, a >> 6; a = 155 + 100 sin(30 x the
  clock)); otherwise the pairs are 0x40ffffff / 0x60ffffff.
- Argon: logic\attack_warnings.hpp (the flags of the last 60 Hz step), Weapons::warnCraftsNear / warnCraftsOnPath
  (weapon_shots.cpp), GameHud::updateAttackWarning. Not ported: the five sprite pairs and "No Weapon" (their textures
  are not decoded) and the multiplayer lock-on case of bit 0x20.
