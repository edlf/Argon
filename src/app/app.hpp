// Argon: the App class (state + declarations). Method bodies live in app_*.cpp;
// see docs/argon/source.md for which file holds what.
#pragma once
#include "ui/fe_canvas.hpp"
#include <functional>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <array>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

#include "audio/audio.hpp"
#include "audio/custom_music.hpp"
#include "audio/music_fade.hpp"
#include "audio/race_sfx.hpp"
#include "render/camera_facing.hpp"
#include "render/d3d_lighting.hpp"
#include "render/texture_bind_cache.hpp"
#include "model/cmm.hpp"
#include "files/game_path.hpp"
#include "hud/hud_font.hpp"
#include "hud/game_hud_host.hpp"
#include "hud/hud_painter.hpp"
#include "platform/pad_rumble.hpp"
#include "files/dds.hpp"
#include "app/race_sounds.hpp"
#include "app/app_weapon_host.hpp"
#include "app/weapons.hpp"
#include "app/screenshot_saver.hpp"
#include "launch/test_runs.hpp"
#include "launch/test_script.hpp"
#include "launch/test_start.hpp"
#include "track/marker_lines.hpp"
#include "gfx/command_list.hpp"
#include "gfx/backend.hpp"
#include "logic/attack_warnings.hpp"
#include "effects/craft_exhausts.hpp"
#include "effects/engine_trails.hpp"
#include "effects/weapon_sprites.hpp"
#include "effects/wake_spray.hpp"
#include "logic/craft_weapon.hpp"
#include "logic/explosions.hpp"
#include "logic/game_progress.hpp"
#include "logic/ai_trigger.hpp"
#include "logic/lock_on.hpp"
#include "logic/shot_flight.hpp"
#include "logic/particles.hpp"
#include "logic/weapon_book.hpp"
#include "logic/pickups.hpp"
#include "logic/viewer_rand.hpp"
#include "logic/weapon_damage.hpp"
#include "logic/hit_flash.hpp"
#include "logic/weapon_effects.hpp"
#include "logic/engine_power_sound.hpp"
#include "logic/split_screen.hpp"
#include "logic/water_wake.hpp"
#include "logic/multiplayer_rules.hpp"
#include "logic/multiplayer_session.hpp"
#include "app/cheat_code_entry.hpp"
#include "app/front_end_stores.hpp"
#include "app/race_weather.hpp"
#include "app/screenshot_run.hpp"
#include "app/settings_keeper.hpp"
#include "audio/heard_speeds.hpp"
#include "craft/drive_tuning.hpp"
#include "hud/speed_readout.hpp"
#include "logic/music_cues.hpp"
#include "logic/player_power_ups.hpp"
#include "movie/loading_flyby.hpp"
#include "render/countdown_digits.hpp"
#include "logic/weapon_rules.hpp"
#include "logic/upgrade_rules.hpp"
#include "window/window_mode.hpp"
#include "logic/trail_sprite.hpp"
#include "logic/weapon_flash.hpp"
#include "logic/explosion.hpp"
#include "logic/shot_motion.hpp"
#include "logic/travel_loops.hpp"
#include "craft/drive.hpp"
#include "app/editor_session.hpp"
#include "app/fe_shell_drawing.hpp"
#include "app/track_arrival.hpp"
#include "app/scene_frame.hpp"
#include "app/race_craft_poses.hpp"
#include "app/race_player_pads.hpp"
#include "app/race_session.hpp"
#include "logic/race_picks.hpp"
#include "logic/tick_accumulator.hpp"
#include "app/editor_track_load.hpp"
#include "track/route_report.hpp"
#include "craft/craftsim.hpp"
#include "craft/craft_flaps.hpp"
#include "model/material.hpp"
#include "render/projection.hpp"
#include "render/frame_recorder.hpp"
#include "render/effect_buffers.hpp"
#include "render/gpu_model.hpp"
#include "render/model_builder.hpp"
#include "render/prop_models.hpp"
#include "render/retired_tracks.hpp"
#include "render/push_constants.hpp"
#include "render/pause_background.hpp"
#include "movie/story_scene.hpp"
#include "render/scene_sources.hpp"
#include "render/speed_blur.hpp"
#include "render/frame_copies.hpp"
#include "render/renderer.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_id.hpp"
#include "files/read_bytes.hpp"
#include "files/string_table.hpp"
#include "ui/fe_shell_host.hpp"
#include "ui/page_hosts.hpp"
#include "ui/race_end_page.hpp"
#include "ui/race_menu_hosts.hpp"
#include "track/course_folder.hpp"
#include "render/scene_views.hpp"
#include "render/scene_toggles.hpp"
#include "files/png.hpp"
#include "ui/menu_common.hpp"
#include "ui/menu_list.hpp"
#include "ui/pause_rules.hpp"
#include "ui/fe_scene.hpp"
#include "ui/game_strings.hpp"
#include "ui/settings_menu.hpp"
#include "input/player_input.hpp"
#include <map>
#include <memory>
#include "track/trackdata.hpp"
#include "logic/ghost.hpp"
#include "logic/game_logic.hpp"
#include "logic/race.hpp"
#include "logic/race_clock.hpp"
#include "logic/race_eggs.hpp"
#include "logic/race_start.hpp"
#include "logic/drive_camera.hpp"
#include "logic/free_camera.hpp"
#include "logic/race_lap_records.hpp"
#include "logic/race_points.hpp"
#include "logic/race_score.hpp"
#include "logic/race_scoring.hpp"
#include "logic/progress_keeper.hpp"
#include "logic/start_grid.hpp"
#include "logic/race_warnings.hpp"
#include "logic/attract_demo.hpp"
#include "logic/replay_director.hpp"
#include "logic/replay_race.hpp"
#include "logic/replay_writer.hpp"
#include "logic/race_recording.hpp"
#include "logic/replay_cameras.hpp"
#include "saves/ghost_library.hpp"
#include "saves/lap_records.hpp"
#include "saves/profile_file.hpp"
#include "saves/profile_fields.hpp"
#include "saves/profile_store.hpp"
#include "logic/tourney_awards.hpp"
#include "logic/tourney_win.hpp"
#include "logic/profile_progress.hpp"
#include "saves/viewer_settings.hpp"
#include "movie/nms_sequence.hpp"
#include "movie/movie_player.hpp"
#include "ui_extras/fps.hpp"
#include "model/model_source.hpp"
#include "track/track_load.hpp"

