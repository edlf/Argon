# Weapons: argon's port

## Argon port (weapons.cpp)
- Fire: as the game (below, "Fire buttons"): pad B red, X blue, Y yellow, held, one colour a frame in that order;
  the keyboard's 1 / 3 / 2 stand in for B / X / Y (App::heldFireButtons). A held special takes any of them. Works for
  slot 0 whether you or the AI drive. `--autofire` fires every colour whenever armed (tests).
- The special (Weapons::fireWeapon -> fireSpecialsShield, logic\weapon_rules.cpp: levelsAfterFiring,
  levelsForSpecialsShield, volleyTargets, raisePowerLevels; tests\weapon_rules_tests.cpp): fired on any request, its
  projectiles aimed as weapon_select aims them (the ones beyond the locks at the nearest lock), then the level 3
  shield; all levels empty. Nothing (no yellow 3 either) while the craft is out or the race has no weapons.
- Tests: `--powerups r b y` sets the levels at race start; with `--autofire` they all fire at once (0.02 s), so a
  TSHIELD (3 s) / 1TSHIELD / 2TSHIELD (7 s) is gone in a later `--shot` and the power-up panel is empty (the levels
  emptied by the shots). `--special-at <s>`: the player holds a special (levels 3 / 3 / 3, a mega-up's grant at
  Weapon Strength 7) s seconds into the race. With `--autofire` the stderr log shows each fire, impact (`impact:`
  shooter, projectile, kind, target) and hit (`hit:` shield lost and left, in %). Checked: `--quick-race 1 --race-go
  --ai-race --autofire --special-at 10 --nopanel --drive-time 14 --shot <png>` (run before 2026-11-22 without `--menu`: the editor's bracken drive, see [argon/editor.md](../argon/editor.md) "Command line") (Abi on Bracken: SPECIAL at 10.00 s,
  2TSHIELD at once, the 8 explosions locked on slots 2 / 5 / 4 / 1 and the rest on the nearest, slot 2 knocked out
  at 11.35 s; the shot: the shield dome on the ship, the shield bar highlighted). On the grid at the race start (the
  player in front) nothing is ahead to lock, so the projectiles fly straight.
- Weapon models (0x64d20: bone index = 0xb9c40("%s%d" % (record name, projectile k))): projectile k is the bone
  "<name><k>" of the firing ship's model (Abi: STAR0..3, SHURIKEN0..7, TSHIELD0, 1TSHIELD0, RETALIATOR0, SPECIAL0..7
  ...); it starts where that bone sits on the ship (ship matrix x the bone's rest matrix, minus forward x +0x100) and
  draws its bone and children at its own matrix (right / up / forward kept square to the flight direction). An active
  TSHIELD draws its "<name>0" bone on the ship. The bones stay hidden on the ship otherwise (as in the game).
- Projectiles fly as decoded above with their trail particles and sprites (app_trail_sprites.cpp: the spray's
  texture, the alpha list with the spray's pipeline, the additive list its own), launched as volleys (stagger,
  repeats). Beams as decoded (no bounces, no grey tint, no per-projectile offset; the end at the hit point along the
  beam) with their flashes ("Beam and spark flashes"). Shields: resistances, blocked, absorb, retaliate, the launch recharge, clearing effects, the colour fade
  (the bone tint of lit meshes), the bones' animation at the shield's age (Cmm::pose, SceneRecorder::drawShield) and
  the sphere (Weapons::updateShieldSpheres, its impact's bursts).
- Hits (weapon_hits.cpp): the shield damage and the leech as decoded, area blasts from the impact bursts, the
  single-player mercy, the Infinite Shield cheat. Scoring (0x6b0d3, while the race runs): the player's hit earns
  weaponhit_bonus x int(float(shield size x % lost x 0.01)) ("Attack", kind 0; 0x83f90 with 0x64070), the % before the
  nemesis race's halving (0x6b135 doubles it back) and doubled again on the player's nemesis (pilot id +- 8, the
  leftover "awesome" flag); a hit taking no shield posts "Opponent Shielded" (kind 14, 0 points); another craft's hit on
  the player posts "Hit by" + its pilot (kind 15, 0x84010); the player's knock-out earns kills_bonus ("Killed" after the
  target's pilot, kind 2), on the nemesis 2 x kills_bonus ("Nemesis Kill", kind 11, 0x83e00). Energy (absolute: % x the shield size) drives the HUD shield bar and
  the position list's energy bars.
- Knock-out: the craft stops (an empty pad), out of hits, locks, pickups and firing, put at the back of the result
  order (Race::knockOut) and into the track's NET_CAGE (GameCraft::retire: no respawn). The player's: the death
  camera, the messages, LOSEMENU after 10 s or A (app_knockout.cpp), the explosions (Weapons::startKnockOutExplosions).
  Test: `--knockout-at <s>` (slot 1 knocks the player out). NET_GLOW is hidden while racing until a craft is out
  (App::hiddenNetGlowBone). Not ported: the levels / cash kept for Race Again, split screen.
- Effects (logic\weapon_effects.cpp, Weapons::afflict / updateCraftEffects, GameCraft::setWeaponEffects): as decoded
  above, with the nemesis race's halving (Weapons::nemesisShareOf); the engine off / on sounds for the player's craft
  ([audio/ships.md](../audio/ships.md) "Ships"), the hit's red flash on the position list ("Flashes"). Not ported: those sounds for the other
  crafts (no positional ship sounds). The corrode's flash needs nothing: no craft has the SHIELD bone it lights.
- AI: drones collect power-ups into their own levels (caps and shield level by speed class and kind,
  Weapons::aiWeaponSetupOf; the track id is the race's loadTrackId) and fire as decoded, from the armed record; a held
  special fires on any request with the level 3 yellow after it (Weapons::fireWeapon, the player too). The nemesis drone
  aims at the player only. Locked on (0x6e1e0) = in the player's lock lists or a shot's target.
- While a class 2 weapon (TSHIELD) with a shield-damage resistance (record +0x90 > 0.0) is up (craft_shield_weapon_up
  0x6aba0; argon: Weapons::shieldGuard(slot).blocked) the HUD shield bar's back panel is highlighted and glows as a
  selected menu row ([physics/hud.md](../physics/hud.md) "HUD", "Shield bar").
- Particles: the game's three systems (starburst / starburstred / bubble: point size 0.6 / 0.5 / 0.35, gravity -10 /
  -13 / 0) with from -> to colours after 80 % of the life.

## Not decoded / not ported yet
- Racer slot +0x74 (> 2.7 / 3 for an AI target) and +0x1c; what +0x6a / +0x6b do beyond DNF; what freeze looks like
  (if anything beyond cutting the throttle).
- The HUD's attack / kill popups; the explosion types' +0x10 / +0x14 (no code reads them).
