// Argon: the settings lists the option pages show (ui\options.cpp) and the pause menu steps: the profile's Game and
// Sound Settings (lists 0x81fd0 / 0x824c0, 0x81d80) and the viewer's own Argon Options (settings.json), split into its
// General, Graphics, Audio and Game Fixes pages (ui\argon_options.cpp). Each row is a
// type, its label and description ids, its option texts and the one chosen. What a choice does (the volumes, the
// window, the renderer) and where it is written are App's (app_settings.cpp).
#pragma once
#include "saves/viewer_settings.hpp"
#include "window/resolution.hpp"

#include <array>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>

class GameStrings;

struct SettingRow
{
    int type, label, desc;   // label / desc: fe string ids (the viewer's own rows: their ViewerText)
    std::vector<std::u16string> opts;
    int chosen, defaultChoice;
    bool oneAtATime = false;   // only the chosen option drawn, between < and > (Argon Options' Resolution, Language)
};

enum class SettingsList
{
    Game,
    Sound,
    ArgonGeneral,
    ArgonGraphics,
    ArgonAudio,
    ArgonFixes,
};
inline constexpr int kSettingsListCount = 6;

// Argon Options' pages: their rows change nothing until Save applies and writes them (settings.json)
inline bool isArgonList(SettingsList list)
{
    return list != SettingsList::Game && list != SettingsList::Sound;
}

// what Argon Options' rows offer and start from
struct ViewerChoices
{
    const ViewerSettings &settings;
    std::span<const Language> languages;            // the translated ones, English first
    std::span<const argon::Resolution> resolutions;   // the monitor's, narrowest first
};

class SettingsMenu
{
  public:
    // the game's rows the viewer reads by type
    static constexpr int kVibrationFirst = 2;   // 2..5: Controller 1..4 Vibration
    static constexpr int kSpeedUnits = 6;
    static constexpr int kBrightness = 8;
    static constexpr int kEngineVolume = 0xb;
    static constexpr int kSfxVolume = 0xc;
    static constexpr int kMusicVolume = 0xd;
    static constexpr int kHud = 0xf;
    static constexpr int kLaps = 0x10;
    // the viewer's own rows (Argon Options, the pause menu's HUD Size), in settings.json
    static constexpr int kHudSize = 0x100;
    static constexpr int kWidescreen = 0x101;
    static constexpr int kResolution = 0x103;
    static constexpr int kVsync = 0x104;
    static constexpr int kSave = 0x105;   // Argon Options' Save row (in place of Exit)
    static constexpr int kFullscreen = 0x106;
    static constexpr int kFpsMeter = 0x107;
    static constexpr int kSpeakers = 0x108;      // stereo or 5.1
    static constexpr int kHrtf = 0x109;          // Xbox or MIT KEMAR
    static constexpr int kMsaa = 0x10a;          // Off / 2x / 4x / 8x
    static constexpr int kPostAa = 0x10b;        // Off / FXAA / SMAA
    static constexpr int kTranslucency = 0x10c;  // Game Order / Order Independent
    static constexpr int kKemarTaps = 0x10d;     // 64 / 128 / 256 / 512
    static constexpr int kFmv = 0x10e;           // the intro videos Off / On
    static constexpr int kLanguage = 0x10f;
    static constexpr int kRenderer = 0x110;      // Vulkan / Direct3D 12, at the next start
    static constexpr int kSpeedBlur = 0x111;
    static constexpr int kMarsBlurFix = 0x112;   // Mars Flicker Fix

    bool hasProfileLists() const;
    void buildProfileLists(const GameStrings &strings);   // the Game and Sound Settings, at their defaults
    void clearProfileLists();
    void buildViewerList(const GameStrings &strings, const ViewerChoices &choices);   // Argon Options' lists from settings.json

    std::vector<SettingRow> &rows(SettingsList list);
    const std::vector<SettingRow> &rows(SettingsList list) const;

    int current(int type) const;   // the chosen option, 0 for a type no list has
    int optionCount(int type) const;
    std::u16string optionText(int type) const;   // the chosen option's text ("Full", "On", "4", "75%")
    void select(int type, int option);
    static void resetToDefaults(std::vector<SettingRow> &rows);
    bool isViewerSetting(int type) const;
    bool isChangeable(int type) const;   // false for a row another row's choice makes moot (KEMAR Taps under the Xbox HRTF): drawn grey
    float volumeMillibels(int type) const;   // 0x1a8564: a volume row's step as DirectSound millibels (step 4 before the lists exist)

    static std::u16string label(const SettingRow &row, const GameStrings &strings);
    static std::u16string description(const SettingRow &row, const GameStrings &strings);

    // the profile's "options" string: "type=option;" per Game / Sound row with options
    std::string profileString() const;
    void readProfileString(const std::string &text);   // the lists at their defaults, then the string's choices

    ViewerSettings chosenViewerSettings(const ViewerChoices &choices) const;   // choices.settings with Argon Options' rows in them

  private:
    std::optional<std::reference_wrapper<const SettingRow>> findRow(int type) const;
    std::vector<SettingRow> &list(SettingsList which);
    std::array<std::vector<SettingRow>, kSettingsListCount> lists_;
};
