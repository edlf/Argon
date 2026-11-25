cmake_minimum_required(VERSION 3.24)
project(argon LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
    # the C library's sscanf / fopen / ... without MSVC's "unsafe, use the _s version" deprecation (C4996)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# argon is the game alone (only --game read from the command line); ARGON_DEVELOP also builds argon_debug, the developer
# build: the track editor, the debug tools (FPS meter, T for turbo) and the command line
option(ARGON_DEVELOP "Also build argon_debug: the track editor, the debug tools and the command-line options" OFF)

find_package(Vulkan REQUIRED COMPONENTS glslc)

if(WIN32)
    set(QR_PLATFORM win32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(QR_PLATFORM linux)
else()
    message(FATAL_ERROR "argon builds on Windows and Linux only (src/platform)")
endif()

# --- third party (downloaded into the build folder) ---
include(FetchContent)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw  GIT_REPOSITORY https://github.com/glfw/glfw.git   GIT_TAG 3.4     GIT_SHALLOW TRUE)
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.91.9 GIT_SHALLOW TRUE)
# song decoders for custom music (public domain, single files): dr_flac (FLAC), stb_vorbis (Ogg Vorbis)
FetchContent_Declare(dr_libs URL https://github.com/mackron/dr_libs/archive/f12e197fd462e4c22873fbd70d11b1d2f2c1dc3c.tar.gz)
FetchContent_Declare(stb     URL https://github.com/nothings/stb/archive/78770d85c4d94787a56c7859017da963bc4f3439.tar.gz)
# JSON for the viewer's files (settings, lap records): nlohmann/json, header only
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
# vector maths: glm, header only (fetched so the build does not depend on the Vulkan SDK shipping it)
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1 GIT_SHALLOW TRUE)
# sound output: miniaudio (public domain, one header); the viewer decodes the game's sounds itself
FetchContent_Declare(miniaudio URL https://github.com/mackron/miniaudio/archive/refs/tags/0.11.21.tar.gz
    URL_HASH SHA256=6afb5c231613d2fab4f1c668b7243ff9a7d6d78a7f5a2692c133f026fe508506)
# the intro videos' picture (Theora in Ogg, made by tools/assets/convert_assets.py): libogg + libtheora's decoder (BSD)
set(INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(INSTALL_PKG_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
set(INSTALL_CMAKE_PACKAGE_MODULE OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)   # libogg's own tests
FetchContent_Declare(ogg URL https://github.com/xiph/ogg/archive/refs/tags/v1.3.6.tar.gz
    URL_HASH SHA256=95b643da661155d79db9de2fca55daed3a8d491039829def246aacb3d9201c81)
FetchContent_Declare(theora URL https://github.com/xiph/theora/archive/refs/tags/v1.2.0.tar.gz
    URL_HASH SHA256=e0c35771b425c32a052ffb358a2aed14219340ab850b48dc85b01939c0513a31)
# the post-process SMAA (MIT): the reference shader SMAA.hlsl (#included by shaders/smaa_common.glsl) and its area /
# search tables (Textures/AreaTex.h, SearchTex.h: render/post_anti_aliasing.cpp); no build of its own
FetchContent_Declare(smaa URL https://github.com/iryoku/smaa/archive/71c806a838bdd7d517df19192a20f0c61b3ca29d.tar.gz
    URL_HASH SHA256=046a435f5713f8bfe8148fe6305c2abf1409fc2122bee6070061c675e1e55f57)
FetchContent_MakeAvailable(glfw imgui dr_libs stb json glm miniaudio ogg theora smaa)   # theora, smaa: no CMake build

# the fetched libraries' headers are system headers to the code that uses them: their own warnings stay out of ours
foreach(library glfw glm::glm nlohmann_json::nlohmann_json)
    get_target_property(aliased ${library} ALIASED_TARGET)
    if(aliased)
        set(library ${aliased})
    endif()
    get_target_property(headers ${library} INTERFACE_INCLUDE_DIRECTORIES)
    set_target_properties(${library} PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${headers}")
endforeach()

# every warning for Argon's own code and tests: MSVC's /W4 (angle-bracket includes, the libraries and the SDKs, are
# external and quiet), else -Wall -Wextra -Wpedantic
function(argon_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /utf-8 /external:anglebrackets /external:W0)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(theoradec STATIC
    ${theora_SOURCE_DIR}/lib/apiwrapper.c
    ${theora_SOURCE_DIR}/lib/bitpack.c
    ${theora_SOURCE_DIR}/lib/decapiwrapper.c
    ${theora_SOURCE_DIR}/lib/decinfo.c
    ${theora_SOURCE_DIR}/lib/decode.c
    ${theora_SOURCE_DIR}/lib/dequant.c
    ${theora_SOURCE_DIR}/lib/fragment.c
    ${theora_SOURCE_DIR}/lib/huffdec.c
    ${theora_SOURCE_DIR}/lib/idct.c
    ${theora_SOURCE_DIR}/lib/info.c
    ${theora_SOURCE_DIR}/lib/internal.c
    ${theora_SOURCE_DIR}/lib/quant.c
    ${theora_SOURCE_DIR}/lib/state.c)
target_include_directories(theoradec SYSTEM PUBLIC ${theora_SOURCE_DIR}/include)
target_link_libraries(theoradec PUBLIC ogg)
target_compile_options(theoradec PRIVATE $<IF:$<C_COMPILER_ID:MSVC>,/W0,-w>)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp)
target_include_directories(imgui SYSTEM PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)

# --- shaders -> SPIR-V as C number lists, #included into the app ---
# fragment shaders are not optimised: they keep every input they declare (the vertex shader's outputs), which the
# validation layers check against the vertex shader (the driver optimises the SPIR-V anyway)
set(SHADER_OUT ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT})
set(SHADER_INCS)
# what the shaders #include: shaders/*.glsl and the SMAA reference shader
set(SHADER_INCLUDES -I ${CMAKE_SOURCE_DIR}/shaders -I ${smaa_SOURCE_DIR})
set(SHADER_INCLUDED
    ${CMAKE_SOURCE_DIR}/shaders/smaa_common.glsl
    ${smaa_SOURCE_DIR}/SMAA.hlsl
    ${CMAKE_SOURCE_DIR}/shaders/mesh_draws.glsl
    ${CMAKE_SOURCE_DIR}/shaders/mesh_shading.glsl
    ${CMAKE_SOURCE_DIR}/shaders/oit_composite.glsl
    ${CMAKE_SOURCE_DIR}/shaders/oit_split.glsl)
foreach(s
    mesh.vert
    mesh.frag
    mesh_oit.frag
    mesh_oit_opaque.frag
    line.vert
    line.frag
    water.vert
    water.frag
    shadow.vert
    shadow.frag
    shadowcast.frag
    shadowsolid.frag
    trail.vert
    trail.frag
    hudadd.vert
    hudadd.frag
    hudpanel.frag
    spray.frag
    quadsprite.vert
    quadsprite.frag
    occl.vert
    occl.frag
    fullscreen.vert
    fxaa.frag
    smaa_edges.frag
    smaa_weights.frag
    smaa_blend.frag
    oit_composite.frag
    oit_composite_ms.frag)
    set(out ${SHADER_OUT}/${s}.inc)
    set(optimise -O)
    if(s MATCHES "\\.frag$")
        set(optimise)
    endif()
    add_custom_command(OUTPUT ${out}
        COMMAND Vulkan::glslc -mfmt=num ${optimise} ${SHADER_INCLUDES} -o ${out} ${CMAKE_SOURCE_DIR}/shaders/${s}
        DEPENDS ${CMAKE_SOURCE_DIR}/shaders/${s} ${SHADER_INCLUDED}
        COMMENT "glslc ${s}")
    list(APPEND SHADER_INCS ${out})
endforeach()
# Windows: the same shaders for Direct3D 12, SPIR-V cross-compiled to HLSL (SPIRV-Cross: descriptor set N in register
# space N, the push constants in b0, the base instance in b1; clip space y flipped) and compiled to DXIL (DXC), as
# byte arrays named k<Name>Dxil (src/gfx/d3d12/d3d12_shaders.hpp). glslc does not optimise here: Direct3D links the
# stages by register, so a fragment shader keeps every input it declares (the vertex shader's outputs, in order)
if(WIN32)
    find_program(SPIRV_CROSS spirv-cross HINTS $ENV{VULKAN_SDK}/Bin REQUIRED)
    find_program(DXC dxc HINTS $ENV{VULKAN_SDK}/Bin REQUIRED)
    foreach(entry
        mesh.vert:MeshVert
        mesh.frag:MeshFrag
        mesh_oit.frag:MeshOitFrag
        mesh_oit_opaque.frag:MeshOitOpaqueFrag
        line.vert:LineVert
        line.frag:LineFrag
        water.vert:WaterVert
        water.frag:WaterFrag
        shadow.vert:ShadowVert
        shadow.frag:ShadowFrag
        shadowcast.frag:ShadowCastFrag
        shadowsolid.frag:ShadowSolidFrag
        trail.vert:TrailVert
        trail.frag:TrailFrag
        hudadd.vert:HudAddVert
        hudadd.frag:HudAddFrag
        hudpanel.frag:HudPanelFrag
        spray.frag:SprayFrag
        quadsprite.vert:QuadSpriteVert
        quadsprite.frag:QuadSpriteFrag
        occl.vert:OcclVert
        occl.frag:OcclFrag
        fullscreen.vert:FullscreenVert
        fxaa.frag:FxaaFrag
        smaa_edges.frag:SmaaEdgesFrag
        smaa_weights.frag:SmaaWeightsFrag
        smaa_blend.frag:SmaaBlendFrag
        oit_composite.frag:OitCompositeFrag
        oit_composite_ms.frag:OitCompositeMsFrag)
        string(REPLACE ":" ";" parts ${entry})
        list(GET parts 0 s)
        list(GET parts 1 name)
        set(cross_options --hlsl --shader-model 60)
        if(s MATCHES "\\.vert$")
            set(profile vs_6_0)
            # occl.vert's point size has no Direct3D equivalent: compat drops it (milestone 3 draws those as quads)
            list(APPEND cross_options --flip-vert-y --hlsl-enable-compat --hlsl-support-nonzero-basevertex-baseinstance
                 --hlsl-basevertex-baseinstance-binding 1 0)
        else()
            set(profile ps_6_0)
        endif()
        set(spv ${SHADER_OUT}/${s}.spv)
        set(hlsl ${SHADER_OUT}/${s}.hlsl)
        set(out ${SHADER_OUT}/${s}.dxil.h)
        add_custom_command(OUTPUT ${out}
            COMMAND Vulkan::glslc ${SHADER_INCLUDES} -o ${spv} ${CMAKE_SOURCE_DIR}/shaders/${s}
            COMMAND ${SPIRV_CROSS} ${spv} ${cross_options} --output ${hlsl}
            COMMAND ${DXC} -nologo -T ${profile} -E main -Fh ${out} -Vn k${name}Dxil ${hlsl}
            DEPENDS ${CMAKE_SOURCE_DIR}/shaders/${s} ${SHADER_INCLUDED}
            COMMENT "dxil ${s}")
        list(APPEND SHADER_INCS ${out})
    endforeach()
endif()
add_custom_target(shaders DEPENDS ${SHADER_INCS})

set(ARGON_SOURCES
    src/main.cpp
    src/ui_extras/fps.cpp
    src/window/window_mode.cpp
    src/launch/launch_options.cpp
    src/launch/startup.cpp
    src/launch/test_start.cpp
    src/launch/test_runs.cpp
    src/app/app_gpu.cpp
    src/app/app_effects.cpp
    src/effects/engine_trails.cpp
    src/effects/wake_spray.cpp
    src/effects/craft_exhausts.cpp
    src/app/app_track.cpp
    src/app/track_arrival.cpp
    src/app/scene_frame.cpp
    src/app/race_craft_poses.cpp
    src/app/race_player_pads.cpp
    src/app/app_drive.cpp
    src/app/app_overlay.cpp
    src/hud/hud_painter.cpp
    src/files/string_table.cpp
    src/app/app_ui.cpp
    src/logic/game_logic.cpp
    src/logic/time_attack.cpp
    src/logic/race.cpp
    src/logic/results_rules.cpp
    src/app/app_frame.cpp
    src/app/screenshot_saver.cpp
    src/app/app_split.cpp
    src/render/scene_recorder.cpp
    src/render/view_cameras.cpp
    src/render/countdown_digits.cpp
    src/render/frame_recorder.cpp
    src/render/post_anti_aliasing.cpp
    src/render/order_independent_translucency.cpp
    src/gfx/vulkan/vulkan_command_list.cpp
    src/gfx/vulkan/vulkan_device.cpp
    src/gfx/vulkan/vulkan_layer_target.cpp
    src/gfx/backend.cpp
    src/gfx/vulkan/vulkan_backend.cpp
    src/gfx/vulkan/vulkan_presenter.cpp
    src/gfx/vulkan/vulkan_ui_renderer.cpp
    src/render/effect_passes.cpp
    src/render/editor_markings.cpp
    src/gfx/vulkan/vulkan_gpu_context.cpp
    src/gfx/vulkan/vulkan_swapchain.cpp
    src/gfx/vulkan/vulkan_frame_copy.cpp
    src/render/video_texture.cpp
    src/app/app_pickups.cpp
    src/logic/pickups.cpp
    src/logic/particles.cpp
    src/app/app_race.cpp
    src/app/weapons.cpp
    src/app/app_race_crafts.cpp
    src/logic/lock_on.cpp
    src/logic/ai_trigger.cpp
    src/logic/craft_weapon.cpp
    src/logic/shot_flight.cpp
    src/app/weapon_shots.cpp
    src/app/weapon_hits.cpp
    src/app/app_knockout.cpp
    src/app/weapon_ai.cpp
    src/app/weapon_replays.cpp
    src/app/app_weapon_host.cpp
    src/logic/weapon_damage.cpp
    src/logic/weapon_effects.cpp
    src/app/app_loader.cpp
    src/app/editor_track_load.cpp
    src/app/app_settings.cpp
    src/app/app_unlocks.cpp
    src/app/app_results.cpp
    src/app/app_menu.cpp
    src/app/fe_shell_drawing.cpp
    src/app/app_launch.cpp
    src/app/app_boot.cpp
    src/app/app_window.cpp
    src/app/front_end_stores.cpp
    src/app/settings_keeper.cpp
    src/ui/read_game_strings.cpp
    src/app/cheat_code_entry.cpp
    src/app/app_pause.cpp
    src/app/app_music.cpp
    src/app/app_sequence.cpp
    src/app/app_tourney.cpp
    src/app/app_score.cpp
    src/logic/race_recording.cpp
    src/logic/replay_cameras.cpp
    src/input/pad_mapping.cpp
    src/input/player_input.cpp
    src/input/key_bindings.cpp
    src/logic/drive_camera.cpp
    src/logic/free_camera.cpp
    src/logic/tourney_win.cpp
    src/logic/race_start.cpp
    src/logic/multiplayer_session.cpp
    src/logic/race_points.cpp
    src/logic/race_score.cpp
    src/logic/race_scoring.cpp
    src/logic/start_grid.cpp
    src/logic/progress_keeper.cpp
    src/app/app_lapline.cpp
    src/app/app_laprecords.cpp
    src/saves/lap_records.cpp
    src/saves/viewer_settings.cpp
    src/saves/profile_file.cpp
    src/saves/profile_fields.cpp
    src/saves/profile_store.cpp
    src/saves/ghost_library.cpp
    src/app/app_intro.cpp
    src/model/cmm.cpp
    src/model/bone_aim.cpp
    src/model/cmm_load.cpp
    src/model/cmm_tracks.cpp
    src/model/model_source.cpp
    src/track/track_source.cpp
    src/track/track_load.cpp
    src/track/marker_lines.cpp
    src/track/route_report.cpp
    src/files/dds.cpp
    src/track/trackdata.cpp
    src/track/track_sprites.cpp
    src/render/sprite_pool.cpp
    src/files/png.cpp
    src/craft/drive.cpp
    src/craft/craftsim.cpp
    src/craft/craft_stats.cpp
    src/craft/ai_graph.cpp
    src/craft/sim_world.cpp
    src/craft/craft_forces.cpp
    src/craft/craft_motion.cpp
    src/craft/craft_engine.cpp
    src/craft/ai_pilot.cpp
    src/craft/craft_tripod.cpp
    src/logic/rumble_rules.cpp
    src/logic/blast_push.cpp
    src/logic/camera_shake.cpp
    src/craft/craft_flaps.cpp
    src/files/game_path.cpp
    src/model/model_read_queue.cpp
    src/app/app_warnings.cpp
    src/logic/race_warnings.cpp
    src/track/pickup_folders.cpp
    src/logic/replay.cpp
    src/logic/replay_race.cpp
    src/logic/rain.cpp
    src/app/app_rain.cpp
    src/app/race_weather.cpp
    src/hud/rain_overlay.cpp
    src/logic/weapon_rules.cpp
    src/logic/catch_up_rules.cpp
    src/logic/ai_routes.cpp
    src/track/ai_route_table.cpp
    src/logic/route_fork_log.cpp
    src/logic/trail_sprite.cpp
    src/logic/weapon_flash.cpp
    src/logic/explosion.cpp
    src/logic/shot_motion.cpp
    src/app/weapon_explosions.cpp
    src/logic/weapon_book.cpp
    src/logic/explosions.cpp
    src/effects/weapon_sprites.cpp
    src/logic/hover_dust.cpp
    src/logic/hull_contacts.cpp
    src/app/app_trail_sprites.cpp
    src/render/effect_buffers.cpp
    src/render/model_builder.cpp
    src/render/prop_models.cpp
    src/render/retired_tracks.cpp
    src/render/ship_shadows.cpp
    src/render/flare_queries.cpp
    src/render/flare_occlusion.cpp
    src/render/mesh_draw_records.cpp
    src/render/scene_buffers.cpp
    src/render/scene_constants.cpp
    src/render/scene_views.cpp
    src/render/renderer.cpp
    src/render/frame_copies.cpp
    src/render/camera_speed.cpp
    src/render/speed_blur.cpp
    src/render/pause_background.cpp
    src/model/weapon_bones.cpp
    src/logic/replay_writer.cpp
    src/app/app_replays.cpp
    src/logic/profile_progress.cpp
    src/logic/tourney_rules.cpp
    src/logic/lap_record_rules.cpp
    src/logic/race_lap_records.cpp
    src/logic/replay_weapons.cpp
    src/logic/replay_director.cpp
    src/app/app_replay_weapons.cpp
    src/render/bump_basis.cpp
    src/render/water_basis.cpp
    src/render/camera_facing.cpp
    src/render/d3d_lighting.cpp
    src/render/texture_bind_cache.cpp
    src/model/cmm_morph.cpp
    src/track/track_visibility.cpp
    src/logic/water_waves.cpp
    src/logic/water_wake.cpp
    src/logic/sound_rules.cpp
    src/logic/travel_loops.cpp
    src/app/app_race_sounds.cpp
    src/app/race_sounds.cpp
    # what differs between operating systems (src/platform)
    src/platform/platform_${QR_PLATFORM}.cpp
    src/platform/pad_rumble_${QR_PLATFORM}.cpp
    # NMS sequences, the movies (src/movie)
    src/movie/nms_sequence.cpp
    src/movie/loading_flyby.cpp
    src/movie/nms_reader.cpp
    src/movie/story_scene.cpp
    src/movie/story_scenes.cpp
    src/movie/speech_lines.cpp
    src/movie/head_turn.cpp
    src/movie/scene_light.cpp
    src/model/cms_animation.cpp
    src/movie/theora_reader.cpp
    src/movie/vorbis_track.cpp
    src/movie/ycbcr.cpp
    src/movie/movie_player.cpp
    # sound and music (src/audio)
    src/audio/audio.cpp
    src/audio/sound_reading.cpp
    src/audio/sound_load.cpp
    src/audio/engine_voices.cpp
    src/audio/track_loops.cpp
    src/audio/loop_voice.cpp
    src/audio/audio_output.cpp
    src/audio/voice.cpp
    src/audio/peak_limiter.cpp
    src/audio/output_dither.cpp
    src/audio/music_sequence.cpp
    src/audio/music_modes.cpp
    src/audio/music_fade.cpp
    src/audio/song_stream.cpp
    src/audio/song_feed.cpp
    src/audio/song_sources.cpp
    src/audio/wav.cpp
    src/audio/adpcm_chunk_ends.cpp
    src/audio/ship_sound_list.cpp
    src/audio/generic_sounds.cpp
    src/audio/race_sfx.cpp
    src/audio/spatial.cpp
    src/audio/hrtf.cpp
    src/audio/fir_block.cpp
    src/audio/kemar.cpp
    src/audio/voice_router.cpp
    src/audio/crosstalk.cpp
    src/audio/low_pass.cpp
    src/audio/world_effects.cpp
    src/audio/song_files.cpp
    src/audio/custom_music.cpp
    src/audio/game_sound_files.cpp
    # the in-race HUD (src/hud)
    src/hud/game_hud.cpp
    src/hud/hud_elements.cpp
    src/hud/hud_minimap.cpp
    src/hud/minimap_rules.cpp
    src/hud/markers.cpp
    src/hud/speedometer_rules.cpp
    src/hud/shield_bar_rules.cpp
    src/hud/hud_language_layout.cpp
    src/hud/hud_font.cpp
    # the menus (src/ui: the pages and their drawing; their game logic is in src/app and src/logic)
    src/ui/fe_shell.cpp
    src/ui/fe_page.cpp
    src/ui/intro_screens.cpp
    src/ui/fe_models.cpp
    src/ui/menu_kit.cpp
    src/ui/lap_time_text.cpp
    src/ui/game_strings.cpp
    src/ui/settings_menu.cpp
    src/ui/menu.cpp
    src/ui/feedback_swirl.cpp
    src/ui/mainmenu.cpp
    src/ui/viewer_text.cpp
    src/ui/title.cpp
    src/ui/choosesave.cpp
    src/ui/keyboard.cpp
    src/ui/messagebox.cpp
    src/ui/charsel.cpp
    src/ui/loader.cpp
    src/ui/options.cpp
    src/ui/argon_options.cpp
    src/ui/key_bindings_page.cpp
    src/ui/eggs.cpp
    src/ui/records.cpp
    src/ui/credits.cpp
    src/ui/playlist.cpp
    src/ui/speed.cpp
    src/ui/tourney.cpp
    src/ui/ghostselect.cpp
    src/ui/ghostsave.cpp
    src/ui/race_menus.cpp
    src/ui/multiplayer_end.cpp
    src/ui/multiplayer_select.cpp
    src/ui/pause.cpp
    src/ui/results.cpp
    src/ui/score.cpp
    src/ui/upgrade.cpp
    src/ui/awards.cpp
    src/ui/rotated_quad.cpp
    src/ui/picture_quad.cpp
    src/ui/nemesis_intro.cpp
    src/ui/message_buttons.cpp
    src/ui/tourney_icons.cpp
    src/ui/upgrade_icons.cpp
    src/ui/keyboard_moves.cpp
    src/ui/pause_rules.cpp)
if(WIN32)
    # the Direct3D 12 backend (src/gfx/d3d12, --renderer d3d12)
    list(APPEND ARGON_SOURCES
        src/gfx/d3d12/d3d12_context.cpp
        src/gfx/d3d12/d3d12_device.cpp
        src/gfx/d3d12/d3d12_command_list.cpp
        src/gfx/d3d12/d3d12_presenter.cpp
        src/gfx/d3d12/d3d12_ui_renderer.cpp
        src/gfx/d3d12/d3d12_backend.cpp
        src/gfx/range_allocator.cpp)
endif()

# the song decoders and miniaudio, compiled once for argon and argon_debug
add_library(argon_audio_codecs STATIC
    src/audio/codecs.c
    src/audio/miniaudio.c)
target_include_directories(argon_audio_codecs SYSTEM PUBLIC ${dr_libs_SOURCE_DIR} ${stb_SOURCE_DIR} ${miniaudio_SOURCE_DIR})
target_compile_definitions(argon_audio_codecs PUBLIC MA_NO_DECODING MA_NO_ENCODING)
target_compile_options(argon_audio_codecs PRIVATE $<IF:$<C_COMPILER_ID:MSVC>,/W0,-w>)
find_package(Threads REQUIRED)
target_link_libraries(argon_audio_codecs PUBLIC Threads::Threads ${CMAKE_DL_LIBS})   # miniaudio: its audio thread, backends loaded at run time

# the game executable; develop adds the developer build's extras (ARGON_DEVELOP)
function(add_argon_game target develop)
    add_executable(${target} ${ARGON_SOURCES})
    add_dependencies(${target} shaders)
    target_compile_definitions(${target} PRIVATE ARGON_DEVELOP=$<BOOL:${develop}> GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS)
    if(develop)
        # viewer extras, not the game's UI (src/ui_extras)
        target_sources(${target} PRIVATE src/ui_extras/editor_panel.cpp)
    endif()
    target_include_directories(${target} PRIVATE ${SHADER_OUT} src)
    target_include_directories(${target} SYSTEM PRIVATE ${smaa_SOURCE_DIR}/Textures)
    target_link_libraries(${target} PRIVATE argon_audio_codecs imgui glfw Vulkan::Vulkan glm::glm nlohmann_json::nlohmann_json theoradec)
    if(WIN32)
        target_link_libraries(${target} PRIVATE d3d12 dxgi Xinput)   # Xinput: platform/pad_rumble_win32.cpp
        target_compile_definitions(${target} PRIVATE NOMINMAX)
        if(MSVC)
            # vulkan-1.dll loaded when the Vulkan backend starts, not with the exe: without a Vulkan driver the game
            # still runs, on Direct3D 12 (gfx\vulkan\vulkan_backend.cpp)
            target_link_options(${target} PRIVATE /DELAYLOAD:vulkan-1.dll)
            target_link_libraries(${target} PRIVATE delayimp)
        endif()
        if(NOT develop)
            # no console window: only the developer build shows its log; main stays the entry point
            set_target_properties(${target} PROPERTIES WIN32_EXECUTABLE TRUE)
            if(MSVC)
                target_link_options(${target} PRIVATE /ENTRY:mainCRTStartup)
            endif()
        endif()
    endif()
    argon_warnings(${target})
endfunction()

add_argon_game(argon OFF)
if(ARGON_DEVELOP)
    add_argon_game(argon_debug ON)
endif()

# argon_decode <sound> <out.wav>: one sound decoded as argon plays it (src\tools\decode_sound.cpp), for
# tools\assets\convert_assets.py
add_executable(argon_decode
    src/tools/decode_sound.cpp
    src/audio/sound_load.cpp
    src/audio/wav.cpp
    src/audio/adpcm_chunk_ends.cpp
    src/audio/song_files.cpp
    src/files/game_path.cpp)
target_include_directories(argon_decode PRIVATE src)
target_link_libraries(argon_decode PRIVATE argon_audio_codecs)
argon_warnings(argon_decode)

# --- tests (tests\menu_tests.cpp: the menu helpers' arithmetic, no window / GPU); run with test.bat or ctest
enable_testing()
add_executable(argon_tests tests/menu_tests.cpp)
target_include_directories(argon_tests PRIVATE src)
target_link_libraries(argon_tests PRIVATE glm::glm)   # ghost.hpp
argon_warnings(argon_tests)
add_test(NAME menu_tests COMMAND argon_tests)

# tests\save_file_tests.cpp: the viewer's JSON files (settings.json, the lap records)
add_executable(argon_save_tests
    tests/save_file_tests.cpp
    src/saves/viewer_settings.cpp
    src/input/key_bindings.cpp
    src/saves/lap_records.cpp
    src/saves/profile_file.cpp
    src/saves/profile_fields.cpp
    src/saves/profile_store.cpp
    src/logic/profile_progress.cpp)
target_include_directories(argon_save_tests PRIVATE src)
target_link_libraries(argon_save_tests PRIVATE nlohmann_json::nlohmann_json glfw)   # glfw: the key codes
argon_warnings(argon_save_tests)
add_test(NAME save_file_tests COMMAND argon_save_tests)

# tests\game_path_tests.cpp: finding the game's files whatever their case (game_path.hpp)
add_executable(argon_game_path_tests
    tests/game_path_tests.cpp
    src/files/game_path.cpp)
target_include_directories(argon_game_path_tests PRIVATE src)
argon_warnings(argon_game_path_tests)
add_test(NAME game_path_tests COMMAND argon_game_path_tests)

# tests\peak_limiter_tests.cpp: the master limiter on the final mix (audio\peak_limiter.hpp)
add_executable(argon_peak_limiter_tests
    tests/peak_limiter_tests.cpp
    src/audio/peak_limiter.cpp)
target_include_directories(argon_peak_limiter_tests PRIVATE src)
argon_warnings(argon_peak_limiter_tests)
add_test(NAME peak_limiter_tests COMMAND argon_peak_limiter_tests)

# tests\output_dither_tests.cpp: the 16-bit TPDF dither on the final mix (audio\output_dither.hpp)
add_executable(argon_output_dither_tests
    tests/output_dither_tests.cpp
    src/audio/output_dither.cpp)
target_include_directories(argon_output_dither_tests PRIVATE src)
argon_warnings(argon_output_dither_tests)
add_test(NAME output_dither_tests COMMAND argon_output_dither_tests)

# tests\adpcm_chunk_ends_tests.cpp: the Xbox ADPCM encoder's lost chunk-end samples rebuilt (audio\adpcm_chunk_ends.hpp)
add_executable(argon_adpcm_chunk_ends_tests
    tests/adpcm_chunk_ends_tests.cpp
    src/audio/adpcm_chunk_ends.cpp)
target_include_directories(argon_adpcm_chunk_ends_tests PRIVATE src)
argon_warnings(argon_adpcm_chunk_ends_tests)
add_test(NAME adpcm_chunk_ends_tests COMMAND argon_adpcm_chunk_ends_tests)

# tests\weapon_damage_tests.cpp: a weapon hit's damage arithmetic (logic\weapon_damage.hpp, the game's 0x6acf0)
add_executable(argon_weapon_damage_tests
    tests/weapon_damage_tests.cpp
    src/logic/weapon_damage.cpp)
target_include_directories(argon_weapon_damage_tests PRIVATE src)
argon_warnings(argon_weapon_damage_tests)
add_test(NAME weapon_damage_tests COMMAND argon_weapon_damage_tests)

# tests\weapon_effects_tests.cpp: a weapon hit's lasting effects on a craft (logic\weapon_effects.hpp, 0x6d000)
add_executable(argon_weapon_effects_tests
    tests/weapon_effects_tests.cpp
    src/logic/weapon_effects.cpp)
target_include_directories(argon_weapon_effects_tests PRIVATE src)
argon_warnings(argon_weapon_effects_tests)
add_test(NAME weapon_effects_tests COMMAND argon_weapon_effects_tests)

# tests\weapon_rules_tests.cpp: drones and their catch-up rows, the nemesis race, the special's fire rule, the shields'
# fade and sphere and the trail sprites (logic\weapon_rules.hpp, logic\catch_up_rules.hpp, logic\trail_sprite.hpp:
# 0x3f3d0, 0x58e40, 0x6d650, 0x791f0, 0x6d000, 0x69970, 0x9c540)
add_executable(argon_weapon_rules_tests
    tests/weapon_rules_tests.cpp
    src/logic/weapon_rules.cpp
    src/logic/catch_up_rules.cpp
    src/logic/trail_sprite.cpp)
target_include_directories(argon_weapon_rules_tests PRIVATE src)
target_link_libraries(argon_weapon_rules_tests PRIVATE glm::glm)
argon_warnings(argon_weapon_rules_tests)
add_test(NAME weapon_rules_tests COMMAND argon_weapon_rules_tests)

# tests\weapon_flash_tests.cpp: the beams' and sparks' one-frame flashes (logic\weapon_flash.hpp: 0x68620, 0x68150 -> 0xbd670)
add_executable(argon_weapon_flash_tests
    tests/weapon_flash_tests.cpp
    src/logic/weapon_flash.cpp)
target_include_directories(argon_weapon_flash_tests PRIVATE src)
target_link_libraries(argon_weapon_flash_tests PRIVATE glm::glm)
argon_warnings(argon_weapon_flash_tests)
add_test(NAME weapon_flash_tests COMMAND argon_weapon_flash_tests)

# tests\craft_weapon_tests.cpp: a craft's weapon object (logic\craft_weapon.hpp: 0x6aa00, 0x6b1b8, 0x6d000, 0x69970)
add_executable(argon_craft_weapon_tests
    tests/craft_weapon_tests.cpp
    src/logic/craft_weapon.cpp
    src/logic/ai_trigger.cpp
    src/logic/weapon_effects.cpp
    src/logic/weapon_rules.cpp)
target_include_directories(argon_craft_weapon_tests PRIVATE src)
target_link_libraries(argon_craft_weapon_tests PRIVATE glm::glm)
argon_warnings(argon_craft_weapon_tests)
add_test(NAME craft_weapon_tests COMMAND argon_craft_weapon_tests)

# tests\lock_on_tests.cpp: lock-on and the crosshairs (logic\lock_on.hpp, logic\race_crafts.hpp: 0x6c420, 0x4e7f0)
add_executable(argon_lock_on_tests
    tests/lock_on_tests.cpp
    src/logic/lock_on.cpp
    src/logic/ai_trigger.cpp)
target_include_directories(argon_lock_on_tests PRIVATE src)
target_link_libraries(argon_lock_on_tests PRIVATE glm::glm)
argon_warnings(argon_lock_on_tests)
add_test(NAME lock_on_tests COMMAND argon_lock_on_tests)

# tests\weapons_tests.cpp: weapon.nwp's book, the explosions in flight, the weapon sprites (logic\weapon_book.hpp,
# logic\explosions.hpp, effects\weapon_sprites.hpp: 0x640c0, 0x65f80, 0x7e0f0, 0x9d000)
add_executable(argon_weapons_tests
    tests/weapons_tests.cpp
    src/logic/weapon_book.cpp
    src/logic/explosions.cpp
    src/logic/explosion.cpp
    src/logic/trail_sprite.cpp
    src/logic/weapon_flash.cpp
    src/effects/weapon_sprites.cpp
    src/render/sprite_pool.cpp)
target_include_directories(argon_weapons_tests PRIVATE src)
target_link_libraries(argon_weapons_tests PRIVATE glm::glm)
argon_warnings(argon_weapons_tests)
add_test(NAME weapons_tests COMMAND argon_weapons_tests)

# tests\race_effects_tests.cpp: the engine trails and the water spray (effects\engine_trails.hpp, effects\wake_spray.hpp: 0x50960, 0x9db10, 0x9c540)
add_executable(argon_race_effects_tests
    tests/race_effects_tests.cpp
    src/effects/engine_trails.cpp
    src/effects/wake_spray.cpp
    src/logic/water_wake.cpp
    src/logic/rain.cpp
    src/render/sprite_pool.cpp)
target_include_directories(argon_race_effects_tests PRIVATE src)
target_link_libraries(argon_race_effects_tests PRIVATE glm::glm)
argon_warnings(argon_race_effects_tests)
add_test(NAME race_effects_tests COMMAND argon_race_effects_tests)

# tests\pickups_tests.cpp: the pickups and the particles (logic\pickups.hpp, logic\particles.hpp: 0x8fcd0, 0x78380, 0x88750, 0xbc640)
add_executable(argon_pickups_tests
    tests/pickups_tests.cpp
    src/logic/pickups.cpp
    src/logic/particles.cpp)
target_include_directories(argon_pickups_tests PRIVATE src)
target_link_libraries(argon_pickups_tests PRIVATE glm::glm)
argon_warnings(argon_pickups_tests)
add_test(NAME pickups_tests COMMAND argon_pickups_tests)

# tests\attack_warnings_tests.cpp: ATTACK WARNING's danger rules (logic\attack_warnings.hpp: 0x66e60, 0x683b0, 0x4d720)
add_executable(argon_attack_warnings_tests tests/attack_warnings_tests.cpp)
target_include_directories(argon_attack_warnings_tests PRIVATE src)
target_link_libraries(argon_attack_warnings_tests PRIVATE glm::glm)
argon_warnings(argon_attack_warnings_tests)
add_test(NAME attack_warnings_tests COMMAND argon_attack_warnings_tests)

# tests\explosion_tests.cpp: the weapons' explosions, impact flashes and sparks (logic\explosion.hpp: 0x65f80, 0x7e0f0,
# 0x7de80, 0x6a8a0, 0x67231, 0x67e81)
add_executable(argon_explosion_tests
    tests/explosion_tests.cpp
    src/logic/explosion.cpp
    src/logic/trail_sprite.cpp)
target_include_directories(argon_explosion_tests PRIVATE src)
target_link_libraries(argon_explosion_tests PRIVATE glm::glm)
argon_warnings(argon_explosion_tests)
add_test(NAME explosion_tests COMMAND argon_explosion_tests)

# tests\shot_motion_tests.cpp: a projectile's frame at launch and in flight (logic\shot_motion.hpp: 0x64e20, 0x66d30)
add_executable(argon_shot_motion_tests
    tests/shot_motion_tests.cpp
    src/logic/shot_motion.cpp)
target_include_directories(argon_shot_motion_tests PRIVATE src)
target_link_libraries(argon_shot_motion_tests PRIVATE glm::glm)
argon_warnings(argon_shot_motion_tests)
add_test(NAME shot_motion_tests COMMAND argon_shot_motion_tests)

# tests\shot_flight_tests.cpp: projectiles, beams and volleys in flight (logic\shot_flight.hpp: 0x66d30, 0x683b0, 0x67753,
# 0x675f2, 0x69e30)
add_executable(argon_shot_flight_tests
    tests/shot_flight_tests.cpp
    src/logic/shot_flight.cpp
    src/logic/shot_motion.cpp)
target_include_directories(argon_shot_flight_tests PRIVATE src)
target_link_libraries(argon_shot_flight_tests PRIVATE glm::glm)
argon_warnings(argon_shot_flight_tests)
add_test(NAME shot_flight_tests COMMAND argon_shot_flight_tests)

# tests\ai_routes_tests.cpp: the AI's route at the AI nodes' forks: the list each craft takes and the branch each list
# picks (logic\ai_routes.hpp: 0xe1370, 0xe4730, 0xe3460, 0xe3740, 0xe35d0, 0xe16e0, 0xe38c0)
add_executable(argon_ai_routes_tests
    tests/ai_routes_tests.cpp
    src/logic/ai_routes.cpp)
target_include_directories(argon_ai_routes_tests PRIVATE src)
argon_warnings(argon_ai_routes_tests)
add_test(NAME ai_routes_tests COMMAND argon_ai_routes_tests)

# tests\hover_dust_tests.cpp: the hover's dust behind a craft on rough / loose ground (logic\hover_dust.hpp: 0x5f377..0x5f667)
add_executable(argon_hover_dust_tests
    tests/hover_dust_tests.cpp
    src/logic/hover_dust.cpp
    src/logic/trail_sprite.cpp)
target_include_directories(argon_hover_dust_tests PRIVATE src)
target_link_libraries(argon_hover_dust_tests PRIVATE glm::glm)
argon_warnings(argon_hover_dust_tests)
add_test(NAME hover_dust_tests COMMAND argon_hover_dust_tests)

# tests\vorbis_track_tests.cpp: a converted video's soundtrack out of the muxed Ogg file (movie\vorbis_track.hpp)
add_executable(argon_vorbis_track_tests
    tests/vorbis_track_tests.cpp
    src/movie/vorbis_track.cpp)
target_include_directories(argon_vorbis_track_tests PRIVATE src)
target_link_libraries(argon_vorbis_track_tests PRIVATE ogg)
argon_warnings(argon_vorbis_track_tests)
add_test(NAME vorbis_track_tests COMMAND argon_vorbis_track_tests)

# tests\d3d12_shaders_tests.cpp: the shaders cross-compiled for Direct3D 12 are DXIL (gfx\d3d12\d3d12_shaders.hpp)
if(WIN32)
    add_executable(argon_d3d12_shaders_tests tests/d3d12_shaders_tests.cpp)
    add_dependencies(argon_d3d12_shaders_tests shaders)
    target_include_directories(argon_d3d12_shaders_tests PRIVATE src ${SHADER_OUT})
    argon_warnings(argon_d3d12_shaders_tests)
    add_test(NAME d3d12_shaders_tests COMMAND argon_d3d12_shaders_tests)
endif()

# tests\ycbcr_tests.cpp: the video frames' colour conversion (movie\ycbcr.hpp)
add_executable(argon_ycbcr_tests
    tests/ycbcr_tests.cpp
    src/movie/ycbcr.cpp)
target_include_directories(argon_ycbcr_tests PRIVATE src)
argon_warnings(argon_ycbcr_tests)
add_test(NAME ycbcr_tests COMMAND argon_ycbcr_tests)

# tests\song_files_tests.cpp: song files, the converted game sounds' names and loop comments, the decodes (audio\song_files.hpp)
add_executable(argon_song_files_tests
    tests/song_files_tests.cpp
    src/audio/song_files.cpp
    src/audio/game_sound_files.cpp
    src/audio/codecs.c)
target_include_directories(argon_song_files_tests PRIVATE src ${dr_libs_SOURCE_DIR} ${stb_SOURCE_DIR})
argon_warnings(argon_song_files_tests)
add_test(NAME song_files_tests COMMAND argon_song_files_tests)

# tests\string_table_tests.cpp: reading the string tables, Locale\<language>\*.str (string_table.hpp), and looking text up in them
# (ui\game_strings.hpp)
add_executable(argon_string_table_tests
    tests/string_table_tests.cpp
    src/files/string_table.cpp
    src/ui/game_strings.cpp
    src/ui/viewer_text.cpp)
target_include_directories(argon_string_table_tests PRIVATE src)
argon_warnings(argon_string_table_tests)
add_test(NAME string_table_tests COMMAND argon_string_table_tests)

# tests\drive_input_tests.cpp: the drive views (logic\drive_camera.hpp) and what a gamepad's state means
# (input\pad_mapping.hpp)
add_executable(argon_drive_input_tests
    tests/drive_input_tests.cpp
    src/logic/drive_camera.cpp
    src/input/pad_mapping.cpp)
target_include_directories(argon_drive_input_tests PRIVATE src)
target_link_libraries(argon_drive_input_tests PRIVATE glm::glm glfw)
argon_warnings(argon_drive_input_tests)
add_test(NAME drive_input_tests COMMAND argon_drive_input_tests)

# tests\race_start_tests.cpp: the grid intro and countdown clocks, the race's clock and limit, the player's knock-out
# (logic\race_start.hpp, logic\race_clock.hpp)
add_executable(argon_race_start_tests
    tests/race_start_tests.cpp
    src/logic/race_start.cpp)
target_include_directories(argon_race_start_tests PRIVATE src)
argon_warnings(argon_race_start_tests)
add_test(NAME race_start_tests COMMAND argon_race_start_tests)

# tests\race_score_tests.cpp: a race's points (the HUD's popups and count-up) and score (the config's values, SCOREMENU's sum)
add_executable(argon_race_score_tests
    tests/race_score_tests.cpp
    src/logic/race_points.cpp
    src/logic/race_score.cpp)
target_include_directories(argon_race_score_tests PRIVATE src)
argon_warnings(argon_race_score_tests)
add_test(NAME race_score_tests COMMAND argon_race_score_tests)

# tests\custom_music_tests.cpp: custom music's soundtracks, the playlist and its profile string (audio\custom_music.hpp)
add_executable(argon_custom_music_tests
    tests/custom_music_tests.cpp
    src/audio/custom_music.cpp
    src/audio/song_files.cpp
    src/audio/game_sound_files.cpp
    src/audio/codecs.c)
target_include_directories(argon_custom_music_tests PRIVATE src ${dr_libs_SOURCE_DIR} ${stb_SOURCE_DIR})
argon_warnings(argon_custom_music_tests)
add_test(NAME custom_music_tests COMMAND argon_custom_music_tests)

# tests\ghost_library_tests.cpp: Time Attack's saved ghosts, listed, saved and removed (saves\ghost_library.hpp)
add_executable(argon_ghost_library_tests
    tests/ghost_library_tests.cpp
    src/saves/ghost_library.cpp)
target_include_directories(argon_ghost_library_tests PRIVATE src)
target_link_libraries(argon_ghost_library_tests PRIVATE glm::glm)
argon_warnings(argon_ghost_library_tests)
add_test(NAME ghost_library_tests COMMAND argon_ghost_library_tests)

# tests\settings_menu_tests.cpp: the settings lists, the profile's options string and Argon Options' rows (ui\settings_menu.hpp)
add_executable(argon_settings_menu_tests
    tests/settings_menu_tests.cpp
    src/ui/settings_menu.cpp
    src/ui/game_strings.cpp
    src/ui/viewer_text.cpp
    src/files/string_table.cpp
    src/saves/viewer_settings.cpp
    src/input/key_bindings.cpp)
target_include_directories(argon_settings_menu_tests PRIVATE src)
target_link_libraries(argon_settings_menu_tests PRIVATE nlohmann_json::nlohmann_json glfw)   # glfw: the key codes
argon_warnings(argon_settings_menu_tests)
add_test(NAME settings_menu_tests COMMAND argon_settings_menu_tests)

# tests\story_scene_tests.cpp: the story scenes: which plays when, the speech lines, posing with other tracks and,
# with game_data, the NMS dopesheet's timing and the CMS animations (movie\story_scenes.hpp, movie\nms_sequence.hpp)
add_executable(argon_story_scene_tests
    tests/story_scene_tests.cpp
    src/movie/story_scenes.cpp
    src/movie/speech_lines.cpp
    src/movie/head_turn.cpp
    src/movie/scene_light.cpp
    src/movie/nms_sequence.cpp
    src/movie/nms_reader.cpp
    src/model/cms_animation.cpp
    src/model/cmm.cpp
    src/model/bone_aim.cpp
    src/model/cmm_load.cpp
    src/model/cmm_tracks.cpp
    src/model/cmm_morph.cpp
    src/files/game_path.cpp)
target_include_directories(argon_story_scene_tests PRIVATE src)
target_link_libraries(argon_story_scene_tests PRIVATE glm::glm)
target_compile_definitions(argon_story_scene_tests PRIVATE QR_SOURCE_DIR="${CMAKE_SOURCE_DIR}" GLM_FORCE_RADIANS)
argon_warnings(argon_story_scene_tests)
add_test(NAME story_scene_tests COMMAND argon_story_scene_tests)

# tests\ship_sound_tests.cpp: the pilot's ship sound list and the engine power down / up sounds
add_executable(argon_ship_sound_tests
    tests/ship_sound_tests.cpp
    src/audio/ship_sound_list.cpp)
target_include_directories(argon_ship_sound_tests PRIVATE src)
argon_warnings(argon_ship_sound_tests)
add_test(NAME ship_sound_tests COMMAND argon_ship_sound_tests)

# tests\grid_starts_tests.cpp: the crafts' start locators, the player last (logic\grid_starts.hpp)
add_executable(argon_grid_starts_tests tests/grid_starts_tests.cpp)
target_include_directories(argon_grid_starts_tests PRIVATE src)
target_link_libraries(argon_grid_starts_tests PRIVATE glm::glm)   # trackdata.hpp
argon_warnings(argon_grid_starts_tests)
add_test(NAME grid_starts_tests COMMAND argon_grid_starts_tests)

# tests\ship_views_tests.cpp: the ship views the Black button cycles (logic\ship_views.hpp)
add_executable(argon_ship_views_tests tests/ship_views_tests.cpp)
target_include_directories(argon_ship_views_tests PRIVATE src)
argon_warnings(argon_ship_views_tests)
add_test(NAME ship_views_tests COMMAND argon_ship_views_tests)

# tests\multiplayer_rules_tests.cpp: the race's laps (logic\race_laps.hpp), the multiplayer race's laps and end
# (logic\multiplayer_rules.hpp) and its players (logic\multiplayer_session.hpp)
add_executable(argon_multiplayer_tests
    tests/multiplayer_rules_tests.cpp
    src/logic/multiplayer_session.cpp)
target_include_directories(argon_multiplayer_tests PRIVATE src)
argon_warnings(argon_multiplayer_tests)
add_test(NAME multiplayer_rules_tests COMMAND argon_multiplayer_tests)

# tests\results_rules_tests.cpp: the race's results fixed at the player's finish: the finish order and POSTABLE's rows
# (logic\results_rules.hpp: 0x63850, 0x638b0, 0x3f320, 0x8f140)
add_executable(argon_results_rules_tests
    tests/results_rules_tests.cpp
    src/logic/results_rules.cpp)
target_include_directories(argon_results_rules_tests PRIVATE src)
argon_warnings(argon_results_rules_tests)
add_test(NAME results_rules_tests COMMAND argon_results_rules_tests)

# tests\split_screen_tests.cpp: the split screen's views and HUD frames (logic\split_screen.hpp)
add_executable(argon_split_screen_tests tests/split_screen_tests.cpp)
target_include_directories(argon_split_screen_tests PRIVATE src)
argon_warnings(argon_split_screen_tests)
add_test(NAME split_screen_tests COMMAND argon_split_screen_tests)

# tests\music_modes_tests.cpp: the game's music modes (audio\music_modes.hpp), the race events that change them and
# the race end's music fades (audio\music_fade.hpp)
add_executable(argon_music_modes_tests
    tests/music_modes_tests.cpp
    src/audio/music_modes.cpp
    src/audio/music_fade.cpp
    src/audio/game_sound_files.cpp)
target_include_directories(argon_music_modes_tests PRIVATE src)
argon_warnings(argon_music_modes_tests)
add_test(NAME music_modes_tests COMMAND argon_music_modes_tests)

# tests\music_sequence_tests.cpp: the music played back to back (audio\music_sequence.hpp), mixed without a sound card
add_executable(argon_music_tests
    tests/music_sequence_tests.cpp
    src/audio/music_sequence.cpp
    src/audio/song_stream.cpp
    src/audio/song_feed.cpp
    src/audio/audio_output.cpp
    src/audio/peak_limiter.cpp
    src/audio/output_dither.cpp
    src/audio/world_effects.cpp
    src/audio/hrtf.cpp
    src/audio/fir_block.cpp
    src/audio/kemar.cpp
    src/audio/voice_router.cpp
    src/audio/crosstalk.cpp
    src/audio/low_pass.cpp
    src/audio/miniaudio.c)
target_include_directories(argon_music_tests PRIVATE src ${miniaudio_SOURCE_DIR})
target_compile_definitions(argon_music_tests PRIVATE MA_NO_DECODING MA_NO_ENCODING)
target_link_libraries(argon_music_tests PRIVATE Threads::Threads glm::glm ${CMAKE_DL_LIBS})
argon_warnings(argon_music_tests)
add_test(NAME music_sequence_tests COMMAND argon_music_tests)

# tests\song_feed_tests.cpp: a song decoded a few seconds ahead of where it plays (audio\song_feed.hpp)
add_executable(argon_song_feed_tests
    tests/song_feed_tests.cpp
    src/audio/song_feed.cpp
    src/audio/miniaudio.c)
target_include_directories(argon_song_feed_tests PRIVATE src ${miniaudio_SOURCE_DIR})
target_compile_definitions(argon_song_feed_tests PRIVATE MA_NO_DECODING MA_NO_ENCODING)
target_link_libraries(argon_song_feed_tests PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
argon_warnings(argon_song_feed_tests)
add_test(NAME song_feed_tests COMMAND argon_song_feed_tests)

# tests\surround_mix_tests.cpp: each voice path through the mix in stereo and 5.1 (audio\world_effects.hpp), without a sound card
add_executable(argon_surround_mix_tests
    tests/surround_mix_tests.cpp
    src/audio/voice.cpp
    src/audio/audio_output.cpp
    src/audio/peak_limiter.cpp
    src/audio/output_dither.cpp
    src/audio/world_effects.cpp
    src/audio/hrtf.cpp
    src/audio/fir_block.cpp
    src/audio/kemar.cpp
    src/audio/voice_router.cpp
    src/audio/crosstalk.cpp
    src/audio/low_pass.cpp
    src/audio/miniaudio.c)
target_include_directories(argon_surround_mix_tests PRIVATE src ${miniaudio_SOURCE_DIR})
target_compile_definitions(argon_surround_mix_tests PRIVATE MA_NO_DECODING MA_NO_ENCODING)
target_link_libraries(argon_surround_mix_tests PRIVATE Threads::Threads glm::glm ${CMAKE_DL_LIBS})
argon_warnings(argon_surround_mix_tests)
add_test(NAME surround_mix_tests COMMAND argon_surround_mix_tests)

# tests\rumble_rules_tests.cpp: a craft's controller rumble, the knocked-out fade and the pad's motor thresholds
add_executable(argon_rumble_rules_tests
    tests/rumble_rules_tests.cpp
    src/logic/rumble_rules.cpp)
target_include_directories(argon_rumble_rules_tests PRIVATE src)
argon_warnings(argon_rumble_rules_tests)
add_test(NAME rumble_rules_tests COMMAND argon_rumble_rules_tests)

# tests\blast_knock_tests.cpp: an explosion's push on a craft (0x5dee0) and the internal camera's shake (0x5b610 / 0x5ba10)
add_executable(argon_blast_knock_tests
    tests/blast_knock_tests.cpp
    src/logic/blast_push.cpp
    src/logic/camera_shake.cpp)
target_include_directories(argon_blast_knock_tests PRIVATE src)
target_link_libraries(argon_blast_knock_tests PRIVATE glm::glm)
argon_warnings(argon_blast_knock_tests)
add_test(NAME blast_knock_tests COMMAND argon_blast_knock_tests)

# tests\craft_reset_tests.cpp: what puts a craft back on the track (craftsim.cpp, craft +5 / 0x5cb30)
add_executable(argon_craft_reset_tests
    tests/craft_reset_tests.cpp
    src/craft/craftsim.cpp
    src/craft/craft_stats.cpp
    src/craft/ai_graph.cpp
    src/craft/sim_world.cpp
    src/craft/craft_forces.cpp
    src/craft/craft_motion.cpp
    src/craft/craft_engine.cpp
    src/craft/ai_pilot.cpp
    src/craft/craft_tripod.cpp
    src/logic/hull_contacts.cpp
    src/logic/rumble_rules.cpp
    src/logic/blast_push.cpp
    src/logic/camera_shake.cpp
    src/logic/ai_routes.cpp
    src/logic/hover_dust.cpp
    src/logic/trail_sprite.cpp
    src/craft/drive.cpp
    src/track/trackdata.cpp
    src/track/track_sprites.cpp
    src/render/sprite_pool.cpp
    src/files/game_path.cpp
    src/model/cmm.cpp
    src/model/bone_aim.cpp
    src/track/track_visibility.cpp
    src/logic/water_waves.cpp
    src/logic/weapon_effects.cpp)
target_include_directories(argon_craft_reset_tests PRIVATE src)
target_link_libraries(argon_craft_reset_tests PRIVATE glm::glm)
argon_warnings(argon_craft_reset_tests)
add_test(NAME craft_reset_tests COMMAND argon_craft_reset_tests)

# tests\free_camera_tests.cpp: the viewer's free camera (logic\free_camera.hpp)
add_executable(argon_free_camera_tests
    tests/free_camera_tests.cpp
    src/logic/free_camera.cpp)
target_include_directories(argon_free_camera_tests PRIVATE src)
target_link_libraries(argon_free_camera_tests PRIVATE glm::glm)
argon_warnings(argon_free_camera_tests)
add_test(NAME free_camera_tests COMMAND argon_free_camera_tests)

# tests\race_lap_records_tests.cpp: the race's side of the lap records (logic\race_lap_records.hpp)
add_executable(argon_race_lap_records_tests
    tests/race_lap_records_tests.cpp
    src/logic/race_lap_records.cpp
    src/logic/lap_record_rules.cpp
    src/saves/lap_records.cpp
    src/saves/profile_store.cpp
    src/saves/profile_file.cpp
    src/saves/profile_fields.cpp
    src/logic/profile_progress.cpp
    src/files/string_table.cpp
    src/ui/game_strings.cpp
    src/ui/viewer_text.cpp)
target_include_directories(argon_race_lap_records_tests PRIVATE src)
target_link_libraries(argon_race_lap_records_tests PRIVATE nlohmann_json::nlohmann_json)
argon_warnings(argon_race_lap_records_tests)
add_test(NAME race_lap_records_tests COMMAND argon_race_lap_records_tests)

# testsront_end_stores_tests.cpp: the front end's stores: profiles, the tests' eggs, lap records (appront_end_stores.hpp)
add_executable(argon_front_end_stores_tests
    tests/front_end_stores_tests.cpp
    src/app/front_end_stores.cpp
    src/logic/race_lap_records.cpp
    src/logic/lap_record_rules.cpp
    src/logic/progress_keeper.cpp
    src/saves/lap_records.cpp
    src/saves/ghost_library.cpp
    src/saves/profile_store.cpp
    src/saves/profile_file.cpp
    src/saves/profile_fields.cpp
    src/logic/profile_progress.cpp
    src/files/string_table.cpp
    src/ui/game_strings.cpp
    src/ui/viewer_text.cpp
    src/craft/craft_stats.cpp
    src/files/game_path.cpp
    src/logic/tourney_rules.cpp
    src/audio/custom_music.cpp
    src/audio/song_files.cpp
    src/audio/game_sound_files.cpp
    src/audio/codecs.c)
target_include_directories(argon_front_end_stores_tests PRIVATE src ${dr_libs_SOURCE_DIR} ${stb_SOURCE_DIR})
target_link_libraries(argon_front_end_stores_tests PRIVATE imgui glm::glm nlohmann_json::nlohmann_json)
argon_warnings(argon_front_end_stores_tests)
add_test(NAME front_end_stores_tests COMMAND argon_front_end_stores_tests)

# tests\camera_speed_tests.cpp: the speed blur's speed (render\camera_speed.hpp)
add_executable(argon_camera_speed_tests
    tests/camera_speed_tests.cpp
    src/render/camera_speed.cpp)
target_include_directories(argon_camera_speed_tests PRIVATE src)
target_link_libraries(argon_camera_speed_tests PRIVATE glm::glm)
argon_warnings(argon_camera_speed_tests)
add_test(NAME camera_speed_tests COMMAND argon_camera_speed_tests)

# tests\collision_grid_tests.cpp: the collision grid's box query (craft\drive.hpp)
add_executable(argon_collision_grid_tests
    tests/collision_grid_tests.cpp
    src/craft/drive.cpp
    src/track/trackdata.cpp
    src/track/track_sprites.cpp
    src/render/sprite_pool.cpp
    src/files/game_path.cpp
    src/model/cmm.cpp
    src/model/bone_aim.cpp
    src/track/track_visibility.cpp)
target_include_directories(argon_collision_grid_tests PRIVATE src)
target_link_libraries(argon_collision_grid_tests PRIVATE glm::glm)
argon_warnings(argon_collision_grid_tests)
add_test(NAME collision_grid_tests COMMAND argon_collision_grid_tests)

# tests\range_allocator_tests.cpp: the shared buffer blocks' byte ranges (gfx\range_allocator.hpp)
add_executable(argon_range_allocator_tests
    tests/range_allocator_tests.cpp
    src/gfx/range_allocator.cpp)
target_include_directories(argon_range_allocator_tests PRIVATE src)
argon_warnings(argon_range_allocator_tests)
add_test(NAME range_allocator_tests COMMAND argon_range_allocator_tests)

# tests\hull_contacts_tests.cpp: the craft's box against the track's triangles (logic\hull_contacts.hpp: 0xd3c70)
add_executable(argon_hull_contacts_tests
    tests/hull_contacts_tests.cpp
    src/logic/hull_contacts.cpp)
target_include_directories(argon_hull_contacts_tests PRIVATE src)
target_link_libraries(argon_hull_contacts_tests PRIVATE glm::glm)
argon_warnings(argon_hull_contacts_tests)
add_test(NAME hull_contacts_tests COMMAND argon_hull_contacts_tests)

# tests\race_warnings_tests.cpp: the HUD's RESET / WRONG WAY / Shield Low triggers (logic\race_warnings.hpp)
add_executable(argon_race_warnings_tests
    tests/race_warnings_tests.cpp
    src/logic/race_warnings.cpp)
target_include_directories(argon_race_warnings_tests PRIVATE src)
target_link_libraries(argon_race_warnings_tests PRIVATE glm::glm)
argon_warnings(argon_race_warnings_tests)
add_test(NAME race_warnings_tests COMMAND argon_race_warnings_tests)

# tests\sound_rules_tests.cpp: the race's event sounds (logic\sound_rules.hpp, logic\travel_loops.hpp)
add_executable(argon_sound_rules_tests
    tests/sound_rules_tests.cpp
    src/logic/sound_rules.cpp
    src/logic/travel_loops.cpp)
target_include_directories(argon_sound_rules_tests PRIVATE src)
target_link_libraries(argon_sound_rules_tests PRIVATE glm::glm)
argon_warnings(argon_sound_rules_tests)
add_test(NAME sound_rules_tests COMMAND argon_sound_rules_tests)

# tests\audio_effects_tests.cpp: the game sounds' low-pass, HRTF, crosstalk and doppler
add_executable(argon_audio_effects_tests
    tests/audio_effects_tests.cpp
    src/audio/low_pass.cpp
    src/audio/spatial.cpp
    src/audio/hrtf.cpp
    src/audio/fir_block.cpp
    src/audio/kemar.cpp
    src/audio/voice_router.cpp
    src/audio/crosstalk.cpp)
target_include_directories(argon_audio_effects_tests PRIVATE src)
target_link_libraries(argon_audio_effects_tests PRIVATE glm::glm)
target_compile_definitions(argon_audio_effects_tests PRIVATE QR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
argon_warnings(argon_audio_effects_tests)
add_test(NAME audio_effects_tests COMMAND argon_audio_effects_tests)

# tests\rain_tests.cpp: the race's rain (logic\rain.hpp)
add_executable(argon_rain_tests
    tests/rain_tests.cpp
    src/logic/rain.cpp)
target_include_directories(argon_rain_tests PRIVATE src)
target_link_libraries(argon_rain_tests PRIVATE glm::glm)
argon_warnings(argon_rain_tests)
add_test(NAME rain_tests COMMAND argon_rain_tests)

# tests\race_eggs_tests.cpp: the easter eggs a race runs with (logic\race_eggs.hpp) and the Fruity Fruit pickup models
add_executable(argon_race_eggs_tests
    tests/race_eggs_tests.cpp
    src/track/pickup_folders.cpp)
target_include_directories(argon_race_eggs_tests PRIVATE src)
argon_warnings(argon_race_eggs_tests)
add_test(NAME race_eggs_tests COMMAND argon_race_eggs_tests)

# tests\replay_tests.cpp: the replay reader (logic\replay.hpp) on game_data\replays (skipped without them)
add_executable(argon_replay_tests
    tests/replay_tests.cpp
    src/logic/replay.cpp
    src/logic/replay_weapons.cpp)
target_include_directories(argon_replay_tests PRIVATE src)
target_link_libraries(argon_replay_tests PRIVATE glm::glm)
target_compile_definitions(argon_replay_tests PRIVATE QR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
argon_warnings(argon_replay_tests)
add_test(NAME replay_tests COMMAND argon_replay_tests)

# tests\replay_writer_tests.cpp: recording races in the game's replay format (logic\replay_writer.hpp), the replays' lap
# count and the attract demo's idle clock
add_executable(argon_replay_writer_tests
    tests/replay_writer_tests.cpp
    src/logic/replay.cpp
    src/logic/replay_writer.cpp
    src/logic/replay_weapons.cpp
    src/logic/race_recording.cpp)
target_include_directories(argon_replay_writer_tests PRIVATE src)
target_link_libraries(argon_replay_writer_tests PRIVATE glm::glm)
argon_warnings(argon_replay_writer_tests)
add_test(NAME replay_writer_tests COMMAND argon_replay_writer_tests)

# tests\frontend_tests.cpp: the front end's leftovers (rotated quad, Nemesis Challenge intro, message box buttons,
# FE TOURNEYTYPE's icons, UPGRADEMENU's weapon icons, the nemesis race, Percentage Complete)
add_executable(argon_frontend_tests
    tests/frontend_tests.cpp
    src/ui/rotated_quad.cpp
    src/ui/nemesis_intro.cpp
    src/ui/message_buttons.cpp
    src/ui/tourney_icons.cpp
    src/ui/upgrade_icons.cpp
    src/logic/tourney_rules.cpp
    src/logic/tourney_win.cpp
    src/logic/profile_progress.cpp
    src/ui/keyboard_moves.cpp
    src/logic/lap_record_rules.cpp
    src/ui/pause_rules.cpp
    src/ui/viewer_text.cpp)
target_include_directories(argon_frontend_tests PRIVATE src)
target_compile_definitions(argon_frontend_tests PRIVATE QR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(argon_frontend_tests PRIVATE glm::glm)
argon_warnings(argon_frontend_tests)
add_test(NAME frontend_tests COMMAND argon_frontend_tests)

# tests/replay_director_tests.cpp: the replay cameras' director (logic/replay_director.hpp)
add_executable(argon_replay_director_tests
    tests/replay_director_tests.cpp
    src/logic/replay_director.cpp)
target_include_directories(argon_replay_director_tests PRIVATE src)
target_link_libraries(argon_replay_director_tests PRIVATE glm::glm)
argon_warnings(argon_replay_director_tests)
add_test(NAME replay_director_tests COMMAND argon_replay_director_tests)

# tests\bump_basis_tests.cpp: the "Diffuse Specular Bump Map" effect's tangent basis (render\bump_basis.hpp, 0xa2dd0)
add_executable(argon_bump_basis_tests
    tests/bump_basis_tests.cpp
    src/render/bump_basis.cpp)
target_include_directories(argon_bump_basis_tests PRIVATE src)
target_link_libraries(argon_bump_basis_tests PRIVATE glm::glm)
argon_warnings(argon_bump_basis_tests)
add_test(NAME bump_basis_tests COMMAND argon_bump_basis_tests)

# tests\cmm_morph_tests.cpp: the morph meshes' keyframe blend (cmm_morph.hpp, 0x7b7e0) and objects\explodefirelarge
add_executable(argon_cmm_morph_tests
    tests/cmm_morph_tests.cpp
    src/model/cmm.cpp
    src/model/bone_aim.cpp
    src/model/cmm_load.cpp
    src/model/cmm_tracks.cpp
    src/model/cmm_morph.cpp
    src/files/game_path.cpp)
target_include_directories(argon_cmm_morph_tests PRIVATE src)
target_link_libraries(argon_cmm_morph_tests PRIVATE glm::glm)
target_compile_definitions(argon_cmm_morph_tests PRIVATE QR_SOURCE_DIR="${CMAKE_SOURCE_DIR}" GLM_FORCE_RADIANS)
argon_warnings(argon_cmm_morph_tests)
add_test(NAME cmm_morph_tests COMMAND argon_cmm_morph_tests)

# tests\water_waves_tests.cpp: the water's wave heightfield and depth query for the physics (logic\water_waves.hpp), its
# bump basis (render\water_basis.hpp) and the wake's walk (logic\water_wake.hpp)
add_executable(argon_water_waves_tests
    tests/water_waves_tests.cpp
    src/logic/water_waves.cpp
    src/logic/water_wake.cpp
    src/render/water_basis.cpp
    src/render/bump_basis.cpp)
target_include_directories(argon_water_waves_tests PRIVATE src)
target_link_libraries(argon_water_waves_tests PRIVATE glm::glm)
argon_warnings(argon_water_waves_tests)
add_test(NAME water_waves_tests COMMAND argon_water_waves_tests)

# tests\track_visibility_tests.cpp: the tracks' visibility sectors (track_visibility.hpp, visdata.vis, 0xafcf0)
add_executable(argon_track_visibility_tests
    tests/track_visibility_tests.cpp
    src/track/track_visibility.cpp
    src/model/cmm.cpp
    src/model/bone_aim.cpp
    src/model/cmm_load.cpp
    src/model/cmm_tracks.cpp
    src/model/cmm_morph.cpp
    src/files/game_path.cpp)
target_include_directories(argon_track_visibility_tests PRIVATE src)
target_link_libraries(argon_track_visibility_tests PRIVATE glm::glm)
target_compile_definitions(argon_track_visibility_tests PRIVATE QR_SOURCE_DIR="${CMAKE_SOURCE_DIR}" GLM_FORCE_RADIANS)
argon_warnings(argon_track_visibility_tests)
add_test(NAME track_visibility_tests COMMAND argon_track_visibility_tests)

# tests\minimap_rules_tests.cpp: the HUD minimap's map per track (hud\minimap_rules.hpp)
add_executable(argon_minimap_rules_tests
    tests/minimap_rules_tests.cpp
    src/hud/minimap_rules.cpp)
target_include_directories(argon_minimap_rules_tests PRIVATE src)
argon_warnings(argon_minimap_rules_tests)
add_test(NAME minimap_rules_tests COMMAND argon_minimap_rules_tests)

# tests\speedometer_rules_tests.cpp: the speedometer's second needle and its jitter (hud\speedometer_rules.hpp)
add_executable(argon_speedometer_rules_tests
    tests/speedometer_rules_tests.cpp
    src/hud/speedometer_rules.cpp)
target_include_directories(argon_speedometer_rules_tests PRIVATE src)
argon_warnings(argon_speedometer_rules_tests)
add_test(NAME speedometer_rules_tests COMMAND argon_speedometer_rules_tests)

# tests\route_report_tests.cpp: the editor's route summary and issue tally (track\route_report.hpp)
add_executable(argon_route_report_tests
    tests/route_report_tests.cpp
    src/track/route_report.cpp)
target_include_directories(argon_route_report_tests PRIVATE src)
target_link_libraries(argon_route_report_tests PRIVATE glm::glm)
argon_warnings(argon_route_report_tests)
add_test(NAME route_report_tests COMMAND argon_route_report_tests)

# tests\tick_accumulator_tests.cpp: the frames' time in 60 Hz ticks (logic\tick_accumulator.hpp)
add_executable(argon_tick_accumulator_tests tests/tick_accumulator_tests.cpp)
target_include_directories(argon_tick_accumulator_tests PRIVATE src)
argon_warnings(argon_tick_accumulator_tests)
add_test(NAME tick_accumulator_tests COMMAND argon_tick_accumulator_tests)

# tests\shield_bar_rules_tests.cpp: the shield bar's fill colour, its flash and low-shield blinks (hud\shield_bar_rules.hpp)
add_executable(argon_shield_bar_rules_tests
    tests/shield_bar_rules_tests.cpp
    src/hud/shield_bar_rules.cpp)
target_include_directories(argon_shield_bar_rules_tests PRIVATE src)
argon_warnings(argon_shield_bar_rules_tests)
add_test(NAME shield_bar_rules_tests COMMAND argon_shield_bar_rules_tests)

# tests\hud_language_layout_tests.cpp: the race HUD's Japanese offsets (hud\hud_language_layout.hpp)
add_executable(argon_hud_language_layout_tests
    tests/hud_language_layout_tests.cpp
    src/hud/hud_language_layout.cpp)
target_include_directories(argon_hud_language_layout_tests PRIVATE src)
argon_warnings(argon_hud_language_layout_tests)
add_test(NAME hud_language_layout_tests COMMAND argon_hud_language_layout_tests)

# tests\hud_font_tests.cpp: the HUD fonts' glyph boxes and the replacement font's atlas (hud\hud_font.hpp)
add_executable(argon_hud_font_tests
    tests/hud_font_tests.cpp
    src/hud/hud_font.cpp)
target_include_directories(argon_hud_font_tests PRIVATE src ${stb_SOURCE_DIR})
target_compile_definitions(argon_hud_font_tests PRIVATE QR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
argon_warnings(argon_hud_font_tests)
add_test(NAME hud_font_tests COMMAND argon_hud_font_tests)

# tests\track_sprites_tests.cpp: sprite.dat's quads (track_sprites.hpp: 0xbdf40, the sprite pass 0xbd8f0)
add_executable(argon_track_sprites_tests
    tests/track_sprites_tests.cpp
    src/track/track_sprites.cpp
    src/render/sprite_pool.cpp)
target_include_directories(argon_track_sprites_tests PRIVATE src)
target_link_libraries(argon_track_sprites_tests PRIVATE glm::glm)
target_compile_definitions(argon_track_sprites_tests PRIVATE QR_SOURCE_DIR="${CMAKE_SOURCE_DIR}" GLM_FORCE_RADIANS)
argon_warnings(argon_track_sprites_tests)
add_test(NAME track_sprites_tests COMMAND argon_track_sprites_tests)

# tests\sprite_pool_tests.cpp: the sprite pool's quads (sprite_pool.hpp: 0xbd670, 0x9c540, quadsprite.xvu, 0xbd8f0)
add_executable(argon_sprite_pool_tests
    tests/sprite_pool_tests.cpp
    src/render/sprite_pool.cpp)
target_include_directories(argon_sprite_pool_tests PRIVATE src)
target_link_libraries(argon_sprite_pool_tests PRIVATE glm::glm)
target_compile_definitions(argon_sprite_pool_tests PRIVATE GLM_FORCE_RADIANS)
argon_warnings(argon_sprite_pool_tests)
add_test(NAME sprite_pool_tests COMMAND argon_sprite_pool_tests)

# tests\flare_occlusion_tests.cpp: the flares' occlusion rule (render\flare_occlusion.hpp, 0xb6e60)
add_executable(argon_flare_occlusion_tests
    tests/flare_occlusion_tests.cpp
    src/render/flare_occlusion.cpp)
target_include_directories(argon_flare_occlusion_tests PRIVATE src)
target_link_libraries(argon_flare_occlusion_tests PRIVATE glm::glm)
argon_warnings(argon_flare_occlusion_tests)
add_test(NAME flare_occlusion_tests COMMAND argon_flare_occlusion_tests)

# tests\camera_facing_tests.cpp: meshes placed by the camera (render\camera_facing.hpp, 0xb7e72 / 0xb7765 / 0xb4e70)
add_executable(argon_camera_facing_tests
    tests/camera_facing_tests.cpp
    src/render/camera_facing.cpp)
target_include_directories(argon_camera_facing_tests PRIVATE src)
target_link_libraries(argon_camera_facing_tests PRIVATE glm::glm)
argon_warnings(argon_camera_facing_tests)
add_test(NAME camera_facing_tests COMMAND argon_camera_facing_tests)

# tests\depth_lift_tests.cpp: the translucent pass's depth lift (render\depth_lift.hpp)
add_executable(argon_depth_lift_tests tests/depth_lift_tests.cpp)
target_include_directories(argon_depth_lift_tests PRIVATE src)
target_link_libraries(argon_depth_lift_tests PRIVATE glm::glm)
target_compile_definitions(argon_depth_lift_tests PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS)   # the app's depth range
argon_warnings(argon_depth_lift_tests)
add_test(NAME depth_lift_tests COMMAND argon_depth_lift_tests)

# tests\mesh_fog_tests.cpp: a mesh's fog as the surface setup sets it (render\mesh_fog.hpp, 0xba2ac / 0xa8450)
add_executable(argon_mesh_fog_tests tests/mesh_fog_tests.cpp)
target_include_directories(argon_mesh_fog_tests PRIVATE src)
argon_warnings(argon_mesh_fog_tests)
add_test(NAME mesh_fog_tests COMMAND argon_mesh_fog_tests)

# tests\window_placement_tests.cpp: the monitor F11's borderless fullscreen covers (window_placement.hpp)
add_executable(argon_window_placement_tests tests/window_placement_tests.cpp)
target_include_directories(argon_window_placement_tests PRIVATE src)
argon_warnings(argon_window_placement_tests)
add_test(NAME window_placement_tests COMMAND argon_window_placement_tests)

# tests\resolution_tests.cpp: Argon Options' Resolution, kept to the monitor's sizes (window\resolution.hpp)
add_executable(argon_resolution_tests tests/resolution_tests.cpp)
target_include_directories(argon_resolution_tests PRIVATE src)
argon_warnings(argon_resolution_tests)
add_test(NAME resolution_tests COMMAND argon_resolution_tests)

# tests\scene_frame_tests.cpp: the scene constants and the views' areas (render\scene_constants.hpp, render\scene_views.hpp)
add_executable(argon_scene_frame_tests
    tests/scene_frame_tests.cpp
    src/render/scene_constants.cpp
    src/render/scene_views.cpp)
target_include_directories(argon_scene_frame_tests PRIVATE src)
target_link_libraries(argon_scene_frame_tests PRIVATE glm::glm)
argon_warnings(argon_scene_frame_tests)
add_test(NAME scene_frame_tests COMMAND argon_scene_frame_tests)

# testsiew_cameras_tests.cpp: the frame's cameras, the split screen's views (renderiew_cameras.hpp)
add_executable(argon_view_cameras_tests
    tests/view_cameras_tests.cpp
    src/render/view_cameras.cpp
    src/render/scene_views.cpp
    src/logic/free_camera.cpp
    src/logic/multiplayer_session.cpp)
target_include_directories(argon_view_cameras_tests PRIVATE src)
target_link_libraries(argon_view_cameras_tests PRIVATE glm::glm)
target_compile_definitions(argon_view_cameras_tests PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS)   # the app's depth range
argon_warnings(argon_view_cameras_tests)
add_test(NAME view_cameras_tests COMMAND argon_view_cameras_tests)

# tests\weapon_bones_tests.cpp: the weapon and shield bones a prop hides (model\weapon_bones.hpp, 0x506e0 / 0xb97f0)
add_executable(argon_weapon_bones_tests
    tests/weapon_bones_tests.cpp
    src/model/weapon_bones.cpp)
target_include_directories(argon_weapon_bones_tests PRIVATE src)
target_link_libraries(argon_weapon_bones_tests PRIVATE glm::glm)
argon_warnings(argon_weapon_bones_tests)
add_test(NAME weapon_bones_tests COMMAND argon_weapon_bones_tests)

# tests\texture_bind_cache_tests.cpp: the stages' texture-bind cache (render\texture_bind_cache.hpp, 0xcbb40 / 0xcbbd2)
add_executable(argon_texture_bind_cache_tests
    tests/texture_bind_cache_tests.cpp
    src/render/texture_bind_cache.cpp)
target_include_directories(argon_texture_bind_cache_tests PRIVATE src)
argon_warnings(argon_texture_bind_cache_tests)
add_test(NAME texture_bind_cache_tests COMMAND argon_texture_bind_cache_tests)

# tests\d3d_lighting_tests.cpp: a mesh's lighting sources and a craft's lights (render\d3d_lighting.hpp, 0xba4f0 / 0x59bc0)
add_executable(argon_d3d_lighting_tests
    tests/d3d_lighting_tests.cpp
    src/render/d3d_lighting.cpp)
target_include_directories(argon_d3d_lighting_tests PRIVATE src)
target_link_libraries(argon_d3d_lighting_tests PRIVATE glm::glm)
argon_warnings(argon_d3d_lighting_tests)
add_test(NAME d3d_lighting_tests COMMAND argon_d3d_lighting_tests)

# tools\tests: the Python tools' tests (tools\saves\profiles.py), when a Python 3 interpreter is found
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(NAME python_tools_tests
             COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_SOURCE_DIR}/tools/tests -t ${CMAKE_SOURCE_DIR}/tools/tests)
endif()
