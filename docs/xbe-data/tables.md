# XBE data: tables and constants

## Tables and constants in the XBE's data

| address | section | where | source line |
|---|---|---|---|
| 0x1459d8 | DSOUND | src/audio/hrtf.cpp:17 | `constexpr uint32_t kFiltersVa = 0x1459d8, kIndexVa = 0x156f98;` |
| 0x1950a0 | .rdata | src/logic/race_warnings.hpp:37 | `static constexpr float kResetTime = 0.5f;          // 0x1950a0, set by 0x5cb30` |
| 0x195150 | .rdata | src/ui/title_texts.hpp:24 | `inline constexpr TitleText kPressStart{1, "Press START to begin", 320.f, kPressStartColour};` |
| 0x198d94 | .rdata | src/render/bump_basis.cpp:13 | `constexpr float kSmallestDeterminant = 1e-12f;   // 0x198d94` |
| 0x19bef4 | .rdata | src/movie/head_turn.cpp:14 | `const char *const kRightEye = "eyeR";    // 0x19bef4` |
| 0x19befc | .rdata | src/movie/head_turn.cpp:13 | `const char *const kLeftEye = "eyeL";     // 0x19befc` |
| 0x19bf04 | .rdata | src/movie/head_turn.cpp:12 | `const char *const kNeck = "_neck";       // 0x19bf04` |
| 0x1a29cc | .rdata | src/movie/story_scenes.cpp:18 | `const char *const kCharacterFolders[kPilots] = {` |
| 0x1a2f98 | .rdata | src/movie/head_turn.cpp:11 | `const char *const kHead = "skel_head";   // 0x1a2f98` |
| 0x1a3444 | .rdata | src/logic/hull_contacts.hpp:27 | `constexpr float kSpanSteps = 4.f;     // 0xdd4a0 / 0xdd5c0: the span lists keep bounds in 0.25 m steps (0x1a3444)` |
| 0x1a3444 | .rdata | src/ui/nemesis_intro.cpp:12 | `constexpr float kTurnSeconds = 4.f;     // 0x1a3444: the copies settle by then` |
| 0x1a344c | .rdata | src/logic/replay_director.hpp:48 | `constexpr float kOffScore = -100.f;   // 0x1a344c: out of its lap window (or facing the wrong way)` |
| 0x1a3450 | .rdata | src/logic/ai_routes.cpp:12 | `const float kPercent = 100.f;   // 0x1a3450` |
| 0x1a3450 | .rdata | src/logic/replay_director.hpp:49 | `constexpr float kHeldScore = 100.f;   // 0x1a3450: a camera still held; a moving one in its window` |
| 0x1a3450 | .rdata | src/ui/shell_slide.hpp:18 | `static constexpr float kClearance = 100.f;   // 0x1a3450: past the element's own size` |
| 0x1a3454 | .rdata | src/logic/explosion.cpp:32 | `const float kImpactLife = 1.f;              // 0x1a3454` |
| 0x1a3454 | .rdata | src/logic/race_warnings.hpp:39 | `static constexpr float kWrongWayTime = 1.f;        // 0x1a3454: for longer than this` |
| 0x1a3454 | .rdata | src/logic/race_warnings.hpp:75 | `static constexpr float kRepeat = 1.f;      // 0x1a3454` |
| 0x1a345c | .rdata | src/logic/water_waves.hpp:22 | `static constexpr float kShortToUnit = 1.f / 32768.f; // a stored short to a unit height (0x1a345c)` |
| 0x1a3460 | .rdata | src/logic/water_waves.hpp:21 | `static constexpr float kGridStep = 8.f;              // metres between vertices (0x1a3460 / 16; 0x1a3468 = 1 / 8)` |
| 0x1a34d8 | .rdata | src/logic/rumble_rules.cpp:12 | `constexpr float kKnockScale = 1000.f, kKnockMax = 65000.f;                 // 0x1a34d8, 0x1a3740` |
| 0x1a34d8 | .rdata | src/logic/shot_flight.cpp:14 | `const float kBeamReach = 1000.f;   // 0x1a34d8` |
| 0x1a34d8 | .rdata | src/movie/scene_light.cpp:12 | `constexpr float kLargest = 1000.f;   // 0x1a34d8` |
| 0x1a34f8 | .rdata | src/movie/scene_light.cpp:11 | `constexpr float kNearZero = 1e-6f;   // 0x1a34f8` |
| 0x1a34fc | .rdata | src/logic/explosion.cpp:17 | `const float kFullTurn = 360.f;              // 0x1a34fc` |
| 0x1a3500 | .rdata | src/logic/shot_motion.cpp:11 | `const float kMaxPitch = 90.f;   // 0x1a3500` |
| 0x1a3504 | .rdata | src/audio/music_fade.cpp:14 | `constexpr double kDipTurn = 0.5;          // 0x1a3504` |
| 0x1a3504 | .rdata | src/logic/camera_shake.cpp:18 | `constexpr float kJoltGain = 0.5f;             // 0x1a3504` |
| 0x1a3504 | .rdata | src/logic/explosion.cpp:26 | `const float kHalf = 0.5f;                   // 0x1a3504` |
| 0x1a3504 | .rdata | src/logic/weapon_flash.cpp:19 | `const float kSparkLead = 0.5f;                // 0x1a3504: the start ahead along the velocity` |
| 0x1a3504 | .rdata | src/track/track_sprites.cpp:15 | `const float kHalf = 0.5f;             // 0x1a3504` |
| 0x1a3504 | .rdata | src/ui/messagebox.cpp:17 | `constexpr int kBusyTicks = 30;   // open + 0.5 s (0x1a3504); the box acts once the clock is past it` |
| 0x1a3508 | .rdata | src/craft/craft_forces.cpp:21 | `static const float kSlideGrip = 0.1f;   // 0x1a3508, the Slide egg` |
| 0x1a3508 | .rdata | src/logic/camera_shake.cpp:13 | `constexpr float kAxisSize = 0.1f;             // 0x1a3508` |
| 0x1a3508 | .rdata | src/logic/explosion.cpp:27 | `const float kSparkLead = 0.1f;              // 0x1a3508` |
| 0x1a3508 | .rdata | src/logic/hover_dust.cpp:16 | `const float kSizeStep = 0.1f;           // 0x1a3508` |
| 0x1a3508 | .rdata | src/logic/replay_writer.hpp:18 | `constexpr float kPositionDrift = 0.1f;         // 0x1a3508: the dead-reckoned position this far off` |
| 0x1a3508 | .rdata | src/movie/story_scene.cpp:24 | `constexpr float kEndBefore = 0.1f;   // 0x9aa00: ends once the first track's length less 0.1 s has played (0x1a3508)` |
| 0x1a3518 | .rdata | src/logic/camera_shake.cpp:17 | `constexpr float kJoltMinNormalDot = 0.25f;    // 0x1a3518` |
| 0x1a3518 | .rdata | src/logic/weapon_flash.cpp:24 | `const float kSpriteSizeScale = 0.25f;         // 0x1a3518` |
| 0x1a351c | .rdata | src/logic/explosion.cpp:28 | `const float kCraftHitSpeed = 3.f;           // 0x1a351c` |
| 0x1a351c | .rdata | src/logic/explosion.cpp:29 | `const float kCraftHitLife = 3.f;            // 0x1a351c` |
| 0x1a351c | .rdata | src/logic/replay.hpp:122 | `constexpr float kEaseSeconds = 3.f, kEaseShare = 0.1f;` |
| 0x1a351c | .rdata | src/logic/replay_director.cpp:19 | `constexpr float kKerbPhaseRate = 3.f, kKerbAmplitude = 0.02f;              // 0x1a351c, 0x1a36d8 (sines x 15 / 13 / 10)` |
| 0x1a351c | .rdata | src/logic/rumble_rules.cpp:15 | `constexpr float kFadeSeconds = 3.f, kFadeSteps = 33.f;                     // 0x1a351c, 0x1a39b8` |
| 0x1a3530 | .rdata | src/logic/rain.cpp:12 | `constexpr float kNewDropSize = 0.03f, kNewDropSizeRange = 0.08f;   // 0x1a3530, 0x1a3544` |
| 0x1a3534 | .rdata | src/logic/hover_dust.cpp:17 | `const float kBaseSize = 2.f;            // 0x1a3534` |
| 0x1a3534 | .rdata | src/logic/rain.cpp:39 | `const float kReach = 2.f;   // 0x1a3534` |
| 0x1a3534 | .rdata | src/ui/nemesis_intro.cpp:13 | `constexpr float kSettledAt = 2.f;       // 0x1a3534: the yellow text from then` |
| 0x1a3534 | .rdata | src/ui/title.cpp:20 | `constexpr double kIntroSeconds = 2.0;        // 0x1a3534` |
| 0x1a3538 | .rdata | src/logic/hover_dust.cpp:10 | `const float kDriftScale = 0.0125f;      // 0x1a3538` |
| 0x1a3548 | .rdata | src/logic/explosion.cpp:25 | `const float kByteScale = 0.003921568859f;   // 0x1a3548: 1 / 255` |
| 0x1a3548 | .rdata | src/logic/weapon_flash.cpp:17 | `const float kByteScale = 0.003921568859f;     // 0x1a3548: 1 / 255` |
| 0x1a3550 | .rdata | src/logic/blast_push.cpp:12 | `constexpr float kMaxBraking = 0.4f;        // 0x1a3550` |
| 0x1a3550 | .rdata | src/logic/shot_flight.cpp:15 | `const float kBounceLift = 0.4f;    // 0x1a3550` |
| 0x1a3550 | .rdata | src/logic/shot_motion.cpp:14 | `const float kInheritedDecayPerSecond = 0.4f;   // 0x1a3550` |
| 0x1a3554 | .rdata | src/app/app_laprecords.cpp:9 | `const float kRepeat = 5.f;   // 0x1a3554` |
| 0x1a3554 | .rdata | src/logic/ai_trigger.cpp:11 | `const float kLineHalfWidth = 5.f;         // 0x1a3554: \|offset along the right axis\| < 5` |
| 0x1a3554 | .rdata | src/logic/replay_director.cpp:16 | `constexpr float kNarrowestFov = 5.f, kWidestFov = 90.f;                    // 0x1a3554, 0x1a3500` |
| 0x1a3558 | .rdata | src/logic/rain.cpp:15 | `constexpr float kSlideGain = 0.6f;                                  // 0x1a3558` |
| 0x1a355c | .rdata | src/logic/camera_shake.cpp:15 | `constexpr float kKnockShake = 0.01f;          // 0x1a355c` |
| 0x1a355c | .rdata | src/logic/camera_shake.cpp:21 | `constexpr float kJoltPitch = 0.01f;           // 0x1a355c` |
| 0x1a355c | .rdata | src/logic/explosion.cpp:12 | `const float kPercent = 0.01f;               // 0x1a355c` |
| 0x1a355c | .rdata | src/logic/shot_motion.cpp:13 | `const float kPercent = 0.01f;           // 0x1a355c` |
| 0x1a355c | .rdata | src/logic/weapon_damage.cpp:13 | `const float kPushRampFrom = 0.01f, kPushScale = 0.01f;   // 0x1a355c both` |
| 0x1a355c | .rdata | src/logic/weapon_rules.cpp:66 | `const double percentToFraction = 0.01f;   // 0x1a355c` |
| 0x1a3564 | .rdata | src/logic/explosion.cpp:18 | `const float kGreyFull = 255.f;              // 0x1a3564` |
| 0x1a3578 | .rdata | src/logic/results_rules.hpp:9 | `constexpr float kNoTime = 999999.f;   // 0x1a3578: no lap yet` |
| 0x1a357c | .rdata | src/ui/shell_slide.hpp:19 | `static constexpr float kTitleY = 36.f;       // 0x1a357c` |
| 0x1a358c | .rdata | src/logic/explosion.cpp:37 | `const float kKnockOutEchoStep = 0.2f;                                 // 0x1a358c` |
| 0x1a3594 | .rdata | src/render/speed_blur.cpp:72 | `const float kWobble = 8.f;   // 0x1a3594, texels of the 640 x 480 frame` |
| 0x1a3594 | .rdata | src/render/water_basis.cpp:13 | `constexpr float kGridStep = WaterWaves::kGridStep;   // also 0x1a3594` |
| 0x1a359c | .rdata | src/logic/ai_trigger.cpp:12 | `const float kLineRange = 500.f;           // 0x1a359c` |
| 0x1a359c | .rdata | src/logic/travel_loops.cpp:10 | `constexpr float kRadialMaxDistAdd = 500.f;   // 0x1a359c` |
| 0x1a35a0 | .rdata | src/logic/attack_warnings.hpp:14 | `constexpr float kPathWarningRange = 200.f;         // 0x68559 / 0x6857a (0x1a35a0)` |
| 0x1a35a0 | .rdata | src/logic/travel_loops.cpp:9 | `constexpr float kRadialMinDistAdd = 200.f;   // 0x1a35a0` |
| 0x1a35a4 | .rdata | src/app/app_knockout.cpp:17 | `const float kLoseMenuDelay = 10.f;   // 0x1a35a4` |
| 0x1a35a4 | .rdata | src/logic/explosion.cpp:23 | `const float kMinImpactFlash = 10.f;         // 0x1a35a4` |
| 0x1a35a4 | .rdata | src/logic/explosion.cpp:30 | `const float kImpactSpeed = 10.f;            // 0x1a35a4` |
| 0x1a35a4 | .rdata | src/logic/replay_director.cpp:14 | `constexpr float kTrackingRate = 10.f;                                      // 0x1a35a4` |
| 0x1a35a4 | .rdata | src/logic/replay_director.cpp:17 | `constexpr float kKerbReach = 10.f, kKerbScale = 0.05f;                     // 0x1a35a4, 0x1a36e4` |
| 0x1a35a4 | .rdata | src/logic/replay_director.cpp:18 | `constexpr float kKerbRise = 10.f, kKerbFall = 5.f, kKerbMostShake = 0.6f;  // 0x1a35a4, 0x1a3554, 0x1a3558` |
| 0x1a35a4 | .rdata | src/logic/sound_rules.cpp:20 | `constexpr float kLongestImpactDelay = 10.f;   // 0x1a35a4: a longer delay falls back to 0.25` |
| 0x1a35a4 | .rdata | src/logic/weapon_flash.cpp:11 | `const float kSparkHalfSize = 10.f;       // 0x1a35a4, + rand & 3` |
| 0x1a35a4 | .rdata | src/logic/weapon_flash.cpp:18 | `const float kSparkSpread = 10.f;              // 0x1a35a4, twice in starburst` |
| 0x1a35a4 | .rdata | src/logic/weapon_flash.cpp:22 | `const float kSparkSpriteSize = 10.f;          // 0x1a35a4, + rand & 3, x 0.25` |
| 0x1a35a4 | .rdata | src/track/ai_route_table.cpp:16 | `const float kProbeReach = 10.f;   // 0x1a35a4` |
| 0x1a35a4 | .rdata | src/ui/upgrade_icons.cpp:50 | `const double radians = double(frame) * 10.0 * 3.14159265358979 / 180.0;   // 0x5a211c (pi / 180) x 10 (0x1a35a4)` |
| 0x1a35a8 | .rdata | src/audio/music_fade.cpp:11 | `constexpr float kMbPerSecond = 10000.f;   // 0x1a35a8` |
| 0x1a35b0 | .rdata | src/render/sprite_pool.cpp:10 | `const float kCellInset = 0.97f;                           // 0x1a35b0` |
| 0x1a35b8 | .rdata | src/logic/explosion.cpp:40 | `const float kKnockOutReach = 20.f / 1024.f;                           // 0x1a35b8 x 0x1a35d8` |
| 0x1a35bc | .rdata | src/logic/explosion.cpp:15 | `const float kScatterScale = 1.f / 512.f;    // 0x1a35bc` |
| 0x1a35bc | .rdata | src/render/sprite_pool.cpp:9 | `const glm::vec2 kAtlasTexel(1.f / 512.f, 1.f / 1024.f);   // 0x1a35bc, 0x1a35b8` |
| 0x1a35cc | .rdata | src/logic/replay_director.cpp:13 | `constexpr float kLeadBySpeed[5] = {0.15f, 0.13f, 0.11f, 0.08f, 0.05f};   // 0x1a35cc .. 0x1a36e4 by global_game_speed` |
| 0x1a35d0 | .rdata | src/movie/head_turn.hpp:13 | `constexpr float kNeckShare = 0.3f;   // 0x1a35d0: the neck turns 0.3 of the head's share` |
| 0x1a35d8 | .rdata | src/logic/explosion.cpp:31 | `const float kImpactSpeedRange = 20.f;       // 0x1a35d8` |
| 0x1a35d8 | .rdata | src/logic/trail_sprite.cpp:12 | `const float kGrowthRate = 20.f;            // 0x1a35d8` |
| 0x1a35d8 | .rdata | src/logic/weapon_flash.cpp:23 | `const float kHitSpriteSize = 20.f;            // 0x1a35d8, + rand & 3, x 0.25` |
| 0x1a3604 | .rdata | src/logic/ai_trigger.cpp:13 | `const float kShieldBelowPercent = 40.f;   // 0x1a3604` |
| 0x1a3604 | .rdata | src/logic/hover_dust.cpp:18 | `const float kBaseSpin = 40.f;           // 0x1a3604` |
| 0x1a3604 | .rdata | src/ui/tourney_icons.cpp:8 | `constexpr float kGap = 40.f;   // 0x1a3604: after the row, and the trophy's size` |
| 0x1a361c | .rdata | src/logic/explosion.cpp:33 | `const float kSparkHold = 0.8f;              // 0x1a361c: the colour holds this share of the life` |
| 0x1a3620 | .rdata | src/logic/camera_shake.cpp:16 | `constexpr float kJoltKeep = 0.95f;            // 0x1a3620` |
| 0x1a3640 | .rdata | src/hud/shield_bar_rules.cpp:9 | `constexpr float kLowShieldPercent = 0.33f;   // 0x1a3640, compared with the shield in % as it is` |
| 0x1a3640 | .rdata | src/logic/race_warnings.hpp:74 | `static constexpr float kLow = 0.33f;       // 0x1a3640` |
| 0x1a3654 | .rdata | src/logic/replay_writer.hpp:20 | `constexpr float kRotationDrift = 0.05f;        // 0x1a3654 / 0x1a36e4: any rotation element this far off` |
| 0x1a366c | .rdata | src/logic/camera_shake.cpp:22 | `constexpr float kJoltLimit = 0.1f;            // 0x1a366c / 0x1a3508` |
| 0x1a367c | .rdata | src/logic/replay_director.cpp:182 | `static constexpr float kSlideStep = 0.003f, kSlideRate = 4.f;   // 0x1a367c, 0x1a3444` |
| 0x1a3684 | .rdata | src/ui/nemesis_intro.cpp:14 | `constexpr float kSettledScale = 1.2f;   // 0x1a3684` |
| 0x1a3694 | .rdata | src/logic/replay_director.cpp:118 | `static constexpr float kMinHold = 1.f, kStaticFovShrink = 0.7f;   // 0x1a3694` |
| 0x1a36a0 | .rdata | src/logic/race_warnings.hpp:38 | `static constexpr float kWrongWayDot = -0.4f;       // 0x1a36a0: forward . route at or under it counts` |
| 0x1a36a8 | .rdata | src/logic/rain.cpp:13 | `constexpr float kDryPerFrame = 0.0002f;                             // 0x1a36a8` |
| 0x1a36b0 | .rdata | src/app/app_knockout.cpp:16 | `const float kMessageTime = 6.f;      // 0x1a36b0: the warnings, and the camera's fly-in` |
| 0x1a36b0 | .rdata | src/ui/nemesis_intro.hpp:22 | `static constexpr float kSeconds = 6.f;   // 0x1a36b0: the loader's own clock (+0x1a80) below this` |
| 0x1a36b8 | .rdata | src/logic/rain.cpp:14 | `constexpr float kRimRadius = 64.f, kRimTexelRadius = 256.f;         // 0x1a36b8, 0x1a3710` |
| 0x1a36c8 | .rdata | src/ui/playlist.cpp:21 | `constexpr float kLeftX = 62.f, kRightX = 630.f - 62.f - 250.f, kAreaY = 160.f, kAreaW = 250.f, kAreaH = 200.f;` |
| 0x1a36cc | .rdata | src/ui/charsel.hpp:43 | `static constexpr float kInfoShown = 560.f, kInfoAway = 1280.f;   // 0x1a36cc / 0x1a36d0` |
| 0x1a36d4 | .rdata | src/logic/replay_director.cpp:119 | `static constexpr float kInFrame = -0.8f;                          // 0x1a36d4` |
| 0x1a36d8 | .rdata | src/logic/blast_push.cpp:11 | `constexpr float kBrakingPerPush = 0.02f;   // 0x1a36d8` |
| 0x1a36fc | .rdata | src/ui/rotated_quad.cpp:11 | `constexpr float kAddedTurn = 180.f;   // 0x5a211c (pi / 180) x 180 (0x1a36fc), added to every angle` |
| 0x1a3700 | .rdata | src/track/track_sprites.cpp:13 | `const float kCullDistance = 1500.f;   // 0x1a3700; list B compares the squared distance with 0x1a34b8 = 2.25e6` |
| 0x1a3718 | .rdata | src/logic/explosion.cpp:16 | `const float kTurnScale = 1.f / 1023.f;      // 0x1a3718` |
| 0x1a3718 | .rdata | src/logic/trail_sprite.cpp:11 | `const float kRandomScale = 1.f / 1023.f;   // 0x1a3718` |
| 0x1a372c | .rdata | src/logic/blast_push.cpp:13 | `constexpr float kSidePerPush = 10.f;       // -0x1a372c` |
| 0x1a372c | .rdata | src/logic/hover_dust.cpp:9 | `const float kBlowBack = -10.f;          // 0x1a372c` |
| 0x1a3730 | .rdata | src/logic/rumble_rules.cpp:13 | `constexpr float kCompressionStart = 2.2f, kCompressionBase = 1.5f;         // 0x1a3730, 0x1a3688` |
| 0x1a3734 | .rdata | src/logic/camera_shake.cpp:20 | `constexpr float kJoltRoll = -0.003f;          // 0x1a3734` |
| 0x1a3738 | .rdata | src/logic/rumble_rules.cpp:14 | `constexpr float kCompressionScale = -25.f;                                 // 0x1a3738, subtracted` |
| 0x1a3748 | .rdata | src/logic/camera_shake.cpp:12 | `constexpr float kRandScale = 1.f / 32767.f;   // 0x1a3748` |
| 0x1a3768 | .rdata | src/original/event_buffer_records.hpp:17 | `constexpr float kEventOrientationScale = 1.f / 4096.f;   // 0x1a3768 (0x423b0), 4096 = 0x1a3560 (0x582d0)` |
| 0x1a376c | .rdata | src/render/water_basis.cpp:14 | `constexpr float kUvPerMetre = 0.0625f;        // 0x1a376c` |
| 0x1a3778 | .rdata | src/logic/sound_rules.cpp:16 | `constexpr float kScrapeDot = 0.35f, kSilentScrapeDot = 0.1f;   // 0x1a3778 / 0x1a3770` |
| 0x1a37c0 | .rdata | src/logic/tourney_rules.hpp:23 | `constexpr float speedoNeedleDegrees(int level)` |
| 0x1a382c | .rdata | src/logic/attract_demo.hpp:10 | `static constexpr float kIdleSeconds = 75.f;   // 0x1a382c` |
| 0x1a385c | .rdata | src/logic/results_rules.hpp:10 | `constexpr float kPlaceGap = 1.42f;    // 0x1a385c: a place's race time at least the one ahead's + this` |
| 0x1a390c | .rdata | src/ui/upgrade.cpp:28 | `constexpr float kIconX = 430.f, kIconY = 150.f;   // 0x9f45e: panel x + 272 (0x1a390c) - 138 (0x1a3908)` |
| 0x1a3958 | .rdata | src/logic/replay_writer.hpp:17 | `constexpr float kMinRecordInterval = 0.015f;   // 0x1a3958: at most one recorded frame per 0.015 s` |
| 0x1a3964 | .rdata | src/audio/music_fade.cpp:13 | `constexpr float kDipRiseMb = 9000.f;      // 0x1a3964` |
| 0x1a3968 | .rdata | src/audio/music_fade.cpp:12 | `constexpr float kFloorMb = -9600.f;       // 0x1a3968` |
| 0x1a39dc | .rdata | src/ui/title.cpp:22 | `constexpr double kLeavingSeconds = 1.225;    // 0x1a39dc` |
| 0x1a39e0 | .rdata | src/ui/title.cpp:21 | `constexpr double kLogoSeconds = 4.9;         // 0x1a39e0` |
| 0x1a3a04 | .rdata | src/craft/craftsim.hpp:502 | `static constexpr float kBeamRumble = 50000.f;   // 0x1a3a04` |
| 0x1a3a08 | .rdata | src/logic/attack_warnings.hpp:13 | `constexpr float kProjectileWarningRange = 300.f;   // 0x66eaa: distance squared under 90000 (0x1a3a08)` |
| 0x1a3a0c | .rdata | src/logic/replay_weapons.cpp:26 | `constexpr float kDirectionScale = 127.f;   // 0x1a3a0c: x 1 / 127 on reading` |
| 0x1a8454 | .data | src/track/track_source.cpp:82 | `constexpr size_t kMarsMusicSegments = 5;` |
| 0x1b3c90 | .data | src/logic/rain.cpp:17 | `constexpr std::array<float, 8> kRimShape = {0.f, -0.15f, -0.4f, 0.15f, 1.f, 0.15f, -0.4f, -0.15f};` |
| 0x1b3ccc | .data | src/ui/multiplayer_end.cpp:21 | `constexpr float kTimeX = kRight - 325.f + 5.f, kLapX = kRight - 210.f + 5.f, kWinsX = kRight - 47.5f + 5.f;` |
| 0x1b4158 | .data | src/movie/story_scenes.cpp:15 | `const char *const kStoryNames[kPilots] = {` |
| 0x1b6990 | .data | src/original/event_buffer_records.hpp:15 | `constexpr uint32_t kCraftEntryVtable = 0x1b6990;    // the craft entry object's vtable; not read back` |
| 0x1b7094 | .data | src/original/event_buffer_records.hpp:16 | `constexpr uint32_t kWeaponEntryVtable = 0x1b7094;   // the static weapon entry's (0x1be930), as on the disc` |
| 0x1b93f8 | .data | src/hud/hud_minimap.cpp:194 | `static const int kPortrait[16] = {12, 14, 7, 2, 4, 11, 9, 6, 1, 13, 0, 15, 5, 3, 10, 8};   // 0x1b93f8` |
| 0x1b9438 | .data | src/logic/tourney_rules.cpp:23 | `const int kTrackTable[8][8] = {` |
| 0x1b9608 | .data | src/craft/ai_pilot.cpp:172 | `const float velGain = 0.6f, yawGain = 0.018f;   // per-AI tables 0x1b9608 / 0x1b9688` |
| 0x1b96c8 | .data | src/craft/ai_pilot.cpp:189 | `const float brakeErr = 0.4f;   // per-AI table 0x1b96c8` |
| 0x1b9790 | .data | src/logic/race_clock.hpp:9 | `static constexpr float kTimeLimit = 1200.f;   // 0x1b9790` |
| 0x1b9790 | .data | src/logic/race_recording.hpp:19 | `static constexpr float kLongest = 1200.f;   // 0x1b9790` |
| 0x1b9798 | .data | src/ui/charsel.cpp:29 | `const int kShipStats[16][8] = {{2500, 7, 6, 6, 8, 7, 6, 5},` |
| 0x1b99a0 | .data | src/ui/credits.cpp:21 | `const float kMotion[4][6] = {{0, 350, 200, 500, 1.5f, 2.f}, {500, 250, 450, 250, 0.9f, 1.f}, {640, 350, 440, 500, 1.5f, 2.f}, {140, 250, 190, 250, 0.9` |
| 0x1b9a18 | .data | src/ui/charsel.cpp:47 | `const float kGlobeTurn[9][2] = {{-33, -2}, {-55, -188}, {-35, -238}, {-30, -137}, {27, -254}, {-56, -104}, {-47, -117}, {-21, 53}, {0, -106}};` |
| 0x1b9a88 | .data | src/ui/fe_models.cpp:16 | `const float kCharacterPositions[16][3] = {{-6, -24, -77},` |
| 0x1bbbe8 | .data | src/logic/race_laps.hpp:12 | `constexpr int lapsForSpeedClass(int speedClass)` |
| 0x1bbc2c | .data | src/logic/game_logic.hpp:50 | `static constexpr int kRaceHintBase = 0x618, kRaceHints = 32;   // race_strings 0x618 + n (the game's 0x1bbc2c)` |
| 0x1bc518 | .data | src/logic/upgrade_rules.hpp:23 | `constexpr int priceOfLevel(int level)` |
