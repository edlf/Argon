# Done

What has been ported, fixed or confirmed, by date (the latest first); what is still open is in [argon/todo.md](todo.md).

## 2026-11-21
- Per-mesh fog (render\mesh_fog.hpp): glows with their own fog mode 0 (most flares, lamps, pickups' inner meshes) no
  longer fade with distance; additive meshes on the track's fog take the fog colour far away (confirmed in play).
- The single frame freezing for 1-2 s in a race: solved (confirmed in play).
- Widescreen (`g_widescreen` 0x298ecc, [frontend/README.md](../frontend/README.md) "Widescreen layout"): the remaining pages (options,
  course / speed select, profiles, pause, upgrade, score pages, loader) and the race HUD (0x4dc16 etc.) done.
- The front end against the game ([frontend/README.md](../frontend/README.md)), confirmed: the crafts' bump / specular look
  (SpecAlphaHierarchy); FE TITLE against the real Xbox (the background's darkness, the emblem's gold); every ported
  page (FE TITLE, CHOOSESAVE, NAME ENTRY, MAIN, CHARSELECT, CHAR / SHIP INFO, COURSE SELECT, TOURNEYTYPE, SPEEDSELECT,
  Track Data, the LOADER, WINMENU's Quit, FE AWARDS); the UI sounds, the list sound at a list's ends and B on FE MAIN;
  Yes / No and info boxes; Time Attack's Track Records, record row and "Saving"; the pause menu (sounds, "Saving
  Race", the reconnect message, HUD and fonts) and the race-end table and menus. Multiplayer is still open.
- The music by ear ([audio/music.md](../audio/music.md) "Music"), confirmed: the race end's fades, what restores the music after Track
  Select / Main Menu (0x99fb0's caller, WINMENU's items), no gaps between segments and their order.

## 2026-11-20
- Sound effects after leaving a race: none left playing (checked by a voice log on quitting from the pause menu, with
  "Saving Race", and from the results; confirmed in play).
- The `.cmm` leftovers ([models/effects.md](../models/effects.md) "Spheremap Decal"): the shell light the effect's program uses (ambient 0x40,
  colour 0xcc along unit(-0.5, -1, -1); black for a locked character), stage 1's texture matrix (never applied under a
  vertex program), Jaya's stage 2 reading the unwritten oT2 (the texel at (0, 0)); mesh.frag kind 8 lights and samples
  them so (Kutoo's and Jaya's heads confirmed in play on character select).
- The `.cmm` writer: tools\cmm\gltf2cmm.py (an edited .glb's mesh geometry into model.cmm, every other byte kept;
  every disc model rebuilds byte for byte) and the add-on's "Write track model to game files" (Mars round trip in
  Blender: triangles and colours kept, a moved mesh written moved).
