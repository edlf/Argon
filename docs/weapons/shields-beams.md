# Weapons: shields, volleys and beams

## Shields (class 2: launch 0x6a390, update 0x69970)
- Up for the record's life (+0x140): blocks every hit (resistance 100, 0x6aba0), level 2 (1TSHIELD, flags 4) absorbs
  (the hit's shield damage, 0 behind the 100 resistance), level 3 (2TSHIELD, flags 0x10) retaliates with RETALIATOR.
- Launch: the owner gains 10 / 20 / 30 points (slots 1 / 4 / 7; 5 / 10 / 15 for craft 1 in a nemesis race, 0x6a439) and its
  running effects are cleared (+0x8, +0xc, +0x10, +0x14, +0x18, +0x20, +0x24; +0x1c is missed, +0x20 written twice).
- Draw: its bone "<name>0" with the colour fade +0x1a0..+0x1ac above: 0x69970 writes the colour (alpha 0xff, the
  RGB blended linearly, rounded by fistp) to the bone tint +0x104 of the bone and its subtree (0xb9930); the tint only
  shows on lit meshes (0xba1d0). TSHIELD: grey 0x74..0x9d to black over the last 0.3 s of its life.
- Animation: 0x69970 also sets the bone clock +0x188 of the shield's subtree to its age (0xb9e50); bones with their own
  clock (+0x102 flag 0x80, 0xb9f70) sample their track ([models/meshes.md](../models/meshes.md) "Animation") at it. 1TSHIELD0 is a child of
  2TSHIELD0, and 1SHIELD_TARA0 (every craft's only shield track) a child of 1TSHIELD0, so it plays on level 2 and 3
  shields.
- The own clock (0x656b9 / 0x65722: flag 0x80 on the subtree, clock 0) is the shield launch's (class 2) only. A
  projectile's bone is only detached (0x65124), so its tracked children (Abi's XSTAR0..3 under STAR0..3, Leah's
  AVALANCHE0..7 under A0..7, Kukai's sphere4 under TERRABALL0, Kumiko's xx1DATABOMB0 / XXX under 1DATABOMB0 / 1) play
  their tracks on the ship model's clock, the race clock (0xb843f in the ship's draw 0x59f6f); the detached bone's own
  track (Kukai's TERRABALL1 / 2) is not used. Argon: SceneRecorder::drawShots poses the ship at the scene clock (the
  pickups' stand-in for the race clock) under the shot's matrix.

## Volleys (0x6a390, 0x69e30)
- A shot sets the slot object active with the time; projectile k launches once now - time >= k x +0x170. Repeating
  records (+0x13c) relaunch projectile k (the old one done first, 0x65e60) every +0x168 x +0x170 s until +0x144 s after
  the shot. The slot stays active (blocking that colour) while a projectile is out.

## Beams (class 1, update 0x68620)
- Start: the projectile's bone on the ship (0xb9c00, follows the ship). End: the projectile's target (+0x370[k]; the
  target gets +0xb4 locked and +0xb5 in danger), else start + the craft's forward (+0x1f0) x 1000 + a per-projectile
  offset (+0x90 + 12 k).
- Ray start -> end (0xd4770): the world stops it; then the craft hit test along it (0x683b0), radius 5 (excluding the
  shooter): the end moves to the hit point, on the beam as far along it as the craft's centre (start + d x (p . d) /
  |d|^2), not the centre. With no wall in the way and no craft found, a locked target (+0x370) counts as hit (the end
  is its centre already). Only on the projectile's first frame (+0x28, set at launch, cleared after each update
  0x69f7c): sparks (0x68150), the hit (0x6acf0 at the hit point, flag 0) and the target's rumble +0x9dc += 50000.
- Each frame 0x64870 records the segment as a replay entry (kind 1: start, end, +0x3c 2 craft hit / 1 not, +0x3d
  segment count 1, +0x3f the grey k); 0x641f0 plays it into +0x650 (count), +0x654 / +0x6cc (starts / ends), +0x834
  (kinds), +0x85c (0xff000000 + k x 0x10101) per projectile (stride 0x210), which the race-over branch draws.
- With bounces (+0x165, none in the data) the beam reflects off the surface up to +0x16c segments of 1000.
- Grey k = clamp(fistp(255 x (1 - clamp(age / life, 0, 1)))): the bone tint +0x104 (k, k, k, 255) of the beam's bone
  for every player's model (0xb9930; lit meshes only; an additive mesh fades to black), not an alpha. Drawn as its bone stretched from start to end (a
  look-at frame, z x length; the beam meshes span z 0..1), lifetime +0x140 (0.1 .. 1 s).

