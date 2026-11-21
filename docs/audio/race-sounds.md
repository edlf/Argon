# Audio: race sounds, reverb and filters

## Race sounds — `audio\generic\`, `audio\f_settings.txt`
- Leaving a race (the pause menu's Track Select / Main Menu, the race-end pages; App::openMenu): Audio::stopAll stops
  every race voice: the race events and music, RaceSfx (the travel loops too), the track's emitters (not cut_ambient: story scenes only), the
  rain and tunnel loops and every craft's engine layers, the drones' included (Audio::update stops running once the
  front end opens, so a voice left on would keep its last volume).
- Ids: `generic.txt` names each sound `SF_GENERIC_<name> <folder>`; the list parser 0xde740 gives the ids 1 RADIALLAUNCH,
  2 WIND, 3 RAINWINDSHIELD, 4 RAINGROUND, 5 TRACKRUMBLE, 6 LOOSE, 7 ROUGH, 8 WATER, 9 INCOMING, 0xa FASTESTLAP,
  0xb..0xf light impacts water / loose / rough / track / metal, 0x10..0x14 heavy, 0x15 SCRAPEMETAL, 0x16 SCRAPEWALL,
  0x17 MENUNAVIGATE, 0x18 MENUSELECT, 0x19 BADMENUSELECT, 0x1a..0x1c pickups, 0x1d LASERIMPACT, 0x1e PROJECTILEIMPACT,
  0x1f MEGAIMPACT, 0x20 PLAYERHIT, 0x21 CHING, 0x22 PERCHING, 0x23 LOWENERGY. sound_play_ui 0xc3ca0 plays one by id.
- A sound's record (0xc6a40) defaults: MINVOL -10000, MAXVOL 0, MINDIST 20, MAXDIST 200, positional (NOPOSITION
  clears it). A voice starts at MAXVOL. Positional ones use the rolloff curve 0x194f5c (5 points 1, 1/2, 1/4, 1/8, 0;
  SetRolloffCurve 0x13b82c), do not start beyond MAXDIST from the listener (0xc3bb0), follow their craft; split screen
  (mode 4) puts them on the listener. DirectSound 4721's CalcDistanceVolume (in the XBE at 0x13d8a7) splits
  MINDIST..MAXDIST into 5 equal steps and puts point k at the end of step k + 1, from 1 at MINDIST: full volume over
  the first step, 1/2 at the end of the second, 0 at MAXDIST (an earlier argon put 1 at MINDIST and 0 at MAXDIST with
  4 steps, fading every sound too soon).
- `f_settings.txt` (sound_load_f_settings 0xc9890; defaults 0xc96e0: impact force 20 / 60 / 150, ship 10 / 60 / 100,
  water 1 / 10 / 20 down, delay 0.25 (over 10 -> 0.25)). The file holds force 20 / 60 / 150, delay 0.25, ship 1 / 2 / 3,
  water -1 / -20 / -100, but the game reads it with one fscanf("<name> %d\n") per line in a fixed order, and
  "scrape trail 0.1" stops its %d at the fraction: every later fscanf fails, so the ship, water and max lines never
  apply and the game plays with the defaults (an earlier argon applied them, so every craft touch clanked).
  Hit volume (0xcabc0): a light hit runs MINVOL..MAXVOL
  over light..heavy, a heavy one again over heavy..max, in whole percent steps.
- Contacts (0xc2b50, each frame): the craft pairs (0x4479dc: |velocity change| of craft a, from
  physics_objects_contacts) over the ship heavy / light force play HEAVY / LIGHTIMPACTSOLMETAL at craft a. Each craft's
  world contact record (0x442e94, physics_substep: the responses' summed velocity change, their normal . direction
  averaged by 0xd0a30 once the frame's physics ends, and the surface): none -> both scrapes stop; |dot| < 0.35 scrapes from |dot| 0.1 (surface 0 / 4 / 5 SCRAPEMETAL, 1 / 2
  SCRAPEWALL), started once, volume by |change| on the world forces; else the scrapes stop and dot x |change| hits by
  surface (track / rough / loose; water light only). A craft's same hit does not start again within the delay (0xc3ad0).