- Changing tracks could lose the GPU (found and fixed; already in c6d4897, before the renderer work): the props'
  texture sets (ships, pickups: kept for the session in the props' pool) bound the loaded track's environment cube for
  their specular-bump meshes (App::cubeOwner), and replacing the track destroyed its cubes while those sets still used
  them (drawn with a destroyed view, or VK_ERROR_DEVICE_LOST). Now each model lists its specular-bump draws
  (Track::cubeDraws) and App::refreshTrackCubes makes their sets again whenever the cube owner changes, a borrowed
  table's in the track's set pool so they retire with it. Props loaded in the front end now take the race track's
  table once one is loaded. Checked with `argon --validate --track mars --pick-track kalau --frames 400 --shot
  out.png`, four runs clean.
- Answered in play: the colours look right, the flares are done, no leftover ships, the in-race names load (infotext
  400 + character; all 257 fixed-id string lookups find their text, the 68 computed-id ones are unchecked).
- Ghidra: tools\cmm\ghidra\label_frontend_leftovers.py run on the project (24 front-end functions named and commented).
- Confirmed in play: the credits' pictures, the converted audio (music and effects, loops, leaving a race).
- Confirmed in play: the attract demo, the story scenes, UPGRADEMENU's weapon icons, the page transitions.
- The power-up panel's five status sprite pairs and its "No Weapon" text (confirmed in play).
- The shield bar looks right in play (the TSHIELD frame and fill, the hit blink, "Shield Low").
- The racing looks right in play: hull contacts, the ships at the start, the off-track reset, the opponents and their
  catch-up, the AI's routes at forks, the hover over Fuusoku's waves.
- The visual effects look right in play: water states, the wake, the ripples, the wet lens and underwater overlay,
  dust, rain, the track and weapon sprites, the minimap, track culling.
- The weapons look right in play ([weapons/README.md](../weapons/README.md)): explosions, the super weapons, beam sparks and flashes, the weapon
  sprites and trails, the hit effects, blasts, knock-outs, the projectiles in flight and the fire buttons.
- The missing effects: rain splashes on the ground and the water spray (confirmed in play).
- The race end: POSTABLE / SCOREMENU / POINTSMENU / WINMENU / LOSEMENU blur the live race each frame
  (pause_background every frame, held only in the pause), and after a single-player finish the race is seen through the
  replay cameras (seen on the console; the switch is not traced in the xbe).
- HUD Size (pause menu and Main Menu > Argon Options, settings.json `hudSize`) and Widescreen (Main Menu > Argon Options, settings.json `widescreen`: off keeps
  the 4:3 layout of the menus and the HUD); the replacement font's outline as thick as the menu panels' (1 unit).
- Credits: the "[QRLOGO]" icon tag (data\qrlogo.dds at its own 256 x 128) and a black background.
- ATTACK WARNING (race string 0x51f, red, under the power-up panel) while the player's craft is in danger: a homing
  projectile or beam on it, a projectile within 300 m, a shot's path through it ([weapons/hud.md](../weapons/hud.md) "Attack warning"); not
  yet seen in a race.
- The pause menu holds the other crafts' engines too (Audio::setPaused left them playing); to check by ear.
- The debug view (the editor): starts muted (the Audio window's Mute), a Baked lighting toggle in the Render window
  (the vertices' prelit colours drawn white), the mouse cursor hidden in every other mode.
- src's root files moved into topic folders ([argon/source.md](source.md)).

## 2026-11-16
- FE TITLE: the lettering unlit as its surface says (white-to-blue chrome), "Press START" orange (0x34511), as on a
  real Xbox; the front end follows the surface lighting.
- tools\assets\convert_assets.py also converts the music to FLAC (bit-identical) and the effects to Ogg Vorbis at 90 kbit/s
  (libvorbis's highest for mono 22 kHz; full ADPCM blocks, PLAYLENGTH / LOOPSTART / LOOPLENGTH comments); argon
  prefers them. Leaving a race stops the drones' engines; F11 borderless fullscreen; the FPS meter on F2 / the Debug menu.
- Weapon explosions as models (explosion_type0..15, the explosion table at the end of weapon.nwp) with their go-off
  flashes; launch frames and flight motion (hugging, wall sliding, the frame turn); 250 slow sparks on a craft hit;
  knock-outs with type-6 explosions; each ship's special ([weapons/README.md](../weapons/README.md) table).
- The super weapon's full damage: a lock-on volley aims every projectile, the nearest lock first (0x6c22e; the
  argon left all but the first few untargeted); its level-3 shield only when the craft can fire; `--special-at`.
- The window title is just "Argon".
- Original profiles ([saves/README.md](../saves/README.md)): tools\saves\profiles.py reads / writes / signs / converts the game's save folders
  (savegame.sav + SaveMeta.xbx), tools\saves\profile_editor.py edits them; the signing key comes from a file you give.
  Checked on the Xbox: converted and edited saves load fine.
- The course select's globe, read live in xemu: no render list at its draw (0x298ee0 = 0 at 0x1d59b), so it keeps its
  turn, as argon draws it.
- The texture bind cache (d3d_set_texture 0xcbb40, [models/materials.md](../models/materials.md) "Texture table"): a run of meshes binding the same
  animated slot shows the first one's frame (Mars / alt_mars' texture-14 quads).
- The converted videos are one .ogv each, Theora + Vorbis muxed (tools\assets\convert_assets.py; movie\vorbis_track.cpp
  copies the soundtrack's pages out for stb_vorbis); the separate .ogg soundtracks are gone.
- Custom music plays .mp3 songs (dr_mp3, audio\song_files.cpp); no WMA, by choice.
- The .cmm vertex layouts (plain / packed / skinned) as src/original records; all 234 models load byte for byte.
- The namespace qr is now argon (src, tests, docs).
- src/original headers for the replay files, DDS, WAV, the LSBL string tables (layout corrected), the font .map and
  visdata.vis; the readers use them (every game file reads the same).
- Vulkan validation: the Hud Lens's resumePass now shares renderPass's external dependency (VUID 00904 / 02684
  gone); left: unused vertex attribute / output warnings at pipeline creation (no VUID).
- The .cmm records decoded ([models/README.md](../models/README.md); tools\cmm\effect_survey.py): effect records, Keyframe clocks, animation
  tracks, materials, the texture table. Ported: Color Scale No Texture (pod / lamp glows dim by occlusion, no fog),
  Spheremap Decal (Kutoo's / Jaya's heads), a stage rate of 0 freezes, frames round to nearest.
- The shield bar's white blink (huddata +0x70 = the shield flash +0x94: 0.8 a hit, 0.2 a corrode frame, capped at 1,
  minus dt); white / red on opposite halves of a 16-vblank period, now timed at 60 Hz.
- The beam sparks (0x68150): starburst x4 on a hit, starburstred at a wall each frame, the spinning cell-10 sprites
  (0x9d000), the beam's grey fade (bone tint 0xb9930); the stand-in 25-particle burst is gone.
- The beams' one-frame flashes ([weapons/shields-beams.md](../weapons/shields-beams.md) "Beam and spark flashes"): the grey start glow (0x68df5), the spark
  flash 10..13 and the white hit flash 25 on a first-frame hit, the wall spark each frame; the hit point on the beam.
- trail_reset 0x9d9a0 (the race's effects reset 0x3f730) and the sprite pool record 0xbd670 ([effects/README.md](../effects/README.md)): trail
  sprites and dust spin as the game, spray puffs upright, the droplet cell the right way up, the 0.97 uv inset.
- The race end table ([physics/standings.md](../physics/standings.md) "Standings"): 0x8f140 writes the rows once (POSTABLE ON); the finish order fixed
  at the finish (0x3f320), estimated times, the 1.42 s order rule, DNF rows; drones' race times stop at their finish.
- HUD details: needle-2 jitter (rand() noise of +-0.1 g, 0x4ad71), the Japanese layout (language 2: shield line T + 79,
  rows, speed, score, turbo values); AWESOME kinds 1 / 3 / 5 / 7 are never awarded (award_points 0x83f00's 11 callers).
- The water's render states ([water/README.md](../water/README.md) "Render states", water_draw 0xce1c0): no fog, no culling, alpha blend, depth test LEQUAL and write on; the ship shadows now draw after the water.
- Hull contacts as the game (0xd3c70: 24 box triangles vs the track triangles, 0xdb7f0; only world-normal contacts get the response 0xd5140; no push-out).
- No cockpit jolt or shake in replays (0x5ba10 skips while 0x52afac; 0x5aa10 rebuilds the tripod).
- The cockpit's contact jolt +0x8b4 (0x5ba10: landings / head-on hits dip or roll the view up to 0.1) and the
  drone turbo's 1 s cooldown after a blast (+0x9c4, 0x6f9f0); the AI still never turbos (+0x47d / +0x47e unwritten).
- The explosion push (0x5dee0): braking up to 42 % of the speed (doubled for drones in Tournament / Quick Race),
  a sideways shove; the knock and surface camera shake in the cockpit view (0x5b610 / 0x5ba10).
- Story scenes' leftovers: head / neck / eye turns to the look target, the 12-frame hold, the scene light (light 1),
  SpecAlphaHierarchy (the bump / specular effect on every craft hull, UV set 0); the fade is never drawn,
  racestart.nms never played, no pre-race file is ever missing on the disc.
- The tournament's story scenes ([sequences/scenes.md](../sequences/scenes.md) "Story scenes"): before each race (0x80090 / 0x1b4158) and after the last
  win (beatnemesis / winning, 0x7fa50), with posed actors, speech, face animations, subtitles, letterbox and skip.
- Options leftovers: cheat 4 upgrades all 16 characters (0x1a83b); Custom music picks rand() % count each song
  (0xdf150); LEVELMUSIC's areas; rumble (beam boost, explosion knock, hover compression, wrapping words, pad
  thresholds, the knock-out fade); Controller 2..4 Vibration ([physics/rumble.md](../physics/rumble.md) "Rumble", [frontend/README.md](../frontend/README.md)).
- The front end's UI sounds per page from the game's 93 calls ([frontend/README.md](../frontend/README.md) "UI sounds"; ui/fe_sounds.hpp).
- The AI's route choice at forks (0xe1370 / 0xe4730; [physics/opponents.md](../physics/opponents.md) "Opponents"): 8 look-ahead lists (surface, shortest,
  turbo_good, pickups), the nemesis / home circuit on the shortest, the rest by (pilot + lap + slot) & 7.
- Catch-up by drone kind (0x6d650; [physics/opponents.md](../physics/opponents.md) "Opponents"): the nemesis (any race) and home-circuit crafts use
  their zone rows and the player's _Nemesis_AI_modifier; ai +0x470 is never written (0.3); aivalues gains unread.
- Dust behind the crafts (craft_hover 0x5f35b..0x5f662; [effects/README.md](../effects/README.md) "Dust"): the rear rays on rough / loose ground,
  1 in 16 per substep, sandy (Mount grey) by the ground colour, 2.5 s spinning puffs.
- The water wake as the game emits it (both rear corners under water, the walk from s = 0, the heights, 4096
  pools): far denser than before. No water-entry splash exists in the code ([effects/README.md](../effects/README.md)); the wave*.dds are unused.
- Replays ([replays/README.md](../replays/README.md)): the static cameras' KERB shake from passing crafts (0xd7824), View Replay's music restart
  on on_start (0x3e8e0), its HUD shown with the replayed pickups' levels; signal 6 has no visible effect.
- UPGRADEMENU's weapon icons (0x9f43e..0x9f68d): the shadowed base, the caps by level, the pulsing next cap,
  weaponspecial at levels 6 / 7.
- The race end's music fades ([audio/music.md](../audio/music.md) "Music fades"): out over 1 s on WINMENU / LOSEMENU's Yes, a dip and back on
  SCOREMENU / POINTSMENU's A and UPGRADEMENU's Continue, linear in mB, with "Saving Race" that second.
- The pause menu: its sounds (0x17 / 0x18), "Are you sure?" on No, no Track Select in a tournament, "Saving Race"
  after a volume change, the reconnect message (race 0x548 + port; 0x3f9 is unused); the cursor draws nothing itself.
- Bone flag 0x200: the +0x194 selector is the shields' (0x653c0, every shield 0 = attached); projectiles'
  animated child bones (Abi XSTAR, Leah AVALANCHE, Kukai sphere4, Kumiko DATABOMB) now play on the race clock.
- The bump road: no fog (its draw 0x7d6e0 sets fog mode 0) and alpha never seen (opaque, R5G6B5 back buffer).
- Lens drops: every dried drop comes back on surfacing (any track), 21 from spray within 2 m; the underwater overlay
  over the rain; no rain in split screen as in the game ([effects/rain.md](../effects/rain.md) "Rain").
- Lighting as the game sets it ([models/effects.md](../models/effects.md) "Lighting"): setup 1 lit with bit 15, flag 4 / 0x100 / 0x200 / 0x2000
  rules, the crafts' own lights from the ground colour under them (0x59bc0, 0x5f0d7); the sun and lens flares show.
- Camera placement surface flags ([effects/rendering.md](../effects/rendering.md) "Camera-facing meshes"): 0x800 faces the eye (lightglows, sun,
  craft flares), 0x400 offsets from the eye, 0x100000 slides the sun's lens pieces along the view axis.
- Lights through walls: a light not yet tested drew at full brightness for ~2 frames as its sector came into view;
  untested now means hidden, as the game's 999999 (0xb9540 / 0xb709e; [effects/rendering.md](../effects/rendering.md) "Flare occlusion").
- The hit's red flash: the hit human player's row in the position list, 0.8 per hit fading by dt (0x6d430,
  hud_minimap_draw 0x4faf8; [weapons/hud.md](../weapons/hud.md) "Flashes").
- sprite.dat's track quads: Mount / Bracken trees and mist, alt_mount / Silver glows (0xbdf40, the sprite pass
  0xbd8f0; [effects/sprites.md](../effects/sprites.md) "Track sprites").
- The ticker is hidden on FE AWARDS unless the win unlocked an easter egg (the ticker element's visibility, shell
  draw 0x304bc; [frontend/README.md](../frontend/README.md)); pages can now hide it (FePage::showsTicker).
- The minimap on the alt_ tracks without their own map (gamehud_setup 0x4725e picks it by track id: those reuse the
  first route's, Mars / unknown mars.dds) and Fuusoku's scale (11.88 per texel, 0x4f145).
- The nemesis race is one vs one (race setup 0x79250: 0x78d60(2), no random fill; [physics/README.md](../physics/README.md) "Grid").
- The weapons' travel loops (queued at launch, started 0.5 s later, following the projectile / shield, [audio/README.md](../audio/README.md)
  "Ships"); the shield launch's RADIALLAUNCH.
- The race's audio ([audio/README.md](../audio/README.md)): wall hits and scrapes by surface, craft-pair knocks, splashes, the weapons' incoming /
  player hit / impacts / laser hit, the Shield Low beep, the tunnel, the score page's count-up with its chings;
  every craft's engine in 3D with doppler, every craft's ship sounds (engine off / on, projectile / laser fire), the
  turbo's held loop and fail tail for every human player; the reverb by AI node and the internal / underwater
  filters; the track loops on the game's rolloff curve.

## 2026-11-15
- The environment cube slots: water patches and specular-bump meshes take the game's slot (0 envdata\default,
  1 waterref1, 2 waterref2, 3..7 custom1..5); argon had them one slot off.
