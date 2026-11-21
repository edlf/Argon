# Front end: easter eggs and cheats

## Easter Eggs and Cheats
- FE OPTIONS rows 7 Easter Eggs (y 310) and 8 Cheats (y 340): Easter Eggs shows with the cheat menu flag 0x1bbc26 or
  any egg unlocked, Cheats with 0x1bbc26 (0x2d9d0).
- FE EASTEREGGS (vtable 0x19d928: build 0x1ec40, enter 0x1f390, input 0x1f180, update 0x1f440, values 0x1f5e0,
  ticker 0x1f7d0). Title fe 0x314; hint Back only. Rows (62, 100 + 20 i): the 8 eggs fe 0x30c + i (Ultra Focal
  Length, Hyper Blur, Atlantis Mode, Slide, Always Rain, Weapons Off, Fruity Fruit, Mirror Tracks), "????" 0x17f grey
  while locked (0x1bbc08 + i); the value at 462: "Locked" 0x315 grey, else Off (fe 6) at 462 and On (fe 5) at 512, the
  choice 0xff85dde5 and the other 0xff555555 (0x1bbc10 + i); Exit (8) at (62, 260). Left / right on an unlocked egg
  switches it; A on Exit / B: back to FE OPTIONS. The cursor row's choice flashes grey 255 -> 64 (-16 a frame).
  Ticker: 0x317 locked ("Win Expert tournament or above with "bottom row" characters..."), fe 9 (empty) unlocked,
  0x33b on Exit. Unlocking: profile_tournament_won 0x84160 gives the next locked egg for an Expert-or-above win with a
  bottom-row character (the exact rule is in that function).
- FE CHEATMENU (vtable 0x19db38: build 0x19f80, enter 0x1abd0, input 0x1a5e0, update 0x1b030, row 0x1b1a0, ticker /
  hints 0x1b230). Title fe 0x300 "Cheat Menu". Rows (62, 100 + 20 i): the 7 cheats fe 0x2f8 + i (Unlock all
  characters / speeds / tracks / easter eggs, Fully upgrade all characters, Infinite Shield, Infinite Turbo; 0x2ff
  "AI Race" is not used), "????" grey while locked (0x1bbc18 + i); the value at 462: "Locked" 0x315, for 0..4
  Inactivated (0xb) grey / Activated (0xa) (0x1bbc1f + i), for 5 / 6 Off / On at 462 / 512; Enter Cheat Code (0x301)
  at (62, 240), Exit at (62, 260). A on an unlocked 0..4 activates it for good: 0 characters 0xffff, 1 speeds 4
  (0x1bb0e4 / 0x1bbc04), 2 tracks 0xffffffff + Mars, 3 all eggs, 4 all 16 characters' upgrades to 7 / 7 / 7, unlocked
  or not (0x1a83b: 0x1bb0ec + character x 0xb0, +0..+2; 0x19ed0, which does only the unlocked ones, is an entry of a
  function table at 0x4394a8 filled by 0xe7420 and is not called by the menus); left / right
  on an unlocked 5 / 6 switch 0x1bbbf0 / 0x1bbbf1. A on Enter Cheat Code: FE NAME ENTRY in code mode (title 0x304
  "Enter Code"); back in the enter function the typed text is compared (wcscmp) with fe 0x3d4 + i (the codes); a
  match unlocks that cheat. Ticker: locked 0x303, unlocked 0..4 0x302, 5 / 6 empty, Enter Cheat Code 0x375, Exit
  0x33b; right hint Select (0x32), Change Settings (0x4d) on 5 / 6, none while locked.
- The cheat menu: a profile name starting "CHEAT" (fe 0x4b1, 5 characters) runs 0x83af0 from name entry (0x3a561):
  0x1bbc26 = 1, the settings back to their defaults and the custom playlist cleared.
