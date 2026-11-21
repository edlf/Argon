# Source layout

`app/app.hpp` holds the `App` class (argon's state and declarations); its method bodies are split by subsystem, and
the pieces that stand on their own are classes of their own:

| folder | what |
|---|---|
| `app` | the `App` class's method bodies (`app_*.cpp`) |
| `audio`, `hud`, `movie`, `ui` | sound and music, the in-race HUD, videos and story scenes, the menus |
| `craft` | the craft physics (`craftsim`), its flaps, argon's AI drive |
| `files` | file formats and game paths: DDS, PNG, string tables, the case-insensitive lookup, UTF-16 |
| `launch`, `window`, `platform` | the command line and start-up, the window's fullscreen mode, what differs per OS |
| `logic` | game rules with no GPU or audio (race, weapons, replays, ghosts, ...) |
| `model`, `track` | `.cmm` models and their animation; tracks, their data, visibility and sprites |
| `original` | the game's binary layouts as structs |
| `render` | Vulkan: the device, swapchain, scene recording, shared render maths |
| `saves` | argon's settings and the lap records files |
| `ui_extras` | argon extras (editor panel, FPS meter) |

Files by subsystem:

| file | what |
|---|---|
| `main.cpp` | `main()` |
| `launch/launch_options.cpp`, `launch/startup.cpp` | the command line (`LaunchOptions`), start-up (`ViewerStartup`: finding the game and the order of App's start-up steps, the start scene, `--drive-time` pre-runs) |
| `launch/test_start.cpp` | `TestStart`: where a test run starts in the front end (`--menu-page`, `--message`, the test races with `--race-go`, the race-end and pause pages) |
| `app/app_launch.cpp`, `app/app_boot.cpp`, `app/app_window.cpp` | App's side of the command line (`useLaunchSettings`, `testStartSources`) and of start-up, its public side (its folders, lists, audio, the start scene and cameras, the track summary; the window, the stored resolution, ImGui and the mouse wheel) |
| `gfx/command_list.hpp`, `gfx/device.hpp`, `gfx/pipeline_spec.hpp`, `gfx/gfx_types.hpp`, `gfx/vulkan/*`, `gfx/d3d12/*` | the backend interface: GPU handles, `gfx::CommandList` (what the scene, effect passes and markings record through) and `gfx::Device` (the pipelines by `ShaderProgram`, buffers, textures, layer targets, texture / material sets in set pools); `gfx::Presenter` (the frame loop); `gfx::UiRenderer` (ImGui's drawing, the HUD's blends); `gfx::Backend` (the three for one window, chosen with `--renderer`); `gfx/d3d12/d3d12_shaders.hpp` (the shaders' DXIL); `VulkanCommandList`, `VulkanDevice`, `VulkanLayerTarget`, `VulkanFrameCopy`, `VulkanPresenter`, `VulkanUiRenderer`; the Direct3D 12 backend (`D3D12Context`, `D3D12Device`, `D3D12CommandList`, `D3D12Presenter`, `D3D12UiRenderer`, `D3D12Backend`) |
| `gfx/vulkan/vulkan_gpu_context.cpp` | `GpuContext`: the Vulkan instance / device / queue, buffers, textures, shader modules, one-shot commands, upload batches |
| `gfx/vulkan/vulkan_swapchain.cpp` | `Swapchain`: the swapchain, the scene render pass, its MSAA / depth targets and framebuffers |
| `gfx/vulkan/vulkan_frame_copy.cpp` | `VulkanFrameCopy`: the window's image copied (the scene transition, the Hud Lens source, the menus' background feedback) behind `gfx::FrameCopy` |
| `render/order_independent_translucency.cpp` | `OrderIndependentTranslucency`: Argon Options' Translucency (`--oit on`), the alpha-blended translucent meshes' solid pixels drawn first, their other pixels into weighted blended OIT targets composited over the scene, then their depth ([differences.md](differences.md)); `render/depth_lift.hpp` lifts every translucent draw a hair onto its ground |
| `render/post_anti_aliasing.cpp` | `PostAntiAliasingPass`: Argon Options' Post AA over the 3D view, FXAA or SMAA 1x (its edges / weights layer targets and tables), before ImGui |
| `render/scene_recorder.cpp` | `SceneRecorder`: records a frame's 3D scene (models in the game's pass order, ships, pickups, shots, water, effects, editor markings) |
| `app/app_gpu.cpp` | the renderer's set-up: frame sync, descriptor layouts / pools, samplers, pipelines (`PipelineSpec`) |
| `app/app_track.cpp`, `app/track_arrival.cpp` | the race track: its blocking load, the start and top cameras; `TrackArrival` (a load's request and steps: the old track retired, the data, sounds, scenery and editor markers, pickup models, first showing) |
| `app/scene_frame.cpp` | `SceneFrame`: the frame the 3D scene records into (SceneHost): the frame and view slots, the windings, the model clock, the spray texture, the pickup models posed, the countdown digits, the editor's markings |
| `app/race_craft_poses.cpp`, `logic/craft_poses.hpp` | `CraftPoses`: where the race's crafts are drawn (a slot's craft, the player's between its steps, a slot's start, the tick share), read by the race, the HUD and the scene |
| `logic/mode_pages.hpp`, `render/frame_clocks.hpp` | `ModePages`: the pages a game mode leads to (the shell's); `FrameClocks`: the model and GLFW clocks (SceneFrame's) |
| `app/race_player_pads.cpp`, `logic/player_pads.hpp` | `PlayerPads`: each race player's pad (the port joined with in multiplayer) and whether the race is multiplayer |
| `render/model_builder.cpp` | `ModelBuilder`: a read model made into draws (materials, texture sets), the environment cubes, the game's water, posing and skinning, releasing; `Track` / `Draw` are in render/gpu_model.hpp |
| `render/prop_models.cpp` | `PropModels`: the craft, pickup and explosion models, loaded on demand and kept across tracks |
| `render/retired_tracks.cpp` | `RetiredTracks`: replaced tracks and their pools, freed once no frame in flight reads them |
| `model/weapon_bones.cpp` | `WeaponBones`: the weapon and shield bones a prop hides (data\weapon.nwp) |
| `model/model_source.cpp`, `track/track_source.cpp` | reading a model / everything a track's race needs from disk (no Vulkan: runs on a worker thread) |
| `track/track_load.cpp` | `TrackLoad`: hands a read track to the GPU and the audio engine a slice per frame (the loading screen) |
| `app/app_effects.cpp` | the crafts' effects, the ships the shadows are cast from, the screen effects |
| `render/ship_shadows.cpp` | `ShipShadows`: the shadow maps, the casters' passes, the receiver triangles |
| `render/flare_queries.cpp` | `FlareQueries`: the flares' occlusion queries and intensities (rule and test points: render/flare_occlusion) |
| `effects/engine_trails.cpp`, `effects/wake_spray.cpp`, `effects/craft_exhausts.cpp` | the crafts' effects: engine trails (`EngineTrails`), water spray (`WakeSpray`), jets and flaps (`CraftExhausts`) |
| `app/app_drive.cpp` | game physics drive (AI or player pad), which shot the drive camera takes |
| `render/effect_buffers.cpp` | `EffectBuffers`: the effects' pipelines, textures and vertex buffers (trails, spray, trail and track sprites, particles) |
| `hud/hud_painter.cpp` | `HudPainter`: the HUD textures, fonts, text renderer, panels (`App::painter`) |
| `logic/race_recording.cpp`, `logic/replay_cameras.cpp` | `RaceRecording`: the race recorded for View Replay; `ReplayCameras`: the replay director as the viewer runs it |
| `logic/drive_camera.cpp` | `DriveCamera`: the drive views, the TV director, the chase and ship-mounted cameras |
| `input/player_input.cpp`, `input/pad_mapping.cpp` | `PlayerInput`: the keyboard and controller ports; what a gamepad state means for a craft and the menus |
| `app/app_overlay.cpp` | App's side of the 2D layer: loading the HUD textures, string tables and fonts (`loadFonts`, `matchFontsToLanguage`), the brightness overlay, racer names; the drawing itself is `HudPainter` |
| `ui/game_strings.cpp` | `GameStrings`: the game's text by id from the loaded language's tables (`App::strings`), with the caller's fallback |
| `ui/viewer_text.cpp` | Argon's own menu text (Argon Options, its main menu rows) in each language |
| `files/string_table.cpp` | reading a string table (`Locale\<language>\*.str`) by id |
| `hud/game_hud.cpp`, `hud/hud_elements.cpp`, `hud/hud_minimap.cpp`, `hud/markers.cpp` | the in-race HUD as the game's CGameHud: `GameHud` (layout, update, render) and its elements (text, sprite, power-up panel, points popup, minimap with the position list); the crosshairs and nemesis marker |
| `app/app_ui.cpp` | the frame's 2D layer (the front end, or the race's HUD and menus) |
| `ui_extras/editor_panel.cpp` | `EditorPanel`: argon's side panel |
| `ui_extras/fps.cpp` | `FpsMeter`: the FPS meter (Argon Options' FPS Meter or F2, off by default; the Debug menu), a line graph of the last 120 frame times |
| `app/app_frame.cpp` | per-frame camera, the views, the frame's command buffer, the frame's outcome (`--shot`, F12), main loop, shutdown |
| `render/scene_constants.cpp` | the scene constants' rules (race and front end, the sun); `render/scene_buffers.cpp` their buffers per frame and view |
| `ui/fe_canvas.hpp` | `FeCanvas` (what the front end and the race menus draw on), `MenuRowLook`, `Hints`; the painter's `TextStyle` / `PanelStyle` are in hud/hud_painter.hpp |
| `render/scene_views.cpp` | a view's window area and aspect from the game's 640 x 480 layout |
| `logic/race.cpp` (`Race`, `App::race`) | the race manager: the drones on the grid (AI pilots, catch-up, overtaking, craft contacts), places, the standings (laps, race times, kills, finish order) |
| `app/weapons.cpp`, `app/weapon_shots.cpp`, `app/weapon_hits.cpp`, `app/weapon_explosions.cpp`, `app/weapon_ai.cpp`, `app/weapon_replays.cpp`, `app/app_weapon_host.cpp` | `Weapons`: firing, shots and beams, hits and shields, explosions, the AI's trigger, a replay's weapons; `AppWeaponHost`: App as their host |
| `app/app_race.cpp`, `app/app_pickups.cpp` | the grid slots as drawn, what a power-up pickup gives |
| `logic/weapon_book.cpp`, `logic/explosions.cpp`, `effects/weapon_sprites.cpp` | weapon.nwp (`WeaponBook`), the explosions in flight (`Explosions`), the trail sprites, dust and flashes (`WeaponSprites`) |
| `logic/pickups.cpp`, `logic/particles.cpp` | the pickups (`Pickups`: taken, back, once per lap) and the point particles (`Particles`: bursts, sparks, trails, bubbles) |
| `app/app_intro.cpp` | the race start (grid intro, countdown) |
| `movie/theora_reader.cpp`, `movie/ycbcr.cpp`, `movie/movie_player.cpp`, `render/video_texture.cpp`, `ui/intro_screens.cpp` | the intro videos: Theora decoding (libogg + libtheora), Y'CbCr to BGRX, playback, the streamed texture (uploaded through `gfx::CommandList::copyToTexture`), FE INTROSCREENS |
| `movie/nms_sequence.cpp`, `app/app_sequence.cpp` | `NmsSequence`: the game's movies ([sequences/README.md](../sequences/README.md)); the loading screen's flyby (`<t>_glory.nms`) |
| `audio/audio.cpp`, `audio/audio.hpp` | the sound system: track sounds, engine, voices, UI sounds, music ([audio/README.md](../audio/README.md)) |
| `audio/audio_output.cpp`, `audio/voice.cpp`, `audio/peak_limiter.cpp`, `audio/miniaudio.c` | sound output through miniaudio: the device and the master limiter, one playing sound (`Voice`) |
| `audio/music_sequence.cpp`, `audio/song_stream.cpp` | the music played back to back: the playlist, its songs decoded on a worker, one stream the audio thread moves through ([audio/music.md](../audio/music.md) "Music") |
| `audio/wav.cpp`, `audio/wav.hpp` | WAV files: the Xbox ADPCM / PCM16 decoder (`Pcm`, `loadWav`), `writeWav` |
| `audio/custom_music.cpp`, `app/app_music.cpp`, `audio/song_files.cpp`, `audio/codecs.c` | custom music: `CustomMusic` (the soundtracks, the playlist, Music Source), the profile's copy and the race's music; the song files, the FLAC / Ogg / MP3 decoders |
| `audio/game_sound_files.cpp` | the game's sounds by their .wav names: a folder's sounds from its .wav / .flac / .ogg files, the converted copies of a .wav |
| `craft/drive.cpp`, `craft/craftsim.cpp`, `model/cmm.cpp`, `model/cmm_load.cpp`, `files/dds.cpp`, `files/png.cpp`, `track/trackdata.cpp` | physics, file formats |
| `render/bump_basis.cpp`, `render/water_basis.cpp` | the tangent bases of the road bump effect and of the water ([models/effects.md](../models/effects.md) "Effects", [water/README.md](../water/README.md)) |
| `logic/water_waves.cpp`, `model/cmm_morph.cpp`, `track/track_visibility.cpp` | the water's waves for the physics (0xce6b0), morph keyframes, visdata.vis culling |

The menus' game logic is in `src/app`, `src/logic` and `src/saves` ([frontend/README.md](../frontend/README.md)):

| file | what |
|---|---|
| `saves/profile_store.cpp`, `saves/profile_fields.cpp` | profiles: `<game>\profiles\*.json` (`ProfileStore`: the list, the one in use, saving; the JSON's field encodings) |
| `app/settings_keeper.cpp`, `app/app_settings.cpp` | `SettingsKeeper`: what the Game / Sound Settings and Argon Options choices do, the profile's "options", settings.json; App's F2 / F11 toggles and the translated languages |
| `ui/settings_menu.cpp` | `SettingsMenu`: the Game / Sound Settings and Argon Options rows, their choices, the profile's options string (`App::settingsMenu`) |
| `app/app_unlocks.cpp` | easter eggs, cheats (codes, unlock all), the open speeds |
| `logic/game_logic.cpp`, `logic/time_attack.cpp` | the game modes (`GameLogic`: Tournament, Quick Race, Time Attack; `GameModes` holds one of each); `App::logic()` is the selected one; Time Attack's ghosts (recording, the session and loaded ghosts, the ghost crafts) |
| `app/app_lapline.cpp` | the lap line's turbo (every mode), then the mode's `lapLineCrossed` |
| `saves/viewer_settings.cpp` | `ViewerSettings`: `save_data\settings.json` (Argon Options' settings, the key bindings) |
| `saves/ghost_library.cpp` | `GhostLibrary`: Time Attack's saved ghosts in `<saves>\ghosts` |
| `app/front_end_stores.cpp` | what the pages save and load through: `GhostShelf` (a ghost's file name and row text), `LapRecordSaver`, `CustomMusicStore`, `ProfileSwitcher` (a profile made, deleted, used) |
| `saves/lap_records.cpp`, `app/app_laprecords.cpp` | `LapRecordBook`: the lap records table and its file; `ProfileBestLaps`: the profile's own; the race's side (the race's best, new records) |
| `app/app_tourney.cpp`, `app/app_score.cpp`, `app/app_results.cpp` | tournament progress; race points, cash, high scores; the race end's steps (the player's lap reward, POSTABLE when the player finishes; the race-end pages' input and draw) |
| `logic/race_points.cpp`, `logic/race_score.cpp` | `RacePoints`: the HUD's points, popups and count-up; `CashModifiers`, `parseLapGoals`, `scoreRace`: the config's values and SCOREMENU's sum |
| `app/app_split.cpp`, `logic/multiplayer_session.cpp`, `logic/multiplayer_rules.hpp` | the split screen: each player's view and pad; `MultiplayerSession`: the players, their picks and Wins; the multiplayer race's end |
| `app/app_loader.cpp` | loading the race behind the loading screen, the track folders |
| `app/fe_shell_drawing.cpp` | `FeShellDrawing`: the front-end shell's page models (drawn from ImGui callbacks) and menu bars |
| `app/race_session.hpp`, `logic/race_picks.hpp`, `logic/tick_accumulator.hpp` | App's race state: `RaceSession` (who drives, held / paused / over, the race's counters), `RacePicks` (the character, course row and speed picked, the race being loaded), `TickAccumulator` (the frames' time in 60 Hz ticks) |
| `app/editor_session.hpp`, `app/editor_track_load.cpp`, `track/route_report.cpp` | the editor's state: `EditorSession` (open, its last track, its tool windows), `EditorTrackLoad` (a picked track loading behind its screen), `argon::RouteReport` (the route's summary and issues) |

Their pages and drawing are in `src/ui` (the front end and the race's menus). Each front-end page is a class, as in
the game (each FE page an object with its own vtable): `FePage` (`ui/fe_page.hpp`) has `enter`, `input` (every
frame, with this frame's presses), `step` (60 Hz), `draw`, `title`, `ticker`, `bare`, `sounds` (its UI sounds,
`ui/fe_sounds.hpp`) and `scene` (its 3D models: an `FeScene`, `ui/fe_scene.hpp`, with which of the shell's two cameras, the
placement, the light and the fade-in; `FrameRecorder::prepareFrontEndModels` draws it). `FeShell` (`ui/fe_shell.hpp`, `App::fe`) is the
game's CFEShell: it owns one object per `menu::Page` (`ui/page_id.hpp`), the front end's models (`FeModels`, `ui/fe_models.hpp`: the main
menu items, title logo, globes, and the 16 characters and ships preloaded one per frame), its 60 Hz clock, the title
and ticker, the update (input, steps, UI sounds) and the draw. `fe->goTo(id)` switches to a page and runs its `enter`,
and a page with arguments has its own `open(...)` (`fe->speedSelect().open(pilot, track)`).
A page keeps its own cursor and scroll state; what other pages or the race read (`App::picks`, a `RacePicks`; the game
mode, `App::modes`; the profile; the settings lists) stays in `App`. FE MESSAGEBOX is a page too (`MessageBoxPage`, `fe->messageBox()`):
`open(text, kind, *this)` runs it over the current page until it closes, then calls the asking page's
`messageClosed(yes)` (`MessageListener`).

| file | what |
|---|---|
| `ui/menu_common.hpp` | the menus' colours and layout, `menu::fe` / `menu::race` (the text factor 0.75 / 1), and the helpers' arithmetic (blink, cursor, row slide, row panel, ticker) |
| `ui/fe_page.hpp`, `ui/fe_scene.hpp` | the page base class; a page's 3D scene and the shell's cameras |
| `ui/fe_shell.hpp`, `ui/fe_shell.cpp` | the shell: the page objects by `menu::Page`, `goTo`, the update (input, 60 Hz steps, ticker, UI sounds) |
| `ui/fe_shell_sources.hpp`, `ui/fe_shell_host.hpp` | what the shell is built from: `FeShellSources` (its own state), `FePageSources` (each page's), `FeShellHost` (the drawing App does for it) |
| `ui/fe_models.hpp`, `ui/fe_models.cpp` | the front end's models: loading, the character / ship cache and preload, which one is showing |
| `ui/menu_kit.cpp` | `MenuKit`, the shared pieces: `drawMenuRow`, `listRowLook` / `listLayout`, `drawHints`, `drawGradientFrame`, `drawScrollMarks`, `menuBlink`, `fontLine`, `menuCursor` / `stepCursor`, `wrapText`; `blinkColour` and `slideRows` are in `ui/menu_common.hpp` |
| `ui/menu.cpp` | the shell's draw (background feedback, the page's models, title and ticker) |
| `app/app_menu.cpp` | `App::openMenu` over the race / editor (and the editor from Game Demos), the cursor, the page models' callbacks, the menu bars, the scene transition |
| `app/app_pause.cpp` | the race paused and resumed, a racer's pad pulled out, restarting or leaving the race |
| `ui/mainmenu.cpp` | FE MAIN |
| `ui/charsel.cpp` | Character Select, Char Info, Ship Info, Course Select |
| `ui/title.cpp`, `ui/loader.cpp` | the title screen; the loading screen |
| `ui/choosesave.cpp`, `ui/keyboard.cpp` (`ui/profiles.hpp`), `ui/messagebox.cpp` | Choose Save; the on-screen keyboard (Name Entry, cheat codes); the message box |
| `ui/options.cpp`, `ui/eggs.cpp`, `ui/records.cpp`, `ui/credits.cpp`, `ui/playlist.cpp` | Options, Game / Sound Settings, Music Select; Easter Eggs, Cheats; records; credits; Edit Playlist |
| `ui/speed.cpp`, `ui/tourney.cpp`, `ui/ghostselect.cpp`, `ui/ghostsave.cpp` | Select Speed, Track Data; Tournament; Select Ghost, Save Ghost |
| `ui/race_menu.hpp`, `ui/race_menus.hpp`, `ui/race_menus.cpp` | the race's menu base class `RaceMenu` (`input`, `draw`, its `background`); `RaceMenus` (`App::raceMenus`) owns them and picks the one `RaceSession::raceEnd` (0x52afa8) shows |
| `ui/pause.cpp`, `ui/results.cpp`, `ui/score.cpp`, `ui/upgrade.cpp` | the race's menus: PAUSEMENU; POSTABLE, WINMENU / LOSEMENU; SCOREMENU, POINTSMENU; UPGRADEMENU |

A new page: add it to `menu::Page` (ui/page_id.hpp), write a `FePage` subclass in `src/ui` (its header next to its .cpp), and
give `FeShell` a member for it, in `menu::Page` order in `pagesById` (the constructor checks the count); what it is bound
to goes in `FePageSources` (ui/fe_shell_sources.hpp), filled by `App::fePageSources` (app/app_menu.cpp); draw its rows
with `drawMenuRow` / `drawHints` and move its cursor with `menuCursor`; a page with a 3D model overrides `scene()`
(its model from `fe->models()`).

New methods can simply be declared in `app.hpp` and defined in the matching file.
