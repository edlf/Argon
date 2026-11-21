# Effects: rain

## Rain (rain.cpp in the game: 0x90880..0x92340)
- Which races: the race load (0x800b3) sets 0x52afa2 for track id 1 (Kalau) and 0xd (alt_bracken), or for any track
  with Always Rain (0x52c8b8). Note: 0xd is alt_bracken, not alt_kalau (9).
- Rain objects (0xd00 bytes): neon_initialize_rain 0x7efd0 (switch on 0x52cd8c players) builds one per view at
  0x51cce8 + 0xd00 i (0x90880(drops, streaks, x, y, w, h)): 1 player 800 / 1000 on (0, 0, 640, 480); 2 players 400 /
  500 on (0, 0) and (0, 240), 640 x 240; 3 players 200 / 250 on (0, 0), (320, 0), (0, 240), 320 x 240; 4 players the
  same plus (320, 240). 0x77508 (start-up) builds one more at 0x5200e8 (800 / 1000, full view) for the NMS scene.
  The view rect is +0xce0..+0xcec; the 9 x 9 grid (+0x104) is the underwater overlay's.
- Split screen: no rain at all. The race view's copy of the strengths (0x93fb7), the rain draw (0x93ade) and the
  spray drops (0xbd092) all run only with one player (0x52cd8c = 1); the split-screen objects stay at their start
  values (strengths 0) and are never stepped or drawn. Nothing to port.
- Camera fields: camera_set_eye 0xae640 (only race views: +0x345, set by the race view 0x9345c; and a track model
  0x52cddc) first clears +0x34d, then reads the water's depth at the eye (0xce6b0 at the game clock 0x50e430 +
  0x50e434): depth <= 0 is under water (+0x34e). Going under (+0x34e was 0): +0x34c and +0x354 / +0x358 / +0x35c = 0,
  +0x34d = 1 (clear the lens), the underwater sound filter (0xc8500(3)); staying under changes nothing. Out of the
  water: coming up (+0x34e was 1) calls 0xc8500(0 / 1, by 0x448b74) and sets +0x34f = 1 (wet lens) on every track,
  raining or not; then raining -> +0x34c = 1 and +0x354 / +0x358 / +0x35c from +0x360.. (90 / 70 / 12), else all 0.
- The race view's frame (0x93e20, one player, before the scene draw) copies +0x354 / +0x358 / +0x35c to the rain
  object [+0x348] (+8 haze, +4 streaks, +0xc new drops a frame), clears the lens on +0x34d (0x90d60 and every size
  0), and on +0x34f brings back every drop with size <= 0.03 or off the view (0x90740, no limit: all 800 on a dry
  lens) and clears +0x34f; +0x350 (-> rain +0x1c "covered") comes from the view target's +0x50 (0xa3383). The race
  draw (0x93b39) then steps and draws the rain with raining = +0x34c, on dry tracks too: a wet lens on Silver or
  Fuusoku slides and dries like the rain's (nothing new lands), up to 0.11 / 0.0002 = 550 frames; at speed the drops
  slide off the view within a second or two. Under water the overlay 0x92340 is drawn over the rain (0x93b40).
- The NMS scene's camera (0x99360, rain 0x5200e8, one player) instead forces raining -> 90 / 70 / 12 and +0x34d = 0
  (12 -> 0 while paused 0x388f46), dry -> 0 and +0x34d = 1 every frame (so its lens is cleared each frame before
  the +0x34f loop 0x99e30), and draws through 0x98ebf. The view's +0x32c is the velocity (m/s, from the target's
  +0x2c; zeroed for fixed cameras 0x98e67), +0x338 the view's back axis.
