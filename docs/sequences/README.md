# NMS sequences (`data\movies\<t>\*.nms`)

In-engine scripted scenes ("NMS Sequence", `sequence.cpp`), saved by the editor as a memory dump of its objects
(pointers in the file are stale; the loader rebuilds them). Per track folder: `<t>_glory.nms` (the loading
screen's flyby), `<t>_racestart.nms` (one camera), and the story scenes `<t>_<pilot>.nms`, `_winning`,
`_beatnemesis` (actors with `.cms` animations, speech from `data\movies\speech\<pilot>\`, subtitles).

| file | what |
|---|---|
| [argon.md](argon.md) | argon's port |
| [scenes.md](scenes.md) | story scenes, head turns, light |

## Layout (seq_start_copy 0x96230 / seq_start 0x96690)
- Header 0x1210 bytes: track name at 0, track folder at 0x100, object count at +0x204, the scene's light at +0x210
  (direction x y z) and +0x21c (D3DCOLOR; below, "The scene's light").
- Objects, 0x7f0 bytes each, from 0x1210: name +0; +0x40 type (0 group, 1 Scene Stage, 4 camera, 5 camera
  interest, 6 camera position, 7 Master Dopesheet, 8 CMS animation, 9 speech, 2 actor (a character), 10 model (a
  front-end ship)); +0x48 the partner (position <-> interest); +0x4c resource index (the model / speech slot,
  0..7); +0x50 parent, +0x54 last child, +0x58 previous sibling, +0x5c next sibling (out of range -> -1); +0x160 file
  name (2 / 10: a `data\characters` folder, 8: the animation's source, 9: the speech line's key); +0x370 scale x y
  z, rotation x y z (degrees), translation x y z, +0x394 type 8: its animation, +0x398 fov (degrees, 45 by default);
  +0x3ac the object's length (computed; types 3 / 7 / 8 / 9 keep the stored one: an animation's is its length, a
  speech's 1.0); +0x3b0 the time its curves were last taken at; +0x3c0 ten curve slots (non-zero = stored):
  0..8 -> +0x370..+0x390, 9 -> +0x398.
- Then each type-8 object's CMS animation, in object order (linked in place by 0xb5270, which returns its size):
  the `.cms` file's bytes ([models/meshes.md](../models/meshes.md) "Animation" records behind a 0x30-byte header: +0 length in seconds, +4 track
  count). The glory files have none.
- Then every stored curve, objects and slots in order: 0x20 header (+4 key count, +0x10 / +0x14 first / last
  key time, computed), keys of 16 bytes: time, value, weight, unused.
- Then the dopesheet: a count, then per track 4 bytes + 0xa0 bytes (name, +0x40 segment count, +0x4c length,
  +0x50 kind: 1 cameras, 2 actor animation, 3 speech; +0x58 object), each followed by its segments of 0x20 bytes:
  start, end, object (-1 none), source start, source end, flags, +0x18 an object, +0x1c a value. The last segment of
  every track is a 2 s tail with stale data; the lookup skips it. Structs: `src\original\nms_records.hpp`; all 212
  files walk to their end this way (`tests\story_scene_tests.cpp`).

## Playback (seq_update 0x98b70, each frame)
- Time = fmod(the sequence clock, the first track's length) (the "Cameras" track in every file): the glory flyby
  loops; a story scene ends there (below).
- A track's segment (0x971d0): the last of the segments (all but the final one) with start <= t <= end, else the
  first; its object's time = (t - start) x f + source start x f, f = the object's length / (source end - source
  start). Kinds 1 and 2 clamp t to [0, the track's length] first; kind 3 does not.
- Cameras (kind 1, 0x972f0): the camera and its children take their curves at its time (0x98900). The view (0x9ae4c /
  0x99419): eye = the position's world translation, look-at = the interest's, fov = the position's +0x398. A change
  of camera starts the 0.25 s scene transition ([frontend/README.md](../frontend/README.md) "Scene transition").
- Actor animation (kind 2): the track's object is a model (type 2 / 10); a segment on a type-8 object gives the model
  that animation (0x1effa4 + slot), its time (0x1ef98c, the model's clock +0x5e4), the loop flag (segment flag 1,
  0x1efa00; else every bone gets flag 0x400, play once: 0xba000) and the model's world matrix (0x1efd80 + slot x
  0x40). Without such a segment the model keeps what it had. Segment flag 2 with +0x18 / +0x1c: the head turns
  towards that object by that share (below, "Head turns").
- Speech (kind 3): a segment on a type-9 object plays its voice when it differs from the voice started last
  (0x96dd0; the old one stops) and shows its subtitle (0x1b3c88) with the time into the segment (0x439f7c); a group
  object (0) or nothing stops the voice and hides the subtitle.
- Curves (0x98720): before the first key its value, after the last key the last value; between keys i and i + 1 a
  cubic Bezier (0x986c0) from v_i to v_i+1 with inner points v_i + (in x w_i + lin) x dt and v_i+1 - (out x w_i +
  lin) x dt, where lin = (1 - w_i) x (v_i+1 - v_i) / 3 / dt, in (i > 0) = w_i-1 x (v_i+1 - v_i-1) / 3 / (t_i+1 -
  t_i-1) + (1 - w_i-1) x (v_i - v_i-1) / 3 / (t_i - t_i-1), out (i < n - 2) the same around key i + 1. Objects with
  curves outside the cameras take them at their +0x3b0 (one group in all the files).
- An object's length (0x98190) = its last key time, at least 0.1; a position / interest raises its camera's.
- Local matrix (0x97d40): S x Rz x Rx x Ry (0x97740, row vectors, D3D's rotation matrices) and the translation in row
  3; world = local x parent world (0x97e90), for a model with an animation segment local x the animation object's
  local x parent world. A camera position's rotation is aimed at its interest (0x98030: only its children would see
  it; none have any). The story scenes' Scene Stage turns and moves everything under it.
- The game also loads the track model and sky for it (0x969c0) and draws ships / particles (0x99360).