class FeShell;
struct FePageSources;
struct FeShellSources;
struct MenuPicks;
struct ProfileRequest;
struct RaceRequest;
struct RendererRequest;
struct SceneRequest;
struct ShotRequest;
struct ViewerLaunch;
struct ViewerText;
class GameHud;
class RaceMenus;
struct GameHudSources;
struct RaceMenusSources;
class SceneRecorder;
class StoryScene;

namespace fs = std::filesystem;

#include "gfx/vulkan/vulkan_shaders.hpp"
#include "gfx/texture.hpp"

// ------------------------------------------------------------------ types

using LineVertex = MarkerVertex;   // the line pipeline's vertex (track\marker_lines.hpp)

// ------------------------------------------------------------------ app
class App : public MenuDoor,
            public RaceStarter,
            public RaceLoading,
            public SceneRaceView,
            public StoryHost,
            public ModeHost,
            public RaceHost,
            public ReplayHost,
            public TrackArrivalHost,
            public RaceWeaponsHost,
            public RaceMenusHost,
            public GameHudHost,
            public TestRunHost,
            public EditorHost,
            public FrameHost,
            public TestStartHost
{
  public:
    App();
    ~App();

    // start-up, in this order (launch\startup.cpp and main.cpp decide it; app_launch.cpp, app_boot.cpp, app_window.cpp)
    void useLaunchSettings(const ViewerLaunch &launch);   // the command line's settings, taken once after parsing
    void useGameFolders(const std::string &gameFolder, const std::string &savesFolder);
    void loadGameLists(const std::string &pilot);   // the tracks, the grid's craft (pilot: the player's), the stats
    void loadViewerSettings();                      // <saves>\settings.json, written with the defaults the first time
    void silenceAudio();                            // before startAudio: no audio device (screenshot runs)
    void startAudio();
    void openWindow(const std::optional<argon::Resolution> &requestedSize);   // --size, else settings.json's
    void initRenderer();
    void startUi();                                 // ImGui, the languages with real text, the HUD
    void rememberEditorTrack(const std::string &trackName);   // where Game Demos opens the editor
    void openEditorScene();                         // the editor on that track, at start-up
    void pickEditorTrack(const std::string &trackName);       // --pick-track: loaded on the next frame
    void placeShotCamera();                         // a --shot run from the top
    void placeFreeCamera(const glm::vec3 &position, float yawDeg, float pitchDeg);
    void aimFreeCameraAt(const glm::vec3 &target);
    void printTrackSummary() const;
    void applyUnlockAll();                          // --unlock-all
    void loadSettingsString(const std::string &settings);     // --settings
    // the test runs and test starts the command line asks for (launch\test_runs.hpp, launch\test_start.hpp)
    TestRunSources testRunSources();
    TestStartSources testStartSources(TestRuns &testRuns);
    void playCountdown();
    void run();
    void shutdown();
    bool screenshotRun() const override
    {
        return shotRun.active();
    } // --shot: writes no saves (profile, lap records)
    void scrollFreeCameraSpeed(double wheel);       // the mouse wheel: the editor's free camera

  private:
    argon::Resolution validStoredResolution();
    int trackListIndex(const std::string &trackName) const;

    GLFWwindow *window = nullptr;
    argon::WindowMode windowMode;   // Resolution and Fullscreen (F11)
    std::string game;    // the game's files (data, config, audio, Locale, ...): <project>\game_data
    std::string saves;   // what the viewer writes: profiles, ghosts, records, custom_music (<project>\save_data)
    std::vector<std::string> trackList;
    ViewerSettings viewerSettings;   // <saves>\settings.json: language, intro videos (viewer_settings.hpp)

    RendererRequest rendererRequest;   // --renderer, --msaa, --post-aa, --oit, --validate (else settings.json's)
    // the command line's parts (app_launch.cpp)
    void useRendererRequest(const RendererRequest &request);
    void useShotRequest(const ShotRequest &request);
    void useSceneRequest(const SceneRequest &request);
    void useProfileRequest(const ProfileRequest &request);
    void useRaceRequest(const RaceRequest &request);
    void useMenuPicks(const MenuPicks &menuPicks);

    // what changes with the view (the scene constants, the camera-facing sprites) has a slot per frame in flight and view
    static const int kViewSlots = argon::gfx::kFramesInFlight * SceneFrame::kMaxViews;

    Renderer renderer;   // the GPU side kept for the whole run: the backend, shared textures, pools and passes
    FrameCopies frameCopies{renderer};   // the transition's, the Hud Lens's and the feedback's (render\frame_copies.hpp)

    Track track, sky;
    int trackIndex = -1, pendingTrack = -1;

    std::map<std::string, Found<Track>> pickupFor;   // <track>pup.txt type -> model
    std::map<std::string, Found<Track>> pickupTakenFor;   // the _ns model drawn once the player took it this lap

    // ---- pickups (app_pickups.cpp: what a take gives) and the particles they share with the weapons
    argon::Pickups pickups;
    argon::Particles particles;
    bool pickupLogic = true;
    PlayerPowerUps powerUps;   // the player's levels, special and their caps (logic\player_power_ups.hpp)
    // ---- the race's event sounds: RaceSounds (race_sounds.hpp, raceSounds below audio); the listener's filter
    RaceSounds::RaceState raceSoundState() const override;   // app_race_sounds.cpp
    void selectListenerFilter();
    void placeOtherEngines();
    void playLapRecordSound() override;
    void resetPickups() override;
    // ---- weapons (app\weapons.hpp: App::weapons, its host App::weaponHost, both last); here what draws them, the
    // player's fire buttons and power-up levels, Weapons Off
    using Shot = argon::weapons::Shot;
    // trail sprites (trail types > 3, app_trail_sprites.cpp), the dust, the explosions' flashes and the beams' and
    // sparks' one-frame flashes: spray-pool quads from quadsprite.dds, alpha or additive
    argon::WeaponSprites weaponSprites;
    void emitTrailSprite(const Shot &shot, const glm::vec3 &at) override;
    void raiseDust(const argon::GameCraft &craft) override;   // the craft's dust this frame (logic\hover_dust.hpp)
    void flashBeam(const Shot &shot, argon::weapons::BeamStop stop) override;
    void throwSparks(const Shot &shot, const argon::weapons::BeamFlashes &flashes, argon::weapons::BeamStop stop);
    glm::vec3 throwSparkParticles(const glm::vec3 &at, argon::weapons::BeamStop stop);
    argon::weapons::FlashSpriteRoll rollFlashSprite();
    void raisePlayerPowerUps(int type);
    bool weaponsOn = true;   // 0x52abe3 (off with Weapons Off)
    void loadWeapons();      // data\weapon.nwp into the weapons
    void readFireInput();
    argon::weapons::FireButtons heldFireButtons(int player) const;
    HudCamera hudCamera;   // this frame's first view, as the HUD, the markers and the speed blur read it
    // ---- score (logic\race_points.hpp, logic\race_score.hpp; app_score.cpp): the HUD's points, the config's values
    RaceScoring scoring;   // the points, the config's values, the score and its count-up (logic\race_scoring.hpp)
    void awardPoints(int points, int kind, int pilot = 0) override;  // 0x83f00: kind = popup label (table 0x1b9748)
    void scoreLap();                         // 0x83d80 on a finished lap
    void aiTakePickups(const glm::vec3 &ship, int slot) override;   // an AI craft's pickups_collide (AI flag set)
    void takeReplayedPickup(int pickup, int slot);          // a replay's pickup entries (signals 8 / 9)
    void bringReplayedPickupBack(int pickup);
    std::vector<glm::vec3> slotPositions(const glm::vec3 &player) const;   // the particles' followed crafts
    void updatePickups(const glm::vec3 &ship);
    void givePlayerPickup(const argon::PickupChange &taken, const glm::vec3 &ship);
    void burstOnCraft(const argon::PickupChange &taken, const glm::vec3 &craft, int slot);
    void burstOnPickup(const argon::PickupChange &back);
    void stepParticles(const glm::vec3 &player);
    void loadParticleTextures() override;   // the pickup particle atlases and weapon.nwp (once)

    // AI drive (follows the game's AI node graph) + route viability check
    argon::CollisionGrid grid;
    argon::Route route;
    argon::CraftStats craftStats;      // config\character_stats.txt
    argon::AiGraph aiGraph;
    argon::CollisionGrid simGrid;      // track collision only (water is handled as water)
    argon::SimWorld simWorld;
    argon::GameCraft sim;               // the player's craft (racer 0 of the race; the editor drives it too)
    RaceSession session;   // who drives, held / paused / over, the race's counters (app\race_session.hpp)
    RaceCraftPoses craftPoses{RaceCraftPoseSources{track, race, sim, simPrev, simCur, ticks, session}};   // app\race_craft_poses.hpp
    Race race{*this,
              RaceSources{game,
                          track,
                          modes,
                          multiplayer,
                          aiGraph,
                          simWorld,
                          craftStats,
                          startGrid,
                          sim,
                          driveView,
                          weapons,
                          viewerRand,
                          picks.speedClass,
                          session.hudLaps,
                          drive.aiPower,
                          session.playerDrive,
                          craftPoses}};   // the race manager and its drones
    ReplayRace replayRace{
        *this,
        ReplaySources{
            game, weapons, startGrid, race, aiGraph, session.hudLaps, modes, sim, simPrev, simCur, camPrev, camCur, driveView}}; // a recorded race placing the crafts
                                                                                                                         // (logic\replay_race.hpp, --replay n)
    // ---- replays (app_replays.cpp; docs\replays\README.md): the race recorded in the game's format for View Replay, the
    // attract demo's idle trigger and way back, the pause menu's ghost replays
    RaceRecording recording;   // the race being recorded and the last one's, for View Replay (logic\race_recording.hpp)
    bool pendingViewReplay = false;   // tests.viewReplay: the race's playback is due at its end
    AttractIdleClock attractIdle;
    AttractReplayCycle attractCycle;
    struct StateBeforeReplay
    {
        int raceEnd = 0, camView = 2, page = 6, droneCount = 5;
        GameLogic *mode = nullptr;
        Race::Standings standings;
    };
    StateBeforeReplay beforeReplay;
    void startRaceRecording();
    void recordRaceStep();
    void stopRaceRecording();
    void printShieldEnergies(const char *when) const;
    void restartEffectsClock();   // View Replay: simClock where the race's started (RaceRecording::Timing)
    std::vector<argon::replay::CraftEvent> racerStates() const;
    bool startViewReplay() override;
    void startGhostReplay(int ghost) override; // 0 the loaded ghost, 1 the session ghost (the pause menu)
    bool startAttractDemo(int number) override;
    void stepAttractIdle(float dt);
    bool attractAllowedOnPage() const;
    void updateReplayInput();
    bool replaySkipPressed();
    bool replayHidesHud() const;
    void useReplayCamera();
    ReplayCameras replayCameras;   // the replay cameras' director (logic\replay_cameras.hpp)
    argon::replay::FollowedCraft followedCraft(const glm::mat4 &pose) const;
    std::vector<argon::replay::PassingCraft> passingCrafts() const;
    argon::replay::CameraShot replayCameraShot(const glm::mat4 &pose, float dt);
    void finishEndedReplay();
    void leaveAttractDemo();
    void leaveViewReplay();
    // the weapon entries (app_replay_weapons.cpp): written as the race's weapons act, played back by ReplayRace
    void recordWeaponEvent(const argon::replay::WeaponEvent &event) override;
    void recordShieldValues() override;
    void playWeaponEvent(const argon::replay::WeaponEvent &event) override;
    void replayPickupTaken(const argon::replay::WeaponEvent &event);
    void replayPickupBack(const argon::replay::WeaponEvent &event);
    void stepReplayedWeapons() override;
    void showReplayedControls(int slot, uint32_t controls) override;
    RacePicks picks;   // the character, course row and speed picked; the race being loaded (logic\race_picks.hpp)
    glm::mat4 simPrev{1}, simCur{1};                // craft pose at the last two 60 Hz steps (drawn interpolated)
    argon::GameCraft::Tripod camPrev{}, camCur{};
    DriveCamera driveView;   // the drive view chosen and the TV director (logic\drive_camera.hpp)
    bool viewOverride = false;   // the drive camera owns the view (viewShot: with roll) and fov
    ViewShot viewShot;           // the drive camera's, the intro's or a flyby's shot
    bool showPanel = true;
    bool dumpMaterials = false;
    GameStrings strings;                             // Locale/<language>/*.str and the viewer's own text (ui\game_strings.hpp)

    // ---- the front end: the shell, its pages and models (App::fe, ui\fe_shell.hpp; docs\frontend\README.md); here only its
    // opening over the race / editor (app_menu.cpp) and the renderer's side (the page's models from an ImGui callback)
    FeShellDrawing shellDrawing{painter};   // the page models and menu bars the shell asks for (app\fe_shell_drawing.hpp)
    void openMenu() override;
    void closeMenu() override;
    void quit() override;
#if ARGON_DEVELOP
    void startEditor() override;
#endif
    FeShellSources feShellSources();
    FePageSources fePageSources();
    // ---- character select / Character Data / Ship Data (ui\charsel.cpp, docs\frontend\README.md)
    using MenuInput = menu::MenuInput;
    PlayerInput input{viewerSettings.keyBindings()};   // the keyboard (its keys as bound) and the controller ports (input\player_input.hpp)
    using enum menu::Page;   // the front end's pages (ui\page_id.hpp)
    RacePlayerPads playerPads{modes, multiplayer};   // each player's pad (app\race_player_pads.hpp), before what takes it
    std::unique_ptr<FeShell> fe;     // the front end's shell (ui\fe_shell.hpp)
    argon::GameProgress progress;   // the profile block: unlocks, tournaments, cash, upgrades, scores, eggs, cheats
    std::u16string racerName(int pilot) const override; // 0x78b00: infotext 400 + character, the short name in a race
    void startQuickRace(int pilotId, int trackId) override;
    // ---- scene transition: started here (a --shot run keeps its shots clean); drawn by frameCopies
    void startTransition() override;
    // ---- FE COURSE SELECT (ui\charsel.cpp, docs\frontend\race-setup.md "Track select")
    int courseId(const std::string &folder) const override;
    bool onMarsTrack() const;   // Mars (0x10) or Terraformed Mars (0x11) loaded
    // ---- profiles (app_profile.cpp; ui\choosesave.cpp, ui\keyboard.cpp, FE MESSAGEBOX ui\messagebox.cpp; docs\frontend\profiles.md "Profiles"): <game>\profiles\*.json
    using Profile = argon::Profile;   // saves\profile_file.hpp
    argon::ProfileStore profiles;     // the list, the one in use, writing them (saves\profile_store.hpp)
    ProgressKeeper progressKeeper{progress, profiles}; // the rules and writes over the progress (logic\progress_keeper.hpp)
    // ---- Options: the lists (SettingsMenu, ui\settings_menu.hpp; drawn by ui\options.cpp); what a choice does is
    // settingsKeeper's (app\settings_keeper.hpp)
    SettingsMenu settingsMenu;
    std::vector<Language> translatedLanguages{Language::English};   // the Language row's: findTranslatedLanguages
    void findTranslatedLanguages();   // the languages whose Locale tables are real text; settings.json's language else English
    void loadStringTables();     // the language's fe, race, info and strings tables
    void toggleFullscreen();      // F11: Fullscreen switched and written
    // ---- Easter Eggs (page 17, FE EASTEREGGS) and Cheats (page 18, FE CHEATMENU): ui\eggs.cpp; the unlocks app_unlocks.cpp,
    // a code typed app\cheat_code_entry.hpp
    RaceEggs raceEggs; // the eggs this race runs with (0x52c8b8..0x52c8be)
    void switchOnRaceEggs(); // the race load (0x792f0)
    void switchOffRaceEggs(); // back to the front end (main_menu 0x30af0)
    static void useAtlantisFog(argon::TrackProps &props);
    // ---- Select Speed (page 20, FE SPEEDSELECT) and Track Data (page 21, FE TRACK INFO): ui\speed.cpp
    // ---- the race's menus (App::raceMenus, ui\race_menus.hpp): the pause menu (ui\pause.cpp, PAUSEMENU; the race held: app_pause.cpp) and the race end's
    std::unique_ptr<RaceMenus> raceMenus;
    RaceMenusSources raceMenuSources();
    std::array<std::unique_ptr<GameHud>, argon::split::kMaxPlayers> huds; // the in-race HUD per player (hud\game_hud.hpp)
    GameHudSources hudSources();
    void renderHuds();
    void pauseRace() override;
    void resumeRace() override;
    void resetPauseMenu(); // the blur seeded again, the cursor on Continue
    void updatePause();
    menu::PadWatch padWatch; // the racers' controller messages (0x8a7f0, ui\pause_rules.hpp)
    bool watchControllers();
    menu::PadWatch::PerPlayer racersConnected() const;
    menu::PadWatch::PerPlayer racersStartPresses();
    std::array<bool, menu::PadWatch::kPlayers> pzPadStartHeld{};
    void restartRace() override; // the race again from the grid (Restart, Race Again)
    void quitRaceTo(int page) override; // out of the race to FE MAIN or FE COURSE SELECT
    void openGhostSave(int nextPage, ghost::Lap lap) override;
    void leaveRaceFor(int page);
    void drawPause();
    PauseBackground pauseBackground{frameCopies}; // the race frame blurred under the pause menus (render\pause_background.hpp)
    void drawMenuBars(const FeCanvas &canvas, float vw) override;
    // ---- the race end (app_results.cpp; pages ui\results.cpp, ui\score.cpp, ui\upgrade.cpp): which page is up is session.raceEnd
    using enum race_end::Page;
    void resetRaceEnd() override;
    void stepRaceEnd();
    void openResultsTable(const char *how);
    bool followsRaceEndCameras() const;   // the race view through the replay cameras behind the race-end pages
    // a race-end page's second of leaving: the race music at the fade's volume, logged to stderr (app_results.cpp)
    void fadeRaceMusic(const argon::MusicFade &fade, double now) override;
    void pressTestLeave();
    void playerFinished();
    // the race's clock and its 20-minute limit (logic\race_clock.hpp)
    RaceClock raceTime;
    float raceClock() const override
    {
        return raceTime.elapsed(simClock);
    }
    void endRaceAtTimeLimit();
    // the player knocked out (craftdeath 0x61fc0, app_knockout.cpp): every knocked-out craft is held in the track's
    // NET_CAGE (0x58d20); the player sees the death camera (0x620e0) and "You were destroyed by" (0x624f0), then
    // LOSEMENU after 10 s or A
    PlayerKnockOut playerKnockOut;
    std::optional<glm::mat4> netCage() const;
    int hiddenNetGlowBone() const override; // the track's NET_GLOW while hidden (-1: shown or none)
    void retireCraft(int slot) override;
    void stepPlayerKnockedOut();
    argon::GameCraft::Tripod deathCamera() const;
    static constexpr float kDeathCameraFov = 72.f; // 0xae550 (pi x 0.4)
    void drawKnockOutMessages();
    void playerLapFinished(float lapTime) override; // the race manager counted the player's lap: its turbo, points, record
    void grantLapTurbo();                  // race_craft_lap 0x3d5a0's turbo for a counted lap
    void updateRaceEnd();
    void drawRaceEnd();
    // ---- the game mode (0x52ccfc; logic\game_logic.hpp): Tournament, Quick Race or Time Attack, set by FE MAIN
    GameModes modes;   // built with the front end's shell, which opens the pages the modes lead to (App::App)
    ModeSources modeSources();
    GameLogic &logic() const override
    {
        return modes.current();
    }
    bool slotShown(int slot) const override; // a start slot's ship: the player and the opponents racing (the editor: every slot)
    // ---- Tournament (app_tourney.cpp, ui\tourney.cpp; page 22 FE TOURNEYTYPE)
    // ---- the lap line (app_lapline.cpp); Time Attack's ghosts are TimeAttackLogic's (logic\time_attack.cpp)
    void stepLapLine(); // the race's per-frame step 0x429c0: the lap line's turbo, then the mode's lapLineCrossed
    // the saved ghosts (saves\ghost_library.hpp): <saves>\ghosts\<track>_<pilot>.ghost
    GhostLibrary ghosts;
    // the lap records (app_laprecords.cpp): g_track_records, index speed + 5 x track (lap_records.hpp)
    RaceLapRecords lapRecords{profiles, strings, saves}; // the table, the race's best, new records (logic\race_lap_records.hpp)
    void playerLapRecord(float lap);
    TourneyRun tourneyRun;   // the tournament being raced and its last win's awards (logic\tourney_win.hpp)
    void startTourneyRace(int level) override;
    // ---- the profile's progress (app_progress.cpp): Percentage Complete, the Trophy Level, the Redshift wins
    // ---- the race's scores (app_score.cpp, ui\score.cpp): raceEnd 3 SCOREMENU, 6 POINTSMENU
    // the character's upgrade levels 0..7 (0x1bb0ec + character x 0xb0; logic\upgrade_rules.hpp)
    int upgradeChar() const override
    {
        return logic().pilot();
    }
    int upgradeLevel(argon::upgrades::Upgrade upgrade) const override
    {
        return progress.upgrades[upgradeChar()][upgrade];
    }
    float lapTurbo(int character) const override // race_craft_lap 0x3d5a0: a lap's turbo seconds
    {
        return argon::upgrades::lapTurboSeconds(progress.upgrades[character & 15][argon::upgrades::kTurbo]);
    }
    // the player's pickup caps 0x52cd0c / 12 / 18 / 1e (race setup 0x3f3d0)
    void applyUpgradeCaps() override;
    float shieldSize(int pilot, int level) const; // 0x64030: 20 x level + 100 / ((100 - armour) x 0.01)
    // race start (logic\race_start.hpp; app_intro.cpp: the camera, the digits, the sounds): the grid intro, then the
    // countdown; the crafts wait until GO
    RaceStart raceStart;
    CountdownDigits countdownDigits;   // render\countdown_digits.hpp
    void startGridIntro();
    void startRaceCountdown();
    void loadCountdownModels();
    void startCountdown();
    bool raceHeld() const override;
    void stepRaceIntro(float dt) override;
    void introCamera();
    bool gridIntroRunning() const override // 0x52af9d
    {
        return raceStart.introRunning();
    }
    int introCraft() const; // the racer (0 = the player) the grid intro shows
    int introCraftSlot() const override; // its grid slot
    void computeScore() override;
    // ---- custom music (app\app_music.cpp; FE LEVELMUSIC ui\playlist.cpp): <game>\custom_music\<soundtrack>\*.wav stand for the Xbox's ripped
    // soundtracks; the playlist (32 songs, 0x4406fc) and the source (0x1bbc27) belong to the profile
    CustomMusic customMusic;   // <saves>\custom_music, the playlist, Music Source (audio\custom_music.hpp)
    // the front end's stores the pages save and load through (app\front_end_stores.hpp)
    GhostShelf ghostShelf{ghosts, strings};
    LapRecordSaver lapRecordSaver{lapRecords, shotRun};
    CustomMusicStore musicStore{customMusic, profiles};
    ProfileSwitcher profileSwitcher{ProfileSwitcherSources{profiles, strings, settingsKeeper, progress, progressKeeper, tests, customMusic, musicStore, lapRecords}};
    void applyRaceMusic() override;
    // ---- record pages (ui\records.cpp): 14 Track Records (FE FASTESTLAP), 15 FE HIGHSCORES, 16 FE GHOSTMANAGE
    // ---- LOADER (ui\loader.cpp; the load app_loader.cpp)
    int raceHint = 0; // race_strings 0x618 + n (the game's 0x1bbc2c)
    void loadRaceNow() override; // blocking (--race-go)
    bool beginRaceLoad() override; // the loading screen: starts reading the track; false when it is not there
    void setRaceLaps(); // g_race_laps for the race being loaded (0x3b0b0)
    bool stepRaceLoad() override; // a frame's slice of it; true once the race is ready
    void startLoadedRace();
    // ---- NMS sequences (app\app_sequence.cpp): the loading screen's track flyby (<t>_glory.nms)
    LoadingFlyby loadingFlyby;   // drawn behind the loading screen (movie\loading_flyby.hpp)
    void startGlorySequence(const std::string &trackFolder) override; // <t>_glory.nms behind the loading screen
    void updateSequence() override;
    void useSequenceCamera(const SequenceCamera &camera, int &shownCamera) override;
    void showSequenceShot(const ViewShot &shot, bool newCamera);   // the transition on a new camera
    // the story scenes (movie\story_scene.hpp): before a tournament race (ui\loader.cpp), after its last (0x7fa50)
    std::unique_ptr<StoryScene> story;
    bool playRaceEndScene(int page);
    void updateRaceEndScene();
    void finishQuickRace() override; // 0x1b9538: game track id -> data\tracks folder
    // background feedback (0xa7080 / 0xa6eb0): the last finished menu frame, drawn back zoomed / rotated
    // additive ImGui pipeline (created on first use with the backend's pipeline layout)
    void drawBrightness();
    PadRumble padRumble; // the connected pads' motors (platform/pad_rumble.hpp)
    void updateVibration(); // Controller n Vibration: each player's craft's rumble to their pad
    int rumblePadOfPlayer(int player) const;
    bool racerFinished(int slot) const;
    void giveTestSpecial();
    // the HUD's warnings (app_warnings.cpp): RESET / WRONG WAY (craft_race_info 0x5ce70), Shield Low (0x4eb20)
    argon::RaceWarnings raceWarnings;
    argon::ShieldLowWarning shieldLow;
    void resetRaceWarnings();
    void stepRaceWarnings();
    void cueLapMusic();
    void cueLeadMusic();
    argon::music::LeadWatch leadWatch;   // the player's place the music last saw (logic\music_cues.hpp)
    SpeedReadout speedReadout;   // the speedometer's last four speeds (hud\speed_readout.hpp)
    argon::TickAccumulator ticks;   // the frames' time in 60 Hz ticks (logic\tick_accumulator.hpp)
    HeardSpeeds heardSpeeds;   // each craft's speed as its engine sound hears it (audio\heard_speeds.hpp)
    void followOtherEngines();
    void drawDeveloperTools();
    EditorSession editor;
    EditorPanelSources editorPanelSources();
    void enterEditorMode();  // the editor (debug view): muted, the mouse cursor shown
    void leaveEditorMode();
    void showCursorInEditorOnly();   // each frame: the cursor hidden outside the editor

    // keyboard: W/Up throttle, A D / Left Right steer, Q / E airbrakes, R dive, F climb, S/Down brake.
    // gamepad as the game's pad handler (0x70ac0): RT throttle (x 0.390625 of 0..255), left stick or d-pad steer
    // (stick past 12288 / 32768, then (|x| - 12288) x 0.00488), stick forward / back or d-pad up / down = dive /
    // climb, LT = airbrake on the side you steer towards (none when not steering). The game's pad never sets the
    // brake slot; X here drives the ported brake (0x60450) anyway.
    argon::GameCraft::Pad readPad();
    argon::GameCraft::Pad readPlayerPad(int player) override;
    argon::GameCraft::Pad finishPad(argon::GameCraft::Pad p, int player);
    bool gamepadOfPlayer(int player, GLFWgamepadstate &state) const;
    DriveTuning drive;   // the editor's drive settings (craft\drive_tuning.hpp)
    argon::RouteReport routeReport;   // the loaded route's summary and issues (track\route_report.hpp)

    // audio
    argon::Audio audio;
    AppliedSettings applied;   // the Options pages' Speed Units, HUD and Brightness (app\settings_keeper.hpp)
    // what the Options pages' choices do, and a cheat code typed (app\settings_keeper.hpp, app\cheat_code_entry.hpp)
    SettingsKeeper settingsKeeper{SettingsKeeperSources{settingsMenu,
                                                        strings,
                                                        viewerSettings,
                                                        translatedLanguages,
                                                        audio,
                                                        renderer,
                                                        windowMode,
                                                        window,
                                                        game,
                                                        saves,
                                                        profiles,
                                                        progress,
                                                        progressKeeper,
                                                        applied}};
    CheatCodeEntry cheatCodeEntry{strings, progress, progressKeeper, fe};
    RaceSounds raceSounds{audio.raceSfx()};   // the race's event sounds
    TestScript tests;   // the command line's test options (launch\test_script.hpp)
    std::unique_ptr<MoviePlayer> movie; // FE INTROSCREENS' videos (movie\movie_player.hpp)
    bool audioEnabled = true;
    std::string enginePilot;

    // view / UI state
    FreeCamera freeCamera; // the viewer's own (logic\free_camera.hpp)
    SceneToggles toggles;   // what the 3D view draws and how (render\scene_toggles.hpp; the editor panel)

    // screenshot mode
    ScreenshotRun shotRun;   // --shot / --cam / --frames (app\screenshot_run.hpp)




    // ---- the crafts' effects (src\effects): the engine trails, the jets and flaps, the water spray; App draws them
    static const int kShips = argon::kEffectSlots;   // crafts with their own effects state (index = grid slot)
    argon::EngineTrails engineTrails;
    argon::CraftExhausts exhausts;
    argon::WakeSpray wakeSpray;
    // one 60 Hz update of a craft's trail, jet, flaps and spray (its dust is raiseDust)
    void followCraftEffects(const argon::GameCraft &craft, const glm::mat4 &matrix, int slot) override;
    bool nemesisRace() const override;
    bool raceRunning() const override;

    // speed blur (race scene 0x93490): the last frame drawn back zoomed / faded once the camera moves fast
    SpeedBlur speedBlur{frameCopies}; // the blur, the echo and the underwater overlay (render\speed_blur.hpp)
    void drawSpeedBlur();
    bool fixesMarsSpeedBlur() const;   // Argon Options' Mars Flicker Fix on and a Mars track loaded
    // the race's weather (app\race_weather.hpp; app_rain.cpp): the rain, its loop and the tunnel sound, drawn over
    // the race scene by hud\rain_overlay.hpp
    RaceWeather weather{WeatherSources{audio, game}};
    void startRaceWeather(int trackId);
    void startEditorWeather(int listIndex);
    void stepRain();   // one 60 Hz frame of the rain at the camera
    bool playerUnderRoof() const;
    bool cameraUnderWater() const;
    bool sprayNearEye() const;

    // ribbons -> this frame's vertex buffer (triangle list)



    float simClock = 0.f; // seconds of simulated time (the spray ages with it)
    argon::ViewerRand viewerRand;
    uint32_t rnd()
    {
        return viewerRand.next();
    }
    void stepEffectsClock() override; // once per 60 Hz step: the clock, then the spray ages

    // the puffs (upright quads) and the droplet sheets (world quads) into this view's buffer


    // the track's envdata\quadsprite.dds (particle atlas), loaded when the track changes

    // the grid ships and Time Attack ghosts as drawn this frame (model + placement)
    std::vector<std::pair<Track *, glm::mat4>> visibleShips();
    Found<Track> shownGhostModel(int ghost) override;

    // records the shadow-map passes (outside the main render pass) and fills this frame's receiver triangles
    void recordShadows(argon::gfx::CommandList &commands) override;




    // ---------------------------------------------------------- track loading
    // AI route from config\ai_<t>.txt over this track's collision, and the viability check (rb: buildRoute)
    void applyRoute(RouteBuild &&rb) override;

    std::string viabilityText() const override;

    // place the game-physics craft on grid slot 0, AI tracking from the nearest node
    void resetDriver() override;

    void stepDrive(float dt) override;
    void settleDrive(float dt) override;
    void prepareDriveStep();
    bool driveRunning() const;
    void followDrive(float dt);
    // stepDrive's parts: the 60 Hz ticks, then one method per concern of a tick, in the game's order
    void startPendingViewReplay();
    void stepSimulation(float dt);
    void steerPlayerCraft();
    void handControlToPlayer();
    void stepRaceTick();
    void stepHeldGrid();
    void startFromGrid();
    void stepPlayerCraft();
    void stepWorldEffects();
    void stepRacers();
    void stepWeapons();
    void stepRaceProgress();
    void scoreFinishedLaps();
    void scoreAirTime();
    void stepHudReadouts();
    void followEngineSpeeds();
    void followEngineSound();



    // the track's ship-mounted cameras (camera_<t>.txt type=ship)

    // TV director (viewer's own logic): cameras whose lap-progress window contains the ship

    void driveCamera(float dt);
    ViewShot driveShot(float dt);



    // grid slot 0 = your ship, the rest in pilot order
    // the pilots, the player's and the grid order (logic\start_grid.hpp)
    StartGrid startGrid{{modes, multiplayer, replayRace, progress, track.data}};

    // pickup type from <track>pup.txt -> data\objects folder

    // every model's GPU side: loading, materials, cubes, water, posing, releasing (render\model_builder.hpp)
    ModelBuilder models;
    PropModels props;               // the craft, pickup and explosion models, kept across tracks
    RetiredTracks retiredTracks;    // replaced tracks, until no frame in flight reads them
    uint64_t trackSerial = 1;   // a new one each time the race track is cleared (TrackArrival::clearTrack)



    // blocking (the editor, --track, the menus); the loading screen reads on a thread instead (beginRaceLoad)
    void loadTrack(int index);
    // the frame the scene records into: its slots, windings, clocks, countdown and markings (app\scene_frame.hpp)
    SceneFrame sceneFrame{SceneFrameSources{renderer,
                                            models,
                                            track,
                                            toggles,
                                            raceEggs,
                                            game,
                                            pickupFor,
                                            pickupTakenFor,
                                            raceStart,
                                            countdownDigits,
                                            editor}};
    // a track taken in: the load's request and steps, the old track retired (app\track_arrival.hpp)
    TrackArrival trackArrival{TrackArrivalSources{renderer,
                                                  models,
                                                  props,
                                                  track,
                                                  sky,
                                                  retiredTracks,
                                                  trackSerial,
                                                  trackList,
                                                  trackIndex,
                                                  startGrid,
                                                  audio,
                                                  raceSounds,
                                                  enginePilot,
                                                  pickupFor,
                                                  pickupTakenFor,
                                                  raceEggs,
                                                  race,
                                                  scoring,
                                                  drive,
                                                  routeReport,
                                                  game},
                              *this};
    std::unique_ptr<TrackLoad> trackLoad; // the loading screen's load in progress
    EditorTrackLoad editorLoad;   // the editor's track pick, loading behind its loading screen
    void startEditorLoad(int index);
    void waitForFocus();

    void cameraToStart() override;
    void loadGridShips();
    void choosePlayerPilot(int pilot) override;
    void reshuffleGrid() override;
    void chooseDriveView(int chosenView) override;
    void followViewButton();

    void cameraToTop() override;

    // ---------------------------------------------------------- per frame
    void updateCamera(float dt);

    // ------------------------------------------------------------- HUD (the game's gamehud layout)
    // Decoded from default.xbe: setup 0x46040, layout 0x45270, update 0x496f0, render 0x4a100,
    // minimap 0x4f090, rotated quad 0xcbe50, bar 0xc24d0, text 0xb1660. Notes: docs\physics\README.md (HUD).
    void loadFonts();   // into the painter, in settings.json's language
    void matchFontsToLanguage();
    std::vector<std::filesystem::path> replacementFontFolders() const;


    void loadHud();




    HudPainter painter;   // the 2D layer: HUD textures, fonts, text, panels (hud\hud_painter.hpp)

    void drawUi();
    // ---- FPS meter (Argon Options' FPS Meter, F2): the last 120 frame times, top-right
    FpsMeter fpsMeter;
    bool fpsMeterShown() const;
    void toggleFpsMeter();   // F2: FPS Meter switched and written
    // the memory line under it (RAM: private bytes; VRAM: VK_EXT_memory_budget's device-local heap usage), sampled
    // twice a second

    MultiplayerSession multiplayer;   // the race's human players, their picks, pads and Wins (logic\multiplayer_session.hpp)
    // the frame's cameras (render\view_cameras.hpp): the main view, a view per human player in a split-screen race
    ViewCameras cameras{ViewCameraSources{freeCamera, viewShot, raceEggs, multiplayer}};
    ViewMoment viewMoment() const override;
    ViewShot chaseShotOf(int player) const;   // players 2..4: their craft's tripod between its last two steps
    HudCamera playerHudCamera(int player) const override;
    void startMultiplayer(int players) override; // game mode 4 with players 1..n (tests: --multiplayer n)
    void leaveMultiplayer();
    void stepMultiplayerEnd();
    argon::multiplayer::Standing multiplayerStanding() const;
    void endMultiplayerRace(const argon::multiplayer::Standing &standing);
    bool loaderFlybyShown() const;
    bool frontEndScreenShown() const override;
    bool frontEndOpen() const override;
    bool modeHasPickups() const override;
    SceneSources sceneSources() override; // what the frame's SceneRecorder draws from
    FrameSources frameSources();          // what the frame's FrameRecorder records from

    // --shot's frame and F12's (written to the screenshots folder beside save_data)
    ScreenshotSaver screenshots;

    enum class FrameOutcome
    {
        Skipped,
        Presented,
        Captured
    };
    float frameDeltaTime(std::chrono::steady_clock::time_point &last);
    void buildFrameUi(float dt);
    void updateFrame(float dt);
    int frontEndMusicMode() const;
    FrameOutcome renderFrame();
    void adoptRemadeWindow();

    void shutdownImGui();
    void unloadAllModels();

    // ---- the race's weapons (app\weapons.hpp) and App as their host (app\app_weapon_host.hpp); last, as they are given
    // the race, its sounds, particles, sprites, random numbers and test options (App::App)
    AppWeaponHost weaponHost;
    WeaponHostSources weaponHostSources();
    Weapons weapons;
};