- Splashes (craft_water 0x61340, a corner under water): down faster than the heavy water force -> water_enter unless
  the light one plays, faster than the light force -> impact_l_water unless water_enter plays; volume by the speed.
- Weapons: INCOMING (0xc2f90) for a human player a homing projectile (0x6704b) or a beam (0x68d2d) tracks, at most every
  20 frames, stopped 20 frames after the last call (0xc4bf0); PLAYERHIT (0xc2fe0) where a hit takes a human player's
  shield (weapon_damage 0x6ae5a); PROJECTILEIMPACT twice at every explosion going off (0x7dfde), MEGAIMPACT once at the
  special's impact (0x67e7c); LASERIMPACT at the beam's craft unless playing (0x6908a, not in the attract demo).
- LOWENERGY with each "Shield Low" popup (0x4ed72); the tunnel: the track's SF_<T>_TUNNEL from the internal tripod's
  update (0x5aa10, run every frame in any view by the craft update 0x5dae3) under env_inside nodes whenever it is not
  playing (0xc4220), stopped out of them: the covered loops' sound (argon played it only from the cockpit view).
- SCOREMENU's tally (0x95b00, each frame): the score climbs from 0 by a step of 737 that grows by 24, the line x 1.5 at
  y 215; each step plays CHING at 22050 + 2 x step Hz; at the score PERCHING (final_total, 0x95930) and the cash line;
  A skips to the end with MENUSELECT. (The game sets the frequency on the voice's speaker buffers; whether ching's
  single file plays there, so whether the pitch really rises, is not checked.)
- RADIALLAUNCH (Rlaunch, a 5.1 one-shot named like the weapon sounds Plaunch / mlaunch / Limpact) is the shield's
  launch: weapon_launch 0x64e20, reached for a shield too (0x6a390 -> the volley update 0x69e79), tests the record's
  class at 0x65351 and for class 2 plays sound_play_ui(1, the craft) at 0x6537b when the craft's slot (+0x1c) is 0 or
  the game mode is 4 (split screen): only the player's shield is heard in a 1-player race. Argon: RaceSounds::playLaunch
  from Weapons::raiseShield, the rule argon::sound::shieldLaunchSounds.
- Not a track sound: no track has a loop surface. The debug names (0x5ce70) call craft +0x85c 5 "Loop" (hover
  0x5e6d0 copies +0x858 into it, the scrapes treat 5 as track), but +0x858 comes from the collision table's +0
  (material +0x14), which is 0..4 in every track model; Silver's loop-the-loop (AI nodes 92..102, `down` turning over
  near (-1360, 150, 685)) is surface 0, its slippery meshes are the flat ice. mass_loopgravity (character_stats +0x5c,
  craft stats +0x120 next to normal / flight gravity +0x118 / +0x11c) is never read. No track list names Rlaunch; the
  ids from 0x24 up are the track's (0xc30f0), so none reaches 1. On the loop the game plays Silver's SF_SILVER_CITY
  (e_city1, LOOPS 1, TRIGGER at (-1357, 91, 688), MINDIST 30 / MAXDIST 500): argon plays it as every trigger.
- Never played (every call of sound_play_ui 0xc3ca0 and of 0xc3540 checked, none through a pointer; TYPE TERRAIN is
  parsed into the record +0xf8 but only types 2 and 4 are ever compared): WIND, TRACKRUMBLE, LOOSE, ROUGH, WATER.
  Argon loads them by id but plays none either; its uncalled players for them (RaceSounds::playTerrainLoop /
  stopTerrainLoops, playWind / stopWind, playMegaFire) were removed 2026-11-21 and are in git history. TYPE WEAPON's MINDIST 600 / MAXDIST 700 (0xc7283) is read into the folder's options (the
  travel loops, "Ships").
