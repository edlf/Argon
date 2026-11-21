# Sequences: story scenes, head turns, light

## Story scenes (which, where)
- Before every tournament race (loader_run 0x80090; LOADER +0x1a74 = 2 in mode 0 only, +0x1a70 the scene's flag):
  `data\movies\<t>\` + the table 0x1b4158 [track x 16 + the player's character 0x52cdc0] (64-byte names
  `<Track>_<Name>.nms`, the names Tara Yuri Kumiko Zaki Jordan Marcus Leah Kahekili Alicia Vladimir Ami Pharaoh Kiri
  Jaya Max Kutoo; 0x1b3cd8 the folders; Mars has rows too). The disc has 144 of the 256 route-track rows (each track
  has its own subset). The first pass loads the race, the scene
  and the glory flyby while the loader thread draws only infotext 0x210 "Loading..." in font 1 centred at (320, 256)
  (0x81acb), then switches to the scene's page (0x585740, its path +0x1674, vtable 0x19d858: enter 0x9a810, update
  0x9aa00, input 0x9af10, leave 0x99fb0; its view +0x410: 0x99360 / 0x98c30). When the page ends the second pass
  (0x803b1) shows the loading screen with the glory flyby (Press [BUTA], the Nemesis Challenge intro from its start).
- After winning a tournament's last race (the race page's leave 0x7fa50, 0x7fac4: mode 0, progress -1, level 1..4):
  Amateur / Expert / Master (the last race is the nemesis race on the home's other route) `<t>_<winner>_beatnemesis.nms`
  from 0x1b8958 [track] (the winner = track ^ 8's owner, i.e. the player); Redshift (the last race on the home track)
  `<t>_<pilot>_winning.nms` from 0x1b8dd8 [track]; Novice none; Mars rows "should not be loading this file". Played in
  0x7fa50's own loop over the race's track (0x1b32a0 = 1: no controller message); A (+0x88 & 0x10) or START (0x100)
  on any pad ends it at once; then the front end (FE AWARDS). That loop leaves as soon as the page sets 0x439f78
  (0x7fcb7): no 12-frame hold after the race.
- A missing pre-race file: never happens. The first pass names the file without looking for it (0x80354, +0x1a70 = 1
  unconditionally) and the page's enter would hand seq_start_copy an empty resource; but the 144 (track, pilot) rows
  the disc holds are exactly the 144 that the tournaments race (every level's order, 0x79130, `--tourney-order`).
- The page (0x9a810 / 0x9aa00 / 0x9af10): the loading screen's ui_levelload loop stopped (0xca9b0: its four voices
  +0x964, an earlier note said the music), the track's cut_ambient loaded into sound record 0 (0xca300, by track id)
  and looped on its six speaker voices +0xc1c (0xca540) at the SFX option less 500 mB (0xc7990, clamped to
  -9600..0); the tournament-win loop does the same (0x7fc56 / 0x7fc94); the leave (0x99fb0 -> 0xca5d0) fades
  +0xbf4 and empties the six (audio\README.md "Tracks"). The letterbox on (global_cutscene_bars_enabled:
  0x98c30 fills (-100, -100)..(840, 56) and (-100, 424)..(840, 580) black); it ends once the first track's length
  less 0.1 s has played, or on A / START (the voice stops): 0x439f78 = 1. From then on seq_update is not called (the
  camera, actors and speech time hold; a voice that was not skipped plays on), both text boxes get "" (0x9ac31: the
  subtitle text goes, its panel and portrait stay while 0x1b3c88 is set), and each update counts 0x439f78 up; past 12
  (0x9ae05) the page pops (0xaebd0): 12 frames of the held last frame.
- 0x1b3c80: set to 255 by the enter (0x9a86a), -1 by 0x9a8c0, 0 by the race-end loop (0x7fc25), and while playing
  lowered by 10 an update down to 0 (0x9ae9c). Nothing draws it (every reference to the address is one of these, a
  scan of the XBE): its only effect is that the hold above runs while it is >= 0 (0x9adfc), i.e. always after the
  enter.
- Models (0x96be0): `characters\<file>` per type 2 / 10 object into slot +0x4c (0x50fac8 + slot x 0x610; 0xbb2a0 with
  "SpecAlphaHierarchy - 1 Tex", which only the type-10 ships `feship_*` have: [models/effects.md](../models/effects.md) "Effects"; the pilots are
  skinned and draw with shaders\char*bone.xvu, 0xa3df0); 0x9b170 maps the folder to a pilot (its first five bytes against tara yuri kumiko darius jordan
  marcus leah kukailimoku alicia vladimir abi zafir kaitlyn jaya lukas kutoo). Each frame (0x99360) per model: the
  loop flag, the animation bound by bone name when it changes (0xba060 -> 0xb55e0: every bone's track reset, then the
  records whose names match), the clock; drawn with SetTransform(world = 0x1efd80 + slot).
- Speech lines (0x96a60 -> 0x9aff0 / 0x9b070 / 0x9af70 / 0x9b0f0): the table 0x1a8768, 248 records of 0xb0 bytes (+0
  the cutscenes.str id = the index, +4 the speaker, +8 the key "IDS_SILVER_TARA" padded with tabs, +0x30 the WAV under
  `data\movies\speech\` (the voice folders: darius\zaki_N.wav, kukai\haw_N_C.wav ...), +0x70 the face animation under
  `data\characters\` ("tara\tara face 01.cms"), id 9999 ends it); the first record whose key matches the object's,
  compared up to the first character below '!' of either (silver_tara.nms's speech names no key and gets record 0).
  While a speech's subtitle shows and it has a face animation, the models of its speaker get it on their face bones
  (0xb5710: bone flag 0x1000 and the record, kept after the speech) at the speech's time (0xb9ec0: flag 0x80, the
  bone's own clock).
- The subtitle (0x98c30 / 0x9aa00): while a speech shows, the panel 0xc1b40 at (margin, 300) 640 - 2 margins x 100,
  line 0xffffff00, fill 0xa0ffa030, plasma 0.4; the speaker's portrait (g_hud_pilot_icons) 84 x 84 at (margin + 10,
  305); the text box +0x125c at (margin + 96, 310), 640 - 2 margins - 128 wide, 90 high, white: cutscenes.str
  (`Locale\<language>\cutscenes.str`, 0x9a7e9) of the line's id.

## Head turns (0x972f0 kind 2, 0x99360)
- Each frame the track update sets the model slot's target 0x1f0264 = -1; a segment with an animation then sets it
  to +0x18 and the share 0x1f0288 to +0x1c when its flags have 2, else -1 / 0. 193 segments have flag 2 (2 more with
  1 too); their targets: another actor (type 2, 152), a camera position (6: the actor looks into a camera, e.g.
  silver_tara), a group (0, a world point), or a stale index past the object count (skipped).
- 0x99360 per loaded model (slots 0..7, in order), outside the editor: a target <= 0 or >= the object count (0x998b2)
  clears every bone's +0x13c, +0x198 and flags 0x6000. Otherwise the point (0x1f01f8 + slot x 12) is the target's
  world translation (+0x320); for an actor (type 2) the last bone of its model whose name contains "skel_head" (0xb5190
  is "contains"; the string at 0x1a2f98) gives its +0xf0 instead, the translation of +0xc0, which 0x997a2 copies from
  the world matrix +0x40 after each model's draw: this frame's for a slot drawn before, last frame's after.
- Then per bone of the model, by name: contains "skel_head" -> flag 0x2000, share +0x198 = the segment's; else
  contains "_neck" -> 0x4000, share x 0.3 (0x1a35d0); else exactly "eyeL" / "eyeR" (0x19befc / 0x19bef4, 5 bytes) ->
  0x2000, share 1; +0x13c = the point. Others are cleared. So the eyes look straight at the target, the head turns by
  the share (0.56..0.8 in the files, mostly 0.6), the neck by 0.3 of it; the neck's children (the head) hang off the
  unturned neck (0x4000), the head's (the eyes) off the turned head. The turn itself: [models/README.md](../models/README.md) "Bone flags at run
  time" (0xb8630, in world space: the bones pose under the model's world, 0xb83e0(world, ...)). No limit, no
  smoothing: the share is applied afresh each frame; between segments it snaps.
- Argon (movie\head_turn.cpp, bone_aim.cpp, Cmm::pose with aims): the bones pose in model space, so the point goes
  through the inverse of the actor's world, and a turned bone's rows are divided by the world's scale (two actors
  have 1.25 / 1.113; the game's turn keeps only the bone's own scale there).

## The scene's light
- seq_start_copy (0x96308): the header's +0x210 direction is checked by 0xb4f80 (all finite, not all within 1e-6 of
  0, none above 1000); a failing one becomes (0, 1, 0) and black. fe_set_light1 (0xa7ea0) makes it D3D light 1,
  directional, the unit direction (0x2f250) and the colour's red / green / blue / 255, also kept at 0x388f64 /
  0x388f70; the enter 0x9a810 turns light 1 on. The track's ambient and sun (light 0) stay as the race has them
  (0xb04a0). Most story files have (1, -1, 1) or a hand-set direction with 0xff808080 or a tinted grey; the glory
  flybys 0 / garbage (unused there).
- Who sees it: the skinned pilots: shaders\char*bone.xvu (0xa3df0) lights `min(min(v7 N.c5 c9 + c8, 1) + v7 N.c36
  c37, 1)` with c36 = -the unit direction (0x388f64) and c37 = the colour (0x388f70), c5 / c9 / c8 the sun's unit
  direction, its colour and the ambient; the fixed-function lit meshes through D3D (summed, then clamped). The
  SpecAlphaHierarchy ships do not (their draw 0x7d0e0 uses only the sun and ambient).
- Argon: the scene UBO's light1Colour / light1Dir while a scene plays (raceSceneConstants' SceneMoment::light1, render\scene_constants.hpp), mesh_shading.glsl d3dLighting.

## racestart
- `<t>_racestart.nms` (one camera): 0x95e70 loads it into the resource 0x1ba9f4 (flag 0x1ef968) next to the glory
  flyby, and 0x93340 frees it; nothing else reads the resource or its data (a scan for 0x1ba9f4..0x1ba9fc /
  0x1ef968), so it is never played. Not loaded by argon.