- Argon (ui\eggs.cpp, the unlocks app_unlocks.cpp): pages 17 / 18; the profile JSON keeps "eggs" / "eggsOn" / "cheats" / "cheatsOn" (bit masks)
  and "cheatMenu"; the CHEAT name and the codes work as in the game; `--unlock-all` unlocks every egg and cheat and
  shows both rows (the profile's own unlocks are kept). The eggs' and cheats' race effects: "Easter eggs and
  cheats in a race". Cheat 4 as decoded (ProgressKeeper::activateCheat). Not ported: the menu sounds, the code-entry
  page's ticker (the name-entry one shows).

## Easter eggs and cheats in a race
- Switching on: the race load (LOADER 0x80123 -> 0x792f0 with CL = 1, object 0x5293b0) clears seven race flags
  0x52c8b8..0x52c8be and, unless the game mode (+0x394c) is 4 (multiplayer), copies the eggs that are on (0x1bbc10 +
  i) into them; 0x52abe3 (weapons on) = !Weapons Off. main_menu 0x30af0 clears them all again (0x52abe3 = 1) on the
  way back to the front end, so they only hold in a race.
- 0 Ultra Focal Length -> 0x52c8b9: the view camera's update (0xa33d0, vtable 0x19b9d8: the four per-player views
  0x50e678 + 0x480 i, built at 0x107a90) multiplies its field of view by 1.8 (0x3fe66666; 1.0 without) before
  camera_set_fov 0xae550(pi x fov / 180 x 1.8): every race view, chase and TV cameras alike, 60 -> 108 deg.
- 1 Hyper Blur -> 0x52c8ba: the race draw's motion echo (0x93490 at 0x93928, only with one player: 0x52cd8c = 1, set
  by FE MAIN's pick "AppState - setting num players", Ghidra's label global_rain_type is wrong; not in replays 0x52afac):
  normally alpha = clamp((velocity . forward - 170) x 1.5, 0, 120) above 200 m/s and the previous frame drawn back
  zoomed (fe_draw_echo_quad 0xa6f00, zoom alpha x 0.1); with the egg, at any speed, a = clamp((v . f - 50) x 1.4, 0,
  190), alpha = (a + 20) - (a + 20) / 4 (15..158, never 0) and the menus' swirl fe_feedback_swirl 0xa7080 with zoom
  0x52b04c (0) + 1 and turn 0.2: the echo drawn twice, scaled 1 + 1 / 320 and turned by +-0.2 deg.
