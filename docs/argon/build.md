# Building and setting up argon

## Build
Windows: Visual Studio 2022 (C++ workload) and the Vulkan SDK.
```
build.bat
```
Output: `build\argon.exe` and `build\argon_debug.exe`; `test.bat` builds and runs the tests. Visual Studio is found with
`vswhere` wherever it is installed (`tools\visual_studio_env.bat`).

`package.ps1` builds both in Release and zips them into `dist\argon-win64.zip` with the Visual C++ runtime DLLs,
`hrtf_kemar.bin` (make it first: MIT KEMAR below), the third-party license notices and credits, and empty `game_data`
and `fonts` folders, each with a `README.txt` saying what goes in it.

Linux (not built there yet: see [argon/todo.md](todo.md) "Linux port"): a C++20 compiler (GCC or Clang), CMake 3.24+, Ninja,
the Vulkan headers and loader, `glslc` (shaderc), and GLFW's X11 / Wayland development packages. On Debian / Ubuntu
roughly `build-essential cmake ninja-build libvulkan-dev glslc libx11-dev libxrandr-dev libxinerama-dev
libxcursor-dev libxi-dev libwayland-dev libxkbcommon-dev wayland-protocols`. Rumble needs read / write access to
the pad's `/dev/input/event*` (the desktop usually grants it to the logged-in user).
```
./build.sh
```
Output: `build/argon` and `build/argon_debug`; `./test.sh` builds and runs the tests.

`argon` is the game alone: the front end at start, only `--game` read from the command line and, on Windows, no
console window (errors still show in a message box). `argon_debug` is the
developer build, made when the CMake option `ARGON_DEVELOP` is on (off by default; `build.bat` / `build.sh` turn it on):
the track editor (FE MAIN's Debug row, its windows, F1), the debug tools (the FPS meter while the Debug menu is open;
T for 5 s of turbo) and the command line below. In both, **F11** switches the window to fullscreen (the monitor it is
mostly on switched to Argon Options' Resolution), and back, and **F2** shows or hides the FPS meter (Argon Options'
FPS Meter); **F12** writes the frame to `screenshots\argon_<date>_<time>.png` next to `save_data`. For example, `argon` alone:
```
cmake -S . -B build-player -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-player --target argon
```

The first build downloads the libraries into `build/_deps`: GLFW 3.4, Dear ImGui 1.91.9, glm 1.0.1, nlohmann/json
3.11.3, dr_flac, dr_mp3 and stb_vorbis (custom music), miniaudio 0.11.21 (sound), libogg 1.3.6 and libtheora 1.2.0 (the
intro videos), and SMAA's reference shader and tables (MIT, the Post AA's SMAA).