- Per frame, 0x91890(rain, s = (velocity . back) x -1e-4, eye, velocity, back, raining); the slide +0x18 = max(s, 0)
  x 0.2 (forward speed x 2e-5), clamped +-2000:
  - Streaks (0x90f00, not in replays 0x52afac nor covered, double buffered): 1000 lines (FVF 0x42, no texture),
    centre = eye - 16 back + ((rand & 0xfff) - 0x800) / 128 per axis (+-16 m), h = velocity x -0.015 + (0.5, -2,
    0.5); head = centre + h + ((rand & 0xff) x 0.0125, 0, (rand & 0xff) x 0.0125) in 0x808080 with alpha
    round(+4 x 0.5) = 35, tail = centre - h with alpha 0. Covered: the last buffer is drawn again.
  - Lens drops (+0xd4 pos x, y, size; +0xd8 velocity): a drop with size > 0.08 slides: off = (pos - view centre) x
    slide x 0.6; 4 / 16: vel = clamp(off x 10, +-2); else 1 / 4: vel += ((rand & 0xff) / 255 - 128) x slide x 0.6
    per axis (the constants' order makes it about -128; kept); 13 / 16: pos += vel; run angle = atan2(vel.x,
    vel.y) deg, stretch = (|off.x| + |off.y|) x 0.6 (max 2). Smaller drops: angle and stretch 0. Every drop dries
    0.0002 a frame; a drop at size <= 0.03 or off the view (0x91820) comes back (0x90740: x, y = (rand & 0x1ff) /
    511 of the view, size (rand & 0xff) / 255 x 0.08 + 0.03; up to +0xc a frame, only raining and not covered).
    Shake: round(slide x 0.6 x ((rand & 0xff) - 128)) pixels each axis. Starts and clears dry (0x90d60: x -1000).
  - Drop shape (0x910e0): a fan of 8 triangles (FVF 0x144, white, depth off), centre texel = its position; rim k at
    (45 k + angle) deg, radius (shape[k] x stretch + 1) x size x 64 px with shape 0x1b3c90 = (0, -0.15, -0.4, 0.15,
    1, 0.15, -0.4, -0.15) (point 4, behind the run, is the tail); its texel = centre - the same direction x size x
    256 (- 0x298ed4 / 8, not traced): the echo texture 0x50e160 (this frame's 3D view, clamped) magnified x 4 and
    turned over in each drop.
  - Haze (0x92270, raining, not covered, +8 > 0): data\hud\raincloud.dds (128 x 128, white, alpha noise 0..224)
    over the view, texels (+0xcf0 x 0.125, +0xcf0 x 0.25) + 0.25; colour 0xa0a0a0 - (n << 24) with n = round((+0x14
    x 4000 x slide + 20) x +8 x -0.01): alpha about 18 standing, 40 at 300 m/s. Then (not in replays) +0xcf0 -= 0.06
    (wrap + 1024) and +0x14 (1.0 at the start) steps 0.04 at random within 0.3..1.
  - Spray: the sprite pass's list A (0xbcfa0, called by 0xbd8f0 with the view's eye +0x36c and its index +0x450)
    tests each used blended quad: at the first whose spray flag (0x425060[i], 0xbd670's 9th argument: 1 for the
    wake's puffs 0x9d6f2 and droplet sheets 0x51100 -> 0x9d2c0) is set and whose largest axis distance max(|dx|,
    |dy|, |dz|) from the eye (vertex 0's pos) is under 2 m (0x1a3534), with one player, it calls 0x917e0(rain
    [index], 20) once per pass: dried or strayed drops come back (0x90740) until more than 20 have, so 21. Raining
    or not, every frame the spray is that close (the camera behind a craft in another's wake).
- Sound (the internal camera's update 0x5aa10, player 0, not in replays): when the craft's AI node (race slot +0 ->
  nodes 0x1b9bc8) has flag 0x80 (`env_inside`, ai_<t>.txt) the loop stops (0xc42e0(0): every voice of sound 3 / 4
  fades out over 0.5 s, 0x1a34c0), otherwise it starts on a rain track: sound 4 SF_GENERIC_RAINGROUND
  (rain_ground_1) when [[0x4484d8 + 0xa54] + 0x54] != 0, else sound 3 SF_GENERIC_RAINWINDSHIELD (rain_windshield;
  that setting is not traced). Only the internal camera's update calls it.
- Thunder / lightning: none. SF_BRACKEN_THUNDER is a known name (sound 7 of the track list parser 0xde3c9) and
  audio\tracks\bracken\thunder (thunder2.wav, ROUTETO) exists, but bracken.txt never lists it, so it is not loaded;
  no flash was found (+0x14 only drives the haze's alpha).
- Argon (logic\rain.hpp, app_rain.cpp): raceRains at the race load (startRaceWeather), the rain stepped with the
  60 Hz race (stepRain: the eye camPos, the velocity the player's craft's, covered = the craft's AI node has
  env_inside, as the sound does; the visual +0x350 source is not traced; under water = cameraUnderWater, the depth
  at camPos <= 0), drawn after the echo copy on every track (drawAfterEchoCopy -> drawRain: the streaks projected to
  2D, without the depth test; the drops over menuFeedback; raincloud.dds with a wrapping sampler), then the
  underwater overlay when the eye is under water or with Atlantis Mode. Rain::setWeather clears the lens on diving
  and wets it all on surfacing; Rain::splash (21 drops) runs when any of argon's spray puffs or droplet sheets
  (all crafts') is within 2 m of camPos (sprayReachesLens), tested in the 60 Hz step rather than the draw. The sound
  plays rain_windshield from the internal tripod's update (0x5aa10), which runs every frame whatever the view (from the
  craft update 0x5dae3), stopped at once (no fade). Not ported: the replay
  freeze, the NMS scene's per-frame clear (no NMS scene); split screen has no rain in the game either.

## Rain splashes on the ground: none in the code
No effect places splashes where the rain lands; the rain is the streaks, the lens drops and the haze above. The
search (2026-11-16):
- The raining state reaches nothing else. 0x52afa2 is written only by the race load (0x800c6 / 0x800cf) and read only
  by camera_set_eye (0xae75b) and the NMS camera (0x99d1c); the camera's +0x34c only by the two rain steps (0x93b16,
  0x98e95). Always Rain 0x52c8b8 is read by the race load (0x800b3), the rain sound (0x5ad37 / 0x5adf3) and cleared
  by the front end (0x30ba0). The track id 1 / 0xd test (0x52ccf8 followed by cmp 0xd) is only at 0x800ac, 0x5ad29
  and 0x5ade6 (the front end's 0x31f4e / 0x31f6a set the id from the track name). The rain objects (0x51cce8 +
  0xd00 i, 0x5200e8) are used only by the rain code, the race / NMS views, the lens clears 0x3ed28.. and 0x7f5c6,
  and the spray splash 0xbd0a7.
- rain.cpp (0x90880..0x92560) holds only the functions above: 0x90740, 0x90810, 0x90880, 0x90d60, 0x90f00,
  0x910e0, 0x917a0, 0x917e0, 0x91820, 0x91890, 0x92270 and the overlay 0x92340; the next file is CScene (0x92560).
- The race view's draw (0x93490) after the track: the crafts' effects ([+0x850] vtable [1]), the opaque list, the
  water, the shadows, the translucent list, the trail ribbons, the sprite pass 0xbd8f0, the three particle systems
  (starburst, starburstred, bubble), the speed echo, the echo copy, 0xbc200, the rain 0x91890, the overlay 0x92340.
  The view's frame (0x93e20): the track, the pickups, the crafts, the explosions 0x7de80, the rain's copies and the
  underwater bubbles (0x9420c). Nothing else is drawn per view.
- Every sprite pool add (0xbd670: weapons, the trail sprites 0x9d000, droplets 0x9d2c0, puffs 0x9d620) and particle
  add (0xbcb60) caller is listed under "Splash on entering water" above; 0x9d000's callers are weapons (0x66606..
  0x6979d), damage smoke (0x6ce96 / 0x6cfe4), the explosion (0x7df66) and the hover dust (0x5f4e6 / 0x5f662). None
  reads the raining state, and none places sprites around the camera.
- Textures: no file in game_data is named splash / ripple / drop / puddle; the XBE names no such texture (ASCII or
  UTF-16; the hud loads are by full name or tex%d / tex0%d). data\hud\dust.dds, radial, sonicring and unknown have
  no string. Kalau's envdata\quadsprite.dds (the same as alt_kalau's; Bracken's is shared with alt_bracken) has no
  splash cell (its lower half is a grey checker), its 38 track textures none, and its model.cmm no rain / splash
  mesh or bone. data\testdata\waterbump000..059 (32 x 32 normal maps) are the water on every track, not rain-only.
- What may read as splashes on Kalau: the hover dust on its 28 rough meshes (the same on alt_kalau, dry), and the
  streaks' ends where they meet the track: the streak draw sets no depth state of its own (0x91967..: blend 1, depth
  writes off), so it keeps the depth test the sprite pass left on (the particle draw 0xbc900 between them is not
  traced for it), while argon draws them projected to 2D without the test.
