# XBE data: values read off instructions

## Values read off instructions

| code address | where | definition |
|---|---|---|
| 0xae550 | src/app/app.hpp:517 | `static constexpr float kDeathCameraFov = 72.f; // 0xae550 (pi x 0.4)` |
| 0xa33d0 | src/app/app_drive.cpp:10 | `constexpr float kUltraFocalLength = 1.8f;` |
| 0x3de29 | src/app/app_drive.cpp:114 | `const auto &st = craftPoses.startOfSlot(0);   // the player starts last (0x3de29)` |
| 0x7e5e0 | src/app/app_ui.cpp:90 | `const bool playerOneOut = weapons.craft(0).knockedOut() && !playerPads.multiplayerRace();   // split screen: each HUD checks its own craft` |
| 0x3de29 | src/app/race_craft_poses.cpp:49 | `const argon::StartSlot &RaceCraftPoses::startOfSlot(int slot) const` |
| 0xc2f90 | src/app/race_sounds.cpp:10 | `constexpr int kIncomingFrames = 20;   // 0xc2f90: a target's warning holds 20 frames after its last call` |
| 0x7efd0 | src/app/scene_frame.hpp:42 | `static constexpr int kMaxViews = 4;   // split screen: up to four views a frame (0x7efd0)` |
| 0x67231 | src/app/weapon_explosions.cpp:16 | `const int kCraftHitSparks = 250, kImpactSparks = 50;   // 0x67231, 0x67e81` |
| 0x65f80 | src/app/weapon_replays.cpp:13 | `constexpr int kImpactOnCraft = 1, kImpactBounced = 4;   // 0x65f80's kinds (weapon_shots.cpp)` |
| 0x68620 | src/app/weapon_shots.cpp:14 | `const float kBeamHitRadius = 5.f;      // 0x68620: weapon_craft_hit_test(.., 5.0)` |
| 0xc3220 | src/audio/engine_voices.cpp:18 | `constexpr float kOtherMinDist = 50.f, kOtherMaxDist = 200.f;   // 0xc3220` |
| 0xc47de | src/audio/mix_paths.hpp:17 | `constexpr float kVoice3dMb = 0.f;` |
| 0xc484e | src/audio/mix_paths.hpp:23 | `constexpr float kSpeakerFilesMb = -1200.f;` |
| 0xca540 | src/audio/mix_paths.hpp:26 | `constexpr float kSceneAmbienceMb = -500.f;` |
| 0xc473c | src/audio/mix_paths.hpp:28 | `constexpr float kEngineMb = -700.f;` |
| 0xdf854 | src/audio/mix_paths.hpp:30 | `constexpr float kMusicMb = -700.f;` |
| 0xc498c | src/audio/mix_paths.hpp:70 | `constexpr float kCentreCopyGain = 0.5623f;` |
| 0xc3220 | src/audio/spatial.cpp:13 | `constexpr float kDopplerFactor = 3.f;   // 0xc3220: SetDopplerFactor(3) on the engine buffers` |
| 0xe0f40 | src/craft/ai_pilot.cpp:76 | `const uint32_t flags = flags1_ \| flags2_;` |
| 0xe0810 | src/craft/ai_pilot.cpp:79 | `const float blend = targetBlend();` |
| 0x504a0 | src/craft/craft_flaps.cpp:22 | `const FlapRule kSharedFlaps{{"AIRBRAKE_LEFT", "AIRBRAKE_RIGHT"}, 0.005f, true, false, true, 8.f, 4.f, 0.f, 0.f, 1.f, {kPitchDown, kPitchDown}};` |
| 0x563b0 | src/craft/craft_flaps.cpp:24 | `const FlapRule kZafirFlaps{{"AIRBRAKE_LEFT", "AIRBRAKE_LEFT1"}, 0.005f, true, false, true, 8.f, 4.f, 0.f, 0.f, 1.f, {kPitchDown, kPitchDown}};` |
| 0x576c0 | src/craft/craft_flaps.cpp:26 | `const FlapRule kVladimirFlaps{{"AIRBRAKE_LEFT", "AIRBRAKE_RIGHT"}, 0.003f, true, false, true, 8.f, 4.f, 0.f, 0.f, 1.f, {kYawLeft, kYawRight}};` |
| 0x57990 | src/craft/craft_flaps.cpp:28 | `const FlapRule kYuriFlaps{{"AIRBRAKE_LEFT", "AIRBRAKE_RIGHT"}, 0.009f, false, false, true, 8.f, 4.f, 0.001f, -0.9f, 0.9f, {kPitchUp, kPitchUp}};` |
| 0x51d40 | src/craft/craft_flaps.cpp:30 | `const FlapRule kAbiFlaps{` |
| 0x54b90 | src/craft/craft_flaps.cpp:33 | `const FlapRule kKumikoFlaps{{"AIRBRAKE_MAIN", nullptr}, 0.01f, false, true, false, 5.f, 5.f, 0.f, 0.f, 1.f, {kPitchDown, kPitchDown}};` |
| 0x58e60 | src/craft/craft_flaps.cpp:36 | `const FlapRule *const kRuleByPilot[16] = {&kSharedFlaps,` |
| 0x70340 | src/craft/craftsim.cpp:92 | `const float hx = 2.f, hy = 1.f, hz = 3.f;` |
| 0x5f377 | src/craft/craftsim.hpp:284 | `const std::vector<DustPuff> &dustPuffs() const` |
| 0x50960 | src/effects/engine_trails.hpp:28 | `static constexpr int kPoints = 96;   // 0x50960 / 0x9db10: the points of a ring` |
| 0x9d620 | src/effects/wake_spray.cpp:18 | `const glm::vec2 kPuffCell(0.f, 384.f), kDropCell(128.f, 384.f);   // 0x9d620 / the wake's 0x9d2c0 (cell 13)` |
| 0x9d000 | src/effects/weapon_sprites.cpp:10 | `const float kCellPixels = 128.f;                   // 0x9d000 / 0x68150 / 0x68620: 0xbd670's cell size` |
| 0xb2670 | src/hud/game_hud.cpp:36 | `constexpr double kVblanksPerSecond = 60.0;   // 0x439df0, counted by the vertical blank callback 0xb2670` |
| 0x6aba0 | src/hud/game_hud.cpp:534 | `const bool up = sources.weapons.shieldGuard(slot()).blocked;   // 0x6aba0: a live class 2 weapon with shield-damage resistance` |
| 0xc16f0 | src/hud/hud_painter.cpp:136 | `const int frame = int(ImGui::GetTime() * 60.0) % 1000;` |
| 0x4dca1 | src/hud/markers.cpp:120 | `const int pilot = pilotInSlot(slot);   // 0x4dca1: the marked craft's character` |
| 0x49318 | src/hud/minimap_rules.cpp:13 | `constexpr std::array<const char *, 16> kMapByTrack = {"silver",` |
| 0x47362 | src/hud/minimap_rules.cpp:33 | `const auto index = static_cast<unsigned>(trackId);   // ja 0x47362: an unsigned compare, so -1 is past 0xf too` |
| 0xe1370 | src/logic/ai_routes.cpp:10 | `const RouteList kListByTurn[8] = {` |
| 0xe38c0 | src/logic/ai_routes.hpp:23 | `const int kNoSurface = -1;      // 0xe38c0: no ground and no water under the node` |
| 0x66eaa | src/logic/attack_warnings.hpp:13 | `constexpr float kProjectileWarningRange = 300.f;   // 0x66eaa: distance squared under 90000 (0x1a3a08)` |
| 0x68559 | src/logic/attack_warnings.hpp:14 | `constexpr float kPathWarningRange = 200.f;         // 0x68559 / 0x6857a (0x1a35a0)` |
| 0x707ed | src/logic/blast_push.hpp:44 | `constexpr float kPlayerSideScale = 1.f;   // +0x8f0, player 0x707ed` |
| 0x6d4a3 | src/logic/blast_push.hpp:45 | `constexpr float kDroneSideScale = 2.f;    // +0x8f0, drone 0x6d4a3` |
| 0x5b1d4 | src/logic/craft_weapon.hpp:86 | `const CraftEffects &effects() const` |
| 0x6ba30 | src/logic/craft_weapon.hpp:106 | `const PowerLevels &aiLoadout() const` |
| 0x9d000 | src/logic/explosion.cpp:19 | `const int kFlashCell = 15;                  // 0x9d000's EAX 0xf` |
| 0x9d000 | src/logic/explosion.cpp:21 | `const float kFlashGrowth = 0.f;             // 0x9d000's growth argument 0` |
| 0x7dd40 | src/logic/explosion.hpp:18 | `constexpr int kExplosionModels = 16;       // objects\explosion_type0..15 (0x7dd40); a type past 15 shows 15 (0x7e0f9)` |
| 0x6a8a0 | src/logic/explosion.hpp:87 | `constexpr int kKnockOutExplosionType = 6;` |
| 0x6b19a | src/logic/hit_flash.hpp:16 | `static constexpr float kPerHit = 0.8f;            // 0x6b19a: weapon_damage's amount, with shield damage out 0 > 0` |
| 0x6d332 | src/logic/hit_flash.hpp:17 | `static constexpr float kPerCorrodeFrame = 0.2f;   // 0x6d332: each frame the corrode eats a shield above 0 (+0x94 only)` |
| 0x9d000 | src/logic/hover_dust.cpp:20 | `const int kDustCell = 0;                // 0x9d000's EAX` |
| 0x5f377 | src/logic/hover_dust.hpp:16 | `constexpr int kFirstDustRay = 2, kSecondDustRay = 3;` |
| 0xd0c00 | src/logic/hull_contacts.hpp:25 | `constexpr int kMaxCandidates = 100;   // 0xd0c00` |
| 0xd439e | src/logic/hull_contacts.hpp:26 | `constexpr int kMaxContacts = 500;     // 0xd439e` |
| 0xdd4a0 | src/logic/hull_contacts.hpp:27 | `constexpr float kSpanSteps = 4.f;     // 0xdd4a0 / 0xdd5c0: the span lists keep bounds in 0.25 m steps (0x1a3444)` |
| 0x3b0b0 | src/logic/multiplayer_rules.hpp:10 | `constexpr int lapsForOption(int option, int defaultLaps)` |
| 0x63140 | src/logic/music_cues.hpp:9 | `constexpr bool finalLapStarts(int lap, int finalLap)` |
| 0x635c0 | src/logic/music_cues.hpp:21 | `constexpr bool takesTheLead(int previousPlace, int place)` |
| 0x6d000 | src/logic/pickups.cpp:11 | `const float kReach = 14.f;          // 0x6d000 passes 14 as the ship's radius` |
| 0x3b1b5 | src/logic/race_laps.hpp:12 | `constexpr int lapsForSpeedClass(int speedClass)` |
| 0x3b14e | src/logic/race_laps.hpp:23 | `constexpr int raceLaps(bool multiplayer, int lapsOption, int speedClass)` |
| 0x4cc30 | src/logic/race_points.hpp:20 | `static constexpr float kPopupSeconds = 0.8f;   // 0x4cc30: how long the shown popup lasts` |
| 0x5cb30 | src/logic/race_warnings.hpp:37 | `static constexpr float kResetTime = 0.5f;          // 0x1950a0, set by 0x5cb30` |
| 0x77508 | src/logic/rain.hpp:132 | `static constexpr int kDrops = 800, kStreaks = 1000;   // 0x77508, one player` |
| 0x31c10 | src/logic/replay.cpp:21 | `constexpr std::string_view kTrackNames[] = {"silver",` |
| 0x61c60 | src/logic/replay.hpp:122 | `constexpr float kEaseSeconds = 3.f, kEaseShare = 0.1f;` |
| 0x8f2e2 | src/logic/results_rules.hpp:40 | `constexpr bool showsPlaceNumber(bool outOfRace, bool finished)` |
| 0x2edef | src/logic/sound_rules.cpp:18 | `constexpr float kChingRate = 22050.f, kChingHighestRate = 0x2edef;` |
| 0xd0a30 | src/logic/sound_rules.cpp:225 | `const float normalDot = contact.normalDotSum / float(contact.count);   // 0xd0a30` |
| 0x45270 | src/logic/split_screen.hpp:31 | `constexpr ScreenRect hudFrame(const ScreenRect &view, float screenWidth = 640.f)` |
| 0x35e50 | src/logic/tourney_rules.hpp:23 | `constexpr float speedoNeedleDegrees(int level)` |
| 0xc96e0 | src/logic/travel_loops.hpp:43 | `static constexpr float kStartDelay = 0.5f;   // +0x9b8, set by the defaults 0xc96e0 only` |
| 0x82a70 | src/logic/upgrade_rules.hpp:23 | `constexpr int priceOfLevel(int level)` |
| 0x3d5a0 | src/logic/upgrade_rules.hpp:29 | `constexpr float lapTurboSeconds(int turboLevel)` |
| 0x66650 | src/logic/weapon_book.hpp:54 | `static constexpr int kFirstSpriteTrail = 4;   // trail types 1..3 point particles, 4.. sprites (0x66650)` |
| 0x6b6ee | src/logic/weapon_damage.hpp:50 | `constexpr float kMercyFromPercent = 33.f;` |
| 0x9d000 | src/logic/weapon_flash.cpp:20 | `const int kSpriteCell = 10;                   // 0x9d000's EAX` |
| 0x3f3d0 | src/logic/weapon_rules.cpp:16 | `const AiWeaponSetup kAiSetup[kSpeedClasses][3] = {` |
| 0x6d000 | src/logic/weapon_rules.hpp:43 | `const int kSpecialColour = 3;` |
| 0x6ba30 | src/logic/weapon_rules.hpp:57 | `const int kMegaUp = 3;` |
| 0x9aa00 | src/movie/story_scene.cpp:24 | `constexpr float kEndBefore = 0.1f;   // 0x9aa00: ends once the first track's length less 0.1 s has played (0x1a3508)` |
| 0x98c30 | src/movie/story_scene.cpp:27 | `constexpr float kBarTop = 56.f, kBarBottom = 424.f;` |
| 0xc1b40 | src/movie/story_scene.cpp:31 | `constexpr float kPanelY = 300.f, kPanelH = 100.f, kPortraitX = 10.f, kPortraitY = 305.f, kPortraitSize = 84.f;` |
| 0x9b170 | src/movie/story_scenes.cpp:18 | `const char *const kCharacterFolders[kPilots] = {` |
| 0x80000 | src/original/cmm_records.hpp:22 | `constexpr uint32_t kRuntimeModelClock = 0x80000;   // meshes animate on the model's clock (model +0x5e4)` |
| 0x20000 | src/original/cmm_records.hpp:34 | `constexpr uint32_t kHidden = 0x20000;` |
| 0x40000 | src/original/cmm_records.hpp:35 | `constexpr uint32_t kSkinned = 0x40000;` |
| 0xb7920 | src/original/cmm_records.hpp:37 | `constexpr uint32_t kIdentityTransform = 0x44000000;    // drawn with an identity world matrix (0xb7920)` |
| 0x20000 | src/original/cmm_records.hpp:50 | `constexpr uint32_t kLaidOver = 0x20000;         // sorted with key 0, last in its bucket (0xb02d1, 0xb7efb): Draco's sand` |
| 0x40000 | src/original/cmm_records.hpp:51 | `constexpr uint32_t kOcclusionTested = 0x40000;` |
| 0x80000 | src/original/cmm_records.hpp:52 | `constexpr uint32_t kParentIntensity = 0x80000;` |
| 0xb7e88 | src/original/cmm_records.hpp:53 | `constexpr uint32_t kFollowsCamera = 0x400;      // its world position is an offset from the eye (0xb7e88)` |
| 0xb7eca | src/original/cmm_records.hpp:54 | `constexpr uint32_t kFacesCamera = 0x800;        // turned to the eye (0xb7eca / 0xb78a1 -> 0xb4e70)` |
| 0x100000 | src/original/cmm_records.hpp:55 | `constexpr uint32_t kBillboard = 0x100000;       // slid towards the view axis by its factor (0xb7765, 0xbb270)` |
| 0xb7920 | src/original/cmm_records.hpp:56 | `constexpr uint32_t kSortedTranslucent = 0x800000;   // render list bucket 3, sorted by distance (0xb7920)` |
| 0xb6ea8 | src/original/cmm_records.hpp:57 | `constexpr uint32_t kShadowHull = 0x400000;      // the one mesh the ship shadow pass draws (0xb6ea8, 0xbe230)` |
| 0x423b0 | src/original/event_buffer_records.hpp:17 | `constexpr float kEventOrientationScale = 1.f / 4096.f;   // 0x1a3768 (0x423b0), 4096 = 0x1a3560 (0x582d0)` |
| 0x7dd40 | src/original/weapon_records.hpp:135 | `constexpr int kNwpExplosionTypes = 32;` |
| 0xb709e | src/render/flare_occlusion.hpp:21 | `constexpr float kUntestedIntensity = 0.f;` |
| 0xa8450 | src/render/mesh_fog.hpp:35 | `constexpr Curve curveOf(uint32_t mode)` |
| 0xcd4f0 | src/render/model_builder.cpp:494 | `const int d1 = h[j][(i + 1) % side] - h[j][i > 0 ? i - 1 : side - 1];` |
| 0xcd170 | src/render/model_builder.cpp:500 | `const argon::BumpBasis &vertexBasis = basis[size_t(j * side + i)];` |
| 0xbcfa0 | src/render/sprite_pool.hpp:47 | `inline constexpr int kSpriteTriangleCorners[kSpriteTriangleVertices] = {0, 1, 2, 1, 2, 3};` |
| 0xded40 | src/track/track_source.cpp:82 | `constexpr size_t kMarsMusicSegments = 5;` |
| 0xbdf40 | src/track/track_sprites.cpp:16 | `const size_t kBlendedSlots = 2000, kAdditiveSlots = 0x800;   // 0xbdf40 logs "Too many ... quadsprites" past these` |
| 0xbdf40 | src/track/track_sprites.cpp:19 | `const int kOffsetOfVertex[4] = {2, 3, 1, 0};` |
| 0xcdcf0 | src/track/trackdata.hpp:38 | `constexpr int kEnvCubeSlots = 8;` |
| 0x13790 | src/ui/awards.cpp:18 | `constexpr float kFrameX = 52.f, kFrameY = 195.f, kFrameW = 536.f, kFrameH = 170.f;   // 0x13790` |
| 0x128d0 | src/ui/awards.cpp:19 | `constexpr float kTrophyRowY = 200.f, kRowsY = 220.f, kRowStep = 20.f;                 // 0x128d0: row 0 moved to 200` |
| 0x13742 | src/ui/awards.cpp:21 | `const glm::vec3 kTrophyPosition(50.f, 70.f, -550.f);   // 0x13742: the trophy, rotation 0` |
| 0x14a50 | src/ui/charsel.hpp:67 | `static constexpr int kLinesShown = 5, kScrollLines = 4;   // xemu / 0x14a50` |
| 0xb11f0 | src/ui/credits.cpp:24 | `constexpr float kTextScale = 0.65f;       // 0xb11f0` |
| 0xb99f0 | src/ui/fe_scene.hpp:25 | `const char *turnedBone = nullptr;` |
| 0x34660 | src/ui/fe_sounds.hpp:66 | `inline constexpr PageSounds kTitle{.leftForward = UiSound::ScreenForward};` |
| 0x1c070 | src/ui/fe_sounds.hpp:70 | `inline constexpr PageSounds kChooseSave{.cursor = UiSound::Rollover, .back = UiSound::ScreenBack, .forward = UiSound::ScreenForward};` |
| 0x38f10 | src/ui/fe_sounds.hpp:74 | `inline constexpr PageSounds kNameEntry{.cursor = UiSound::Rollover, .pick = UiSound::Select, .back = UiSound::ScreenBack, .forward = UiSound::ScreenFo` |
| 0x28e90 | src/ui/fe_sounds.hpp:78 | `inline constexpr PageSounds kMain{` |
| 0x163b0 | src/ui/fe_sounds.hpp:84 | `inline constexpr PageSounds kCharSelect{.cursor = UiSound::Rollover,` |
| 0x14a50 | src/ui/fe_sounds.hpp:93 | `inline constexpr PageSounds kInfo{.back = UiSound::ScreenBack};` |
| 0x1da10 | src/ui/fe_sounds.hpp:98 | `inline constexpr PageSounds kCourseSelect{.cursor = UiSound::Rollover,` |
| 0x35940 | src/ui/fe_sounds.hpp:106 | `inline constexpr PageSounds kTourneyType{.cursor = UiSound::Rollover, .refused = UiSound::BadSelect, .back = UiSound::ScreenBack};` |
| 0x33ac0 | src/ui/fe_sounds.hpp:110 | `inline constexpr PageSounds kSpeedSelect{.cursor = UiSound::Rollover, .pick = UiSound::Select, .refused = UiSound::BadSelect, .back = UiSound::ScreenB` |
| 0x23b40 | src/ui/fe_sounds.hpp:114 | `inline constexpr PageSounds kGhostSelect{` |
| 0x20a60 | src/ui/fe_sounds.hpp:120 | `inline constexpr PageSounds kRecordTable{.cursor = UiSound::Rollover, .pick = UiSound::Select, .back = UiSound::Select, .leftBack = UiSound::ScreenBac` |
| 0x22000 | src/ui/fe_sounds.hpp:124 | `inline constexpr PageSounds kGhostManage{.cursor = UiSound::Rollover, .leftBack = UiSound::ScreenBack};` |
| 0x2cef0 | src/ui/fe_sounds.hpp:127 | `inline constexpr PageSounds kOptions{.cursor = UiSound::Rollover, .back = UiSound::ScreenBack, .leftForward = UiSound::ScreenForward};` |
| 0xac040 | src/ui/fe_sounds.hpp:132 | `inline constexpr PageSounds kSettings{` |
| 0x2bd80 | src/ui/fe_sounds.hpp:137 | `inline constexpr PageSounds kMusicSelect{` |
| 0x27380 | src/ui/fe_sounds.hpp:141 | `inline constexpr PageSounds kLevelMusic{.leftBack = UiSound::ScreenBack};` |
| 0x1f180 | src/ui/fe_sounds.hpp:144 | `inline constexpr PageSounds kEasterEggs{.cursor = UiSound::Rollover, .back = UiSound::ScreenBack};` |
| 0x1a5e0 | src/ui/fe_sounds.hpp:148 | `inline constexpr PageSounds kCheatMenu{.cursor = UiSound::Rollover, .pick = UiSound::Select, .refused = UiSound::BadSelect, .back = UiSound::ScreenBac` |
| 0x2e110 | src/ui/fe_sounds.hpp:151 | `inline constexpr PageSounds kCredits{.leftBack = UiSound::ScreenBack};` |
| 0x133c0 | src/ui/fe_sounds.hpp:154 | `inline constexpr PageSounds kAwards{.leftForward = UiSound::ScreenForward};` |
| 0x2b4b0 | src/ui/fe_sounds.hpp:158 | `inline constexpr PageSounds kMessageBox{.cursor = UiSound::Rollover, .pick = UiSound::Select};` |
| 0x9ae05 | src/ui/loader.cpp:26 | `constexpr int kStoryTailFrames = 12;   // 0x9ae05: the scene's page pops 12 frames after it ended (0x439f78 > 12)` |
| 0xa7080 | src/ui/menu.cpp:59 | `const FeedbackSwirl kMenuSwirl{15.f, 8.f, 0xa0ffffffu};` |
| 0x2ad70 | src/ui/menu_common.hpp:159 | `constexpr uint8_t stepFlash(uint8_t grey)` |
| 0x2ffa0 | src/ui/menu_common.hpp:175 | `inline constexpr float kTickerSpeed = 2.f * 60.f;` |
| 0x85f10 | src/ui/multiplayer_end.cpp:21 | `constexpr float kTimeX = kRight - 325.f + 5.f, kLapX = kRight - 210.f + 5.f, kWinsX = kRight - 47.5f + 5.f;` |
| 0x95b00 | src/ui/score.cpp:21 | `constexpr float kCountingScale = 1.5f, kCountingY = 215.f;   // 0x95b00 while the score climbs` |
| 0x9f788 | src/ui/upgrade.cpp:25 | `constexpr float kInsufficientFundsWrap = 262.f;   // 0x9f788: the text's wrap width` |
| 0x9f45e | src/ui/upgrade.cpp:28 | `constexpr float kIconX = 430.f, kIconY = 150.f;   // 0x9f45e: panel x + 272 (0x1a390c) - 138 (0x1a3908)` |
| 0xb1230 | src/ui/upgrade.cpp:210 | `const std::vector<std::u16string> lines = menus.wrapText(strings.race(0x516, "Insufficient Funds"), 0, kFe, kInsufficientFundsWrap);` |