## Folders
| folder | what |
|---|---|
| `src`, `shaders`, `tests` | argon, its shaders and tests (code map: [argon/source.md](source.md)) |
| `game_data` | the game's files (USA disc: `data`, `config`, `audio`, `Locale`, `media`, ..., the original `default.xbe`); not in git |
| `save_data` | what argon writes: `profiles` (with the settings), `ghosts`, `records`, `custom_music`; not in git |
| `tools` | Python tools by folder ([tools\README.md](../../tools/README.md)): `assets` (converting the game's media), `saves` (the game's profiles and replays), `xemu` (live checks in the running game), `xbe`, `viewer` (refactor check), `cmm` (models, tracks, Blender, Ghidra scripts) |
| `docs` | the decoded formats and game logic (the game's own saves: `docs\saves\README.md`) |

Argon finds them by walking up from the exe (or the current folder) to a folder holding `game_data\data\tracks`;
its saves go in `save_data` next to `game_data`. `--game <folder>` names that folder instead (a plain game folder, with `data\tracks`
directly in it, also works: the saves then go into its `save_data`).

## Game assets
Videos, music and sound effects: `python tools\assets\convert_assets.py [--force] [--ffmpeg <ffmpeg>] [--decoder <argon_decode.exe>] [--game <folder>]`
converts them next to the originals with ffmpeg (needs ffmpeg installed, with libtheora and libvorbis, and argon built: it decodes the
Xbox ADPCM sounds): the intro videos (Bink, .bik) to one .ogv each (Theora picture + Vorbis soundtrack, muxed), which argon plays before
the title screen (`--no-intro` skips them; A / START skips one once the front end's models have loaded, about a second); the music
(`media\music`) to lossless .flac; the sound effects (`audio`) to Ogg Vorbis, 256 kbit/s or the highest libvorbis takes for the format
(90 kbit/s for their mono 22050 Hz), every ADPCM block whole as argon decodes it, with the one-shot length (`fact`) and the loops kept
as PLAYLENGTH / LOOPSTART / LOOPLENGTH comments. Argon uses a .flac / .ogg in place of
the .wav next to it ([audio/README.md](../audio/README.md) "Converted files"); outputs newer than their source are skipped.

The 3D sounds' HRTF tables: `python -I tools\assets\extract_hrtf.py [--game <folder>] [--force]` copies them out of `default.xbe` into
`audio\hrtf.bin` ([xbe-data/README.md](../xbe-data/README.md)); argon reads that file, and the XBE itself only when it is missing.

The optional MIT KEMAR HRTF (Argon Options' HRTF): download the MIT KEMAR "full" set (Gardner & Martin, MIT Media Lab;
its `elev*` folders of `L<e>e<aaa>a.wav`), `pip install numpy`, then `python -I tools\assets\convert_kemar.py <full set folder> [--out <file>]`
writes `hrtf_kemar.bin` in the project folder, beside `game_data` (argon also finds it beside `argon.exe`; [audio/mix-levels.md](../audio/mix-levels.md) "Mix levels"): 512 taps per ear, which Argon Options'
KEMAR Taps cuts to 64, 128, 256 or 512. A 64-tap file from an older converter still plays, at 64 taps whatever the choice;
the argon before KEMAR Taps refuses the 512-tap file (it plays the Xbox's filters then).

Deleting the converted originals: `python tools\assets\delete_converted_originals.py [--delete] [--ffmpeg <ffmpeg>] [--decoder <argon_decode.exe>] [--game <folder>]`
reports, for each .bik / .wav the converter handles, whether its conversion is there, up to date and passes the converter's check again
(the dry run, with the space it would free); `--delete` deletes only those verified originals, anything else is kept and listed. The
argon runs without them (it lists the folders by their .wav names from the .flac / .ogg); the speech .wav files (`data\movies\speech`)
are not converted and stay.

## Settings
Settings: `save_data\settings.json` (written with the defaults on the first run) holds argon's own settings:
```
{
  "language": "English",
  "skipIntroVideos": false,
  "renderer": "vulkan",
  "msaa": 8,
  "postAntiAliasing": "smaa",
  "orderIndependentTranslucency": true
}
```
`language` (English, French, German, Italian, Japanese) picks `Locale\<language>` for the text and the intro
video; on the USA disc only English and Japanese have real text (French, German and Italian hold placeholder strings:
copy the EU release's `Locale` tables over them; until then Argon Options does not offer them and they start in English), and Japanese
text is drawn in the game's MSPGoth font (sharper from `fonts\MS PGothic.otf`); Argon Options' Language, applied on Save. `skipIntroVideos` starts on the title screen (Argon Options' FMVs Off). `renderer`
picks the graphics API: `vulkan` (the default) or `d3d12` (Windows, Direct3D 12); Argon Options' Renderer, taken at the next start (Save says so), `--renderer` overrides it. `msaa` (1, 2,
4 or 8, default 8; Argon Options' MSAA, taken at the next start, `--msaa` overrides it) and `postAntiAliasing` (`off`,
`fxaa` or `smaa`, default `smaa`; Argon Options' Post AA, `--post-aa` overrides it) pick the anti-aliasing.
`orderIndependentTranslucency` (default `true`; Argon Options' Translucency, `--oit on|off` overrides it) blends the
alpha-blended see-through meshes without the game's draw order (`false`: the game's order). The game's own
Options stay in the profile (`save_data\profiles\*.json`); the lap records are in `save_data\records\laptimes.json`.

## Tests
Tests: `test.bat` builds, then runs `build\argon_tests.exe` and `build\argon_save_tests.exe` (the JSON files:
`tests\save_file_tests.cpp`) through CTest
(`tests\menu_tests.cpp`: the menu helpers in `src\ui\menu_common.hpp`, checked against
the pages' original inline code; no window or GPU needed).
Refactor check: `python tools\viewer\shot_compare.py <old argon.exe> <new argon.exe> <folder>` renders the same scenes
with both builds and diffs them, then compares every track's load summary (needs Pillow).