- Checked live (xemu Audio Debug on Silver's grid, 2026-11-20; voices told apart by their sample count, EBO + 1):
  of the 7 trigger voices the 3 waterfalls and 2 cities are muted by distance, the crowd (56 m, MINDIST 180) plays at
  the SFX option's -4 dB on the front HRTF bins, and the ocean (MINDIST = MAXDIST = 1000, 2.8 km away) at -4 dB on all
  four HRTF bins and the I3DL2 send: its zero-width range makes the rolloff 0 / 0 (NaN) and the voice keeps full volume,
  a constant low rumble over all of Silver (sound_rules rolloffGain; the rear bins and the reverb send are not
  ported). Only Silver's ocean has such a range.
- The track's TRIGGER sounds (0xc30f0) give each point its own voice with its MINDIST / MAXDIST on the same curve;
  argon keeps one voice per sound (the loudest point; one per point overloaded where zones overlap) with the
  curve.
- Argon: logic\sound_rules.cpp (the rules, tests\sound_rules_tests.cpp), audio\generic_sounds.cpp (the bank),
  audio\race_sfx.cpp (the voices), app_race_sounds.cpp (the events), GameCraft::soundContacts.

## Reverb and filters — `audio\reverbs.txt`, `audio\filters.txt`
- Reverb: reverbs.txt holds six I3DL2 presets in order cave, armco, custom1, custom2, custom3, none (FUN_000c9b60:
  room, roomhf, room rolloff, decaytime, decayhfratio, reflections, reflecdly, reverb, reverbdly, diffusion, density,
  hfreference) at sound manager +0x142c, 0x30 each. sound_frame_update (0xc4f35) sets the listener's
  (IDirectSound_SetI3DL2Listener) by the player's AI node: flag 8 (reverb_rock) the cave, flag 0x80 (env_inside) armco,
  else "none" (itself a quiet reverb, room -1000). The AI parser also sets reverb_armco 0x200000 and custom1..3 0x10 /
  0x20 / 0x40, which nothing reads. The Xbox's DSP (audio\dsstdfx.bin: an I3DL2 24 kHz reverb, with its default
  parameters) runs it on the I3DL2 send mixbin, but **no voice sends to it**: the only
  IDirectSoundBuffer_SetI3DL2Source calls (0x13b850) give lRoom -10000 (silent send) to every voice sound_play_ui
  starts (0xcad50 from 0xc3dbc, unconditional, so NOREVERB changes nothing; and 0xc35f9) and to every craft's engine
  submix (0xc3220); the third (0xcada0 from 0xc4d85) only runs while +0x8e9 is set, which nothing does. So the game has
  no audible reverb, and argon none (it had a software approximation on most world sounds until 2026-11-20).
- Filters: filters.txt "Internal" (65280 1400 10 ...) and "Underwater" (65280 440 0 ...), read by FUN_000c97d0: flags
  0xff00 = the DLS2 resonant low-pass (DSFILTER mode 1), cutoff Hz (0xc8910), resonance dB (0xc8990, at most 22.5).
  0xc8500(n): 1 internal on, 2 internal off, 3 underwater on, 4 underwater off, 0 none; the camera (camera_set_eye
  0xae640) switches to underwater while the eye is under the water surface and back to the internal one (or none)
  when it leaves. The filter goes on every positional sound (sound_play_ui) and the engine buffers. Argon: the
  internal camera's filter (the game's view +0xe24 0, "craft%d tripod internal", set by 0x71260 / 0x3df30; argon's
  tripod 3: argon had it on the close chase view, which muffled that camera), or the underwater one while the eye is under a water box (audio\low_pass.cpp, a biquad
  with the resonance as its Q).
- Argon graph (audio\world_effects.cpp): positional sounds and engines play into one sound group; a node filters
  its mix on the way to the main mix.
