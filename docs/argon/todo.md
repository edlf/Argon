# To do

What is still open, in priority order. Details are in the doc each item names; what is done is
in [argon/done.md](done.md), and on-purpose differences from the game in [argon/differences.md](differences.md).

## 1. Wrong now (seen in play, fix next)
- [ ] **Mega-up cage frame colour on a real Xbox** (objects\megaup obj19): a capture from a real Xbox (Silver)
  shows the frame bars mostly white with pink / magenta at their edges; argon's (and xemu's) are
  saturated magenta-pink. Every input and the NV2A program were read live in xemu and match argon ([models/README.md](../models/README.md)
  "The mega-up's cage in the race"), and D3D's maths on them gives magenta, so the real NV2A computes the lit colour
  or the env texgen differently from xemu; xemu cannot be used to find it. Tried: light 0's specular (0.5, power 4,
  the NV2A's specular enable is 1) added to the lit colour: barely changes it (reverted). Left: the texgen (reflection
  map + the matrix 0x50e120) as the real NV2A runs it, and how it lights per vertex.
  The glass star (obj21, the Hud Lens) is settled: clear glass over the black and red core on the real Xbox, as in
  argon; xemu's pale white star came from a blurred echo copy in its memory (the game copies 1:1; [models/README.md](../models/README.md)
  "Hud Lens").
- [ ] Fixed-function lighting per vertex and env maps with the non-local argon (every lit surface and
  env-mapped stage): the shading on low-poly track meshes and the reflections on hulls and pickups against xemu.
- [ ] **Ship trail look**: none of the stage-combine options match the game. The trail draw (0x9dca0) sets no combine
  and inherits the last translucent mesh's state ([effects/README.md](../effects/README.md)).
- [ ] The Ghost Data menu looks off.

## 2. Linux port
Phases 1-5 are done on Windows (platform layer, case-insensitive game paths, miniaudio, Theora + Vorbis videos,
PadRumble); no Win32 API is used outside src/platform/*_win32.cpp.
- [ ] Build with GCC and Clang on Linux (build.sh / test.sh) and fix what they reject; nothing Linux-only has been
  compiled yet (platform_linux.cpp, pad_rumble_linux.cpp, miniaudio's Linux backends).
- [ ] Run it: the game data through the case-insensitive lookup (game_path.hpp), sound, the intro videos, rumble
  (FF_RUMBLE needs read / write access to /dev/input/event*), BC textures on the GPU's driver.
- [ ] [argon/build.md](build.md) "Build" (the Linux packages and steps): written, to check on a real Linux install.

## 3. Open question
- [ ] AI turbo: does the AI turbo in xemu? The decoding ([physics/turbo-flaps-cameras.md](../physics/turbo-flaps-cameras.md) "Turbo", 0xe10f0) says it never does: nothing
  writes its triggers +0x47d / +0x47e.

## 4. To compare with the game (ported; only checked in argon)
Race:
- [ ] Mars / alt_mars: the small quads with the 100-frame texture 14 near (870..940, 345..365, 817..942)
  and (841..909, 505..525, -982..-855): neighbours drawn in a row animate in step at one rate.
- [ ] Glows and heads: ship pod glows and Kalau's lamps dim as they are hidden (no fog on the lamps);
  Kutoo's and Jaya's heads on character select (the reflection); the rate-0 textures on Mars and Alicia's craft still.
- [ ] Race end table: a clear quick race win (the drones behind in their places at the line, estimates
  never below the row above, ties exactly 1.42 s more); the time limit's DNF row (no number, blank lap / kills).
- [ ] HUD: the red needle's wobble at full throttle in a fast class; the Japanese HUD (dashboard in
  Japanese vs settings.json language "Japanese"); the split-screen turbo value 1 px higher.

Look:
- [ ] The Hud Lens (the mega-up's glass star) and the pickups' cube map.
- [ ] The road bump ([models/effects.md](../models/effects.md) "Effects"): the ambient, the masked specular (diffusespecalpha), the far road staying
  unfogged while the scenery fogs (Mount, Bracken, Silver).
- [ ] Crafts' brightness and colour (lit by their ground colour) in the race and the front end; the lens flares on
  Fuusoku / Solar; why Bracken's sun shows nothing from the ground (occlusion or the far plane?).
- [ ] Camera-facing meshes in the race: the start-gantry stars and lightglows against xemu (the front end's globe is
  settled: no render list at its draw, [effects/README.md](../effects/README.md)).
- [ ] Lights no longer flashing behind hills as new track sectors come into view (Draco after the start).
- [ ] The hit flash on the position list row (how much red the portrait lets through).
- [ ] Atlantis Mode's tint and wobble, Hyper Blur, the mirrored minimap.

Audio:
- [ ] The race's new sounds by ear: hit / scrape loudness, the doppler of passing crafts, the turbo's
  fail tail, the reverb under roofs and on rock (an approximation of the DSP's), the cockpit filter's strength (the
  game's 10 dB resonance at 1400 Hz), whether the score page's ching rises in pitch in the game.
- [ ] The engine's loudness (the curve is the game's; the mixing is not), the turbo sound's loudness, `newlap` as
  the record's sound, the first engine-off hit since boot being silent.

Replays ([replays/README.md](../replays/README.md)):
- [ ] View Replay: the KERB shake (bracken "down before jump", flames "Start line"), the HUD and its power-up
  panel filling up, the music starting on on_start.
- [ ] The replay cameras (`--menu --replay 1 --race-go` prints each cut): the same cameras, cuts, framing and fov as
  replay1 in xemu; the replayed weapons, flaps and jets; whether the game knocks out craft 2 in replay1.

Front end ([frontend/README.md](../frontend/README.md)):
- [ ] Multiplayer (needs two or more controllers): joining on FE CHARSELECT, the split views and HUDs,
  MULTIPLAYERENDMENU, players 2..4's turbo at the start, the 3D characters in their boxes, the loader's player rows.

## Notes
Race sounds ([audio/race-sounds.md](../audio/race-sounds.md) "Race sounds", [audio/ships.md](../audio/ships.md) "Ships", [audio/race-sounds.md](../audio/race-sounds.md) "Reverb and filters"):
- Never played (every call of sound_play_ui 0xc3ca0 and of the turbo fail 0xc3540 checked, none through a pointer;
  TYPE TERRAIN is parsed but never read): SF_GENERIC_WIND, TRACKRUMBLE, LOOSE, ROUGH, WATER, the ships' MEGAFIRE and
  WEAPONTRAVELSPECIAL; the AI nodes' reverb_armco and reverb_custom1..3 flags. Argon can play them
  (app_race_sounds.cpp) but, as the game, never does. RADIALLAUNCH is the shield's launch (weapon_launch 0x6537b); no
  track plays a sound of its own on Silver's loop.

The XBE audit also listed every game function whose address the source never cites; besides the items
above they were renderer / file / sound-engine plumbing, save signatures, the Xbox demo launcher (fe_launch_xdemos)
and functions ported under other addresses (the flaps 0x529f0 / 0x546a0 / 0x57220, cashmodifiers, FE MUSICSELECT).
Every FE / race menu page in the XBE has argon page.