## Beam and spark flashes (0x68620, sparks 0x68150 -> 0xbd670)
One-frame quads of the additive sprite pool ([effects/sprites.md](../effects/sprites.md) "Sprite pool record": pool 0, EAX 2 camera facing,
quadsprite.dds cell (256, 256) 128 px, the angle rand & 0x1ff degrees in 0xbd670's own slots, 0x41fa28 = 1 so 0xbd5f0
frees them before the next frame). The pool is drawn in every view, so they show in split screen. Only beams (LASER
and the other class 1 records) make them; the sparks 0x68150 have no other caller.
- Start glow (0x68df5): every frame, at the start (the beam's bone on the ship), half size 2.5, grey k (the beam's).
- Sparks (0x68150(count EAX, weapon, at, system, strength, projectile)): first its count particles, then its flash
  at `at`, half size 10 + (rand & 3) (the angle's rand first), grey clamp(fistp(strength x 255)), strength = 1 - age /
  life (so k), then, on the first frame only (+0x28), its 0x9d000 sprite. Called on a craft hit on the first frame (4
  particles, system 0 starburst) and every frame the world stops the beam with no craft before it (1 particle,
  system 1 starburstred, at the wall point, collision +0x81cc).
  - Particles (0x68165..0x68279, neon_add_particle 0xbcb60 with ESI = position, EDI = velocity): three rand() per
    particle, velocity per axis ((rand & 0xff) x 1/255 (0x1a3548) - 0.5) x 10 (0x1a35a4), x 10 again unless system 1
    (so +-5 m/s starburstred, +-50 starburst; system 2 would be bubble), position = at + velocity x 0.5; colour from
    0xffffffff to 0, life 1, hold 0.8 (0x3f4ccccd), mode 0 (it falls under the system's gravity, -10 / -13).
  - Sprite (0x68335..0x683a0): 0x9d000 with EAX 10 (quadsprite.dds cell 10, (256, 256) as the flashes), velocity the
    last particle's, life 1, from 0xffffffff to 0, fade 0, half size ((rand & 3) + 10) x 0.25 (2.5..3.25), growth 1,
    start angle rand & 0x1ff, spin rand & 0x1ff degrees per second, list B additive (arg 10 = 1), no fade-in; the
    rand order is spin, start angle, size. It is a trail sprite ("Trail sprites"): it lives 1 s, blending white to
    black, unlike the one-frame flashes.
- Hit flash (0x68f0a / 0x69171): a craft hit on the first frame, at the hit point, half size 25, white 0xffffffff,
  after the sparks. Then the hit (0x6acf0), the rumble and the beam's own 0x9d000 sprite (0x68fde / 0x69245): as the
  sparks' but still (velocity 0) and half size ((rand & 3) + 20) x 0.25 (5..5.75). The race-over branch makes it on
  kind 2 too, after its hit flash (0x688bb). A hit after the first frame makes no sparks, no flash and no sprite.
- Race over (0x52af9f, the weapons then follow the replay's entries; the projectiles take 0x66b40): the branch at
  0x6866d draws the recorded segments: a glow (half size 2.5, +0x85c) only at bounce points (segment k > 0, 0x687cf),
  so no start glow; on the first frame per segment kind 1 the sparks (1 particle, strength +0x85c's blue / 255) at the
  segment's end (even 1000 m out), kind 2 the sparks (4) and the hit flash (0x688bb).
- Argon: logic\weapon_flash.hpp (which flashes and sprites by BeamFrame, the sizes, the grey, the sparks' particles
  and their system, the two 0x9d000 sprites, the hit point; tests\weapon_flash_tests.cpp), Weapons::stepBeam / castBeam
  (weapon_shots.cpp), App::flashBeam / throwSparks (app_trail_sprites.cpp: the flashes drawn after the additive trail
  sprites, cleared at each weapon step; the sparks into the particle systems, the sprites into the trail sprites);
  the beam's grey as its bone tint (SceneRecorder::beamTint, the shields' path: lit meshes only); a replay plays as
  the race-over branch. Not ported: the bounce glows (no bounces in the data), the locked-target fallback (the
  argon's craft test reaches the target at the end anyway), freeing them while paused (argon keeps the last
  step's flashes), 0x9d000's own rand() for its pool quad's angle (unused by a trail sprite), the tint written to every
  player's model (argon draws one beam).