- 2 Atlantis Mode -> 0x52c8bb: the race drawn as under water:
  - no sky: 0x936c9 skips the sky model's draw (0xb80f0 / 0xb83e0 on +0x1f18) after the black clear (0x93777);
  - the track's fog replaced after track_load_prop_dat (0xb0453): colour (0x30, 0x60, 0x40), density 0.005
    (0x3ba3d70a), mode 1 (0x390fa8.., 0x390fb4, 0x391118);
  - bubbles (scene_viewport 0x94150, every frame): 30 bubble particles (0x1baad8) at the camera (+0x36c) + ((rand &
    255) - 127) x (0.5, 0.1, 0.5), velocity (0, 0.1, 0), life 0.8, colour 0xffffffff -> 0 from 0.64 s (the bubble
    system holds 1000);
  - the underwater overlay (0x92340, also drawn while the camera is under water, +0x34e; called at 0x93b64 and
    0x98edc): after the echo copy, the echo (this frame's 3D view) is drawn back through the view's 9 x 9 vertex grid
    (built by the rain object's init 0x90880: x0 + w / 8 i, y0 + h / 8 j, colour 0xff507060, 8 x 8 cells), each
    vertex's texel = its position + 8 x (sin, cos)(t + i + j) (0x1a3594 = 8; t the game clock 0x50e430 + 0x50e434):
    the view wobbling and tinted green-blue. The older note "the camera's +0x344 = 1 with the fog colour +0x1a0 =
    0xff306040 (0x98ecf)" was not re-checked (0x98ec4 is the overlay's second call site).
- 3 Slide -> 0x52c8bd: craft_side_friction (0x5ffd0, the player's craft only; 0x600c4): the sideways grip k' = (101 -
  craft +0xa18) x 0.0099 x k becomes a fixed 0.1 (0x1a3508); the blend towards 2 below forward speed 100 still applies,
  so the craft slides sideways at speed ([physics/README.md](../physics/README.md) side friction).
- 4 Always Rain -> 0x52c8b8: the loader (0x800b3) sets the rain flag 0x52afa2 as on Kalau Wastelands (track 1) and
  alt_bracken (0xd; not Kalau's second route, which is 9); the rain (streaks, drops on the lens, the raincloud.dds
  haze) and the rain sound (off where the craft's AI node has env_inside) follow it: [effects/rain.md](../effects/rain.md) "Rain".
- 5 Weapons Off -> 0x52abe3 = 0: gamehud_setup (0x47816) and the HUD modes (0x4964e) hide only the power-up panel
  (+0x3950; the shield and the score stay); craft_update (0x7190f) skips the fire buttons (+0x91c / +0x91d / +0x91e);
  drone_update (0x6e020) skips the AI's weapon use. Pickups are still placed (not checked whether they can be
  collected); whether the player's lock-on (0x6c420, called from 0x6d045) still runs is not checked.
- 6 Fruity Fruit -> 0x1bbc16 read directly by pickups_init (0x3e527): the pickup models become the _ee set
  (objects\powerup_blue_ee / red_ee / yellow_ee, powerchip_ee, megaup_ee and their _ns variants: the names at
  0x1a1f10..0x1a1fa8; all in game_data\data\objects).
- 7 Mirror Tracks -> 0x52c8bc: the projection's x is negated (camera_set_projection 0xae5c0, camera_set_fov 0xae550 at
  0xae5a2: the whole 3D view mirrored, culling flips with it), the player's steering inputs are swapped (player_pad_input
  0x71306: +0x908 <-> +0x90c, +0x910 <-> +0x914) and the minimap is drawn mirrored (hud_minimap_draw 0x4f090: at
  0x4f341 the map's left and right corners swap on screen, at 0x4f6db the icons' angles are negated).
- Cheat 5 Infinite Shield (0x1bbbf0): weapon_damage 0x6b02c returns before the shield is reduced when the hit craft's
  +0x794 is 0 (the player's craft; the AI crafts take damage). Cheat 6 Infinite Turbo (0x1bbbf1): craft_turbo 0x5e2ea
  does not drain the turbo seconds ([physics/turbo-flaps-cameras.md](../physics/turbo-flaps-cameras.md) "Turbo").
- Argon (logic\race_eggs.hpp RaceEggs, App::switchOnRaceEggs at the race load (beginRaceLoad), switchOffRaceEggs in
  openMenu; `--egg <i>` switches one on for tests): ported Ultra Focal Length (driveCamera), Hyper Blur
  (drawHyperBlur, the swirl shared with the menus: ui\feedback_swirl.cpp), Atlantis Mode (no sky, black clear, the fog
  in startLoadedRace, the bubbles Particles::emitBubbles, the overlay drawUnderwaterOverlay over the echo copy), Slide
  (GameCraft::setSlide), Weapons Off (App::weaponsOn = 0x52abe3: no locks, fire or AI weapons, the power-up panel
  hidden), Fruity Fruit (fruityPickupFolder), Mirror Tracks (the projection, sceneFrontFace, mirrorSteering, the
  minimap), Infinite Shield (weapon_hits.cpp), Infinite Turbo (stepDrive -> setTurboAllowed) and Always Rain
  (App::startRaceWeather -> argon::rain::raceRains, app_rain.cpp). The underwater overlay and bubbles only come with the egg (argon
  has no underwater camera). To compare with the game: Atlantis' tint and wobble (the overlay's blend / texture-stage
  states were not decoded: drawn as texture x vertex colour, opaque), Hyper Blur's strength, the minimap mirror.