- Editor: a main window and optional tool windows; its track pick loads behind a loading screen and takes the game's
  track id (Kalau / alt_bracken rain there too).
- The whole app holds while the window is unfocused (the race pauses, no input is read).
- The Hud Lens (the mega-up's glass star) with the track's cube map for the pickups; setup-2 surfaces lit in the race.
- The results table frozen at the finish with the game's estimates; DNF only for racers taken out of the race.
- UPGRADEMENU's Insufficient Funds hides the details (font 0, wrapped at 262).
- The bump road: the game's shader is diffusespecalpha (the specular masked by the texture's alpha).
- FE CHARSELECT's "?" centred and yellow.
- The canopy flicker (the shadow hull only casts the ship shadow); one craft build for the race and the front end.
- The off-track reset: the game's reset request (craft +5).
- Earlier (2026-11-14): the shield start values and bar, the shield / turbo HUD values, the in-race names, the
  seamless music stream, the water heightfield for the physics, the morph meshes and run-time bone flags, visdata.vis,
  FE INTROSCREENS / TITLE / CHOOSESAVE / NAME ENTRY / SPEEDSELECT / TRACK INFO, page transitions, the hud cursor,
  Trophy Level and unlocks, the message box kinds (3 and the per-pad controller message included), the loader's
  multiplayer list.
