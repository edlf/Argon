// Argon: the settings lists (settings_menu.hpp).
#include "ui/settings_menu.hpp"

#include "audio/hrtf_set.hpp"
#include "files/string_table.hpp"
#include "ui/game_strings.hpp"
#include "ui/viewer_text.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>

namespace
{
using argon::widenLatin1;

struct SettingIds
{
    int type, label, desc;
};

// the two rows every profile list starts with: Exit (fe 8) and Reset to Default; kept in entry order, drawn bottom up
void addListHead(std::vector<SettingRow> &list)
{
    list.push_back({0, 0x8, 0x33b, {}, 0, 0});
    list.push_back({1, 0x332, 0x333, {}, 0, 0});
}

std::vector<std::u16string> offOn(const GameStrings &strings)
{
    return {strings.frontEnd(6, "Off"), strings.frontEnd(5, "On")};
}

// fe 6 "Off", fe 5 "On", default On
SettingRow onOffRow(const GameStrings &strings, const SettingIds &ids)
{
    return SettingRow{ids.type, ids.label, ids.desc, offOn(strings), 1, 1};
}

// strings.str 0x4b0..0x4b7, default 4
SettingRow stepsRow(const GameStrings &strings, const SettingIds &ids)
{
    SettingRow steps{ids.type, ids.label, ids.desc, {}, 4, 4};
    for (int step = 0; step < 8; ++step)
    {
        steps.opts.push_back(strings.general(0x4b0 + step, std::to_string(step).c_str()));
    }
    return steps;
}

// a viewer row of Off / On from a flag
SettingRow viewerOnOffRow(const GameStrings &strings, int type, bool on, bool onByDefault)
{
    return SettingRow{type, 0, 0, offOn(strings), on ? 1 : 0, onByDefault ? 1 : 0};
}

// the translated languages, default English, shown one at a time as the Resolution
SettingRow languageRow(const GameStrings &strings, const ViewerChoices &choices)
{
    SettingRow language{SettingsMenu::kLanguage, 0, 0, {}, 0, 0, true};
    for (size_t index = 0; index < choices.languages.size(); ++index)
    {
        language.opts.push_back(strings.viewer(viewer_text::kLanguageNames[int(choices.languages[index])]));
        if (choices.languages[index] == choices.settings.language())
        {
            language.chosen = int(index);
        }
    }
    return language;
}

// the monitor's sizes, narrowest first, default its largest
SettingRow resolutionRow(const ViewerChoices &choices)
{
    const std::vector<argon::Resolution> sizes(choices.resolutions.begin(), choices.resolutions.end());
    const argon::Resolution current = argon::validResolution(choices.settings.resolution(), sizes);
    const argon::Resolution fallback = argon::validResolution(std::nullopt, sizes);
    SettingRow resolution{SettingsMenu::kResolution, 0, 0, {}, 0, 0, true};
    for (size_t index = 0; index < sizes.size(); ++index)
    {
        resolution.opts.push_back(widenLatin1(sizes[index].text().c_str()));
        if (sizes[index] == current)
        {
            resolution.chosen = int(index);
        }
        if (sizes[index] == fallback)
        {
            resolution.defaultChoice = int(index);
        }
    }
    return resolution;
}

// 25 / 50 / 75 / 100 %, default 100
SettingRow hudSizeRow(const ViewerSettings &settings)
{
    constexpr int kLargest = ViewerSettings::kHudSizeSteps - 1;
    SettingRow hudSize{SettingsMenu::kHudSize, 0, 0, {}, settings.hudSizeStep(), kLargest};
    for (int step = 0; step <= kLargest; ++step)
    {
        hudSize.opts.push_back(widenLatin1((std::to_string(100 * (step + 1) / ViewerSettings::kHudSizeSteps) + "%").c_str()));
    }
    return hudSize;
}

// Off (1 sample) / 2x / 4x / 8x, default 8x
SettingRow msaaRow(const GameStrings &strings, const ViewerSettings &settings)
{
    SettingRow msaa{SettingsMenu::kMsaa, 0, 0, {}, 0, 0};
    for (size_t index = 0; index < std::size(ViewerSettings::kMsaaChoices); ++index)
    {
        const int samples = ViewerSettings::kMsaaChoices[index];
        msaa.opts.push_back(samples == 1 ? strings.frontEnd(6, "Off") : widenLatin1((std::to_string(samples) + "x").c_str()));
        if (samples == settings.msaa())
        {
            msaa.chosen = int(index);
        }
        if (samples == ViewerSettings().msaa())
        {
            msaa.defaultChoice = int(index);
        }
    }
    return msaa;
}

// 64 / 128 / 256 / 512, default 64
SettingRow kemarTapsRow(const ViewerSettings &settings)
{
    SettingRow kemarTaps{SettingsMenu::kKemarTaps, 0, 0, {}, 0, 0};
    for (size_t index = 0; index < std::size(argon::hrtf::kKemarTapChoices); ++index)
    {
        const int taps = argon::hrtf::kKemarTapChoices[index];
        kemarTaps.opts.push_back(widenLatin1(std::to_string(taps).c_str()));
        if (taps == settings.kemarTaps())
        {
            kemarTaps.chosen = int(index);
        }
        if (taps == argon::hrtf::kDefaultKemarTaps)
        {
            kemarTaps.defaultChoice = int(index);
        }
    }
    return kemarTaps;
}

// the viewer's own rows have no fe strings (the game reads the widescreen flag from the dashboard; docs\argon\differences.md)
struct ViewerRow
{
    int type;
    const viewer_text::RowText &text;
};

const ViewerRow kViewerRows[] = {
    {SettingsMenu::kSave, viewer_text::kSave},
    {SettingsMenu::kLanguage, viewer_text::kLanguage},
    {SettingsMenu::kRenderer, viewer_text::kRenderer},
    {SettingsMenu::kWidescreen, viewer_text::kWidescreen},
    {SettingsMenu::kHudSize, viewer_text::kHudSize},
    {SettingsMenu::kResolution, viewer_text::kResolution},
    {SettingsMenu::kFullscreen, viewer_text::kFullscreen},
    {SettingsMenu::kVsync, viewer_text::kVsync},
    {SettingsMenu::kMsaa, viewer_text::kMsaa},
    {SettingsMenu::kPostAa, viewer_text::kPostAa},
    {SettingsMenu::kTranslucency, viewer_text::kTranslucency},
    {SettingsMenu::kSpeedBlur, viewer_text::kSpeedBlur},
    {SettingsMenu::kMarsBlurFix, viewer_text::kMarsBlurFix},
    {SettingsMenu::kFmv, viewer_text::kFmvs},
    {SettingsMenu::kFpsMeter, viewer_text::kFpsMeter},
    {SettingsMenu::kSpeakers, viewer_text::kSpeakers},
    {SettingsMenu::kHrtf, viewer_text::kHrtf},
    {SettingsMenu::kKemarTaps, viewer_text::kKemarTaps},
};

std::optional<std::reference_wrapper<const viewer_text::RowText>> viewerRowText(int type)
{
    for (const ViewerRow &row : kViewerRows)
    {
        if (row.type == type)
        {
            return row.text;
        }
    }
    return std::nullopt;
}

// 0x1a8564: DirectSound millibels per volume step 0..7
float stepMillibels(int step)
{
    static const float kMillibels[8] = {-10000.f, -3200.f, -1600.f, -800.f, -400.f, -200.f, -100.f, 0.f};
    return kMillibels[std::clamp(step, 0, 7)];
}
}  // namespace

bool SettingsMenu::hasProfileLists() const
{
    return !rows(SettingsList::Game).empty();
}

void SettingsMenu::buildProfileLists(const GameStrings &strings)
{
    clearProfileLists();
    std::vector<SettingRow> &game = list(SettingsList::Game);
    std::vector<SettingRow> &sound = list(SettingsList::Sound);
    addListHead(game);
    game.push_back(onOffRow(strings, SettingIds{5, 0x324, 0x33f}));
    game.push_back(onOffRow(strings, SettingIds{4, 0x323, 0x33e}));
    game.push_back(onOffRow(strings, SettingIds{3, 0x322, 0x33d}));
    game.push_back(onOffRow(strings, SettingIds{kVibrationFirst, 0x321, 0x33c}));
    game.push_back({kSpeedUnits, 0x329, 0x294, {strings.frontEnd(0x334, "km/h"), strings.frontEnd(0x335, "mph")}, 0, 0});   // region 1: mph
    game.push_back(stepsRow(strings, SettingIds{kBrightness, 0x32b, 0x297}));
    game.push_back({kHud, 0x336, 0x298, {strings.frontEnd(6, "Off"), strings.frontEnd(0x339, "Map Off"), strings.frontEnd(0x337, "Full")}, 2, 2});
    game.push_back({kLaps,
                    0x340,
                    0x296,
                    {strings.frontEnd(0x33a, "Default"),
                     strings.general(0x4b3, "3"),
                     strings.general(0x4b5, "5"),
                     strings.general(0x4b7, "7"),
                     strings.general(0x4b9, "9")},
                    0,
                    0});
    addListHead(sound);
    sound.push_back({9, 0x32c, 0x29a, {}, 0, 0});   // Music Options: A opens FE MUSICSELECT (0x2ebd0)
    sound.push_back(stepsRow(strings, SettingIds{kEngineVolume, 0x32d, 0x29f}));
    sound.push_back(stepsRow(strings, SettingIds{kSfxVolume, 0x32e, 0x29e}));
    sound.push_back(stepsRow(strings, SettingIds{kMusicVolume, 0x32f, 0x29d}));
    sound.push_back({0xe, 0x330, 0x29c, {strings.frontEnd(2, "No"), strings.frontEnd(3, "Yes")}, 0, 0});
}

void SettingsMenu::clearProfileLists()
{
    list(SettingsList::Game).clear();
    list(SettingsList::Sound).clear();
}

// each of Argon Options' pages: Save (in place of Exit: the rows apply as they change, Save writes them, B puts them
// back) and Reset to Default first, its rows after (drawn bottom up, as the game's lists)
void SettingsMenu::buildViewerList(const GameStrings &strings, const ViewerChoices &choices)
{
    const ViewerSettings &settings = choices.settings;
    const ViewerSettings defaults;
    for (const SettingsList which : {SettingsList::ArgonGeneral, SettingsList::ArgonGraphics, SettingsList::ArgonAudio, SettingsList::ArgonFixes})
    {
        list(which) = {{kSave, 0, 0, {}, 0, 0}, {1, 0x332, 0x333, {}, 0, 0}};
    }
    std::vector<SettingRow> &general = list(SettingsList::ArgonGeneral);
    general.push_back(languageRow(strings, choices));
    general.push_back(viewerOnOffRow(strings, kFmv, !settings.skipIntroVideos(), true));
    general.push_back(viewerOnOffRow(strings, kFpsMeter, settings.fpsMeter(), false));

    std::vector<SettingRow> &graphics = list(SettingsList::ArgonGraphics);
    graphics.push_back({kRenderer, 0, 0, {u"Vulkan", u"Direct3D 12"}, int(settings.renderer()), int(defaults.renderer())});
    graphics.push_back(viewerOnOffRow(strings, kWidescreen, settings.widescreen(), true));
    graphics.push_back(hudSizeRow(settings));
    graphics.push_back(resolutionRow(choices));
    graphics.push_back(viewerOnOffRow(strings, kFullscreen, settings.fullscreen(), true));
    graphics.push_back(viewerOnOffRow(strings, kVsync, settings.vsync(), true));
    graphics.push_back(msaaRow(strings, settings));
    graphics.push_back({kPostAa, 0, 0, {strings.frontEnd(6, "Off"), u"FXAA", u"SMAA"}, int(settings.postAntiAliasing()), int(defaults.postAntiAliasing())});

    std::vector<SettingRow> &audio = list(SettingsList::ArgonAudio);
    audio.push_back({kSpeakers, 0, 0, {strings.viewer(viewer_text::kStereo), u"5.1"}, settings.surroundSound() ? 1 : 0, 0});
    audio.push_back(kemarTapsRow(settings));
    audio.push_back({kHrtf, 0, 0, {u"Xbox", u"MIT KEMAR"}, settings.hrtf() == argon::hrtf::Set::Kemar ? 1 : 0, 0});

    std::vector<SettingRow> &fixes = list(SettingsList::ArgonFixes);
    fixes.push_back({kTranslucency,
                     0,
                     0,
                     {strings.viewer(viewer_text::kGameOrder), strings.viewer(viewer_text::kOrderIndependent)},
                     settings.orderIndependentTranslucency() ? 1 : 0,
                     1});
    fixes.push_back(viewerOnOffRow(strings, kMarsBlurFix, settings.marsSpeedBlurFix(), true));
    fixes.push_back(viewerOnOffRow(strings, kSpeedBlur, settings.speedBlur(), true));
}

std::vector<SettingRow> &SettingsMenu::list(SettingsList which)
{
    return lists_[size_t(which)];
}

std::vector<SettingRow> &SettingsMenu::rows(SettingsList which)
{
    return list(which);
}

const std::vector<SettingRow> &SettingsMenu::rows(SettingsList which) const
{
    return lists_[size_t(which)];
}

std::optional<std::reference_wrapper<const SettingRow>> SettingsMenu::findRow(int type) const
{
    for (const std::vector<SettingRow> &rows : lists_)
    {
        for (const SettingRow &row : rows)
        {
            if (row.type == type)
            {
                return row;
            }
        }
    }
    return std::nullopt;
}

int SettingsMenu::current(int type) const
{
    const auto row = findRow(type);
    return row ? row->get().chosen : 0;
}

int SettingsMenu::optionCount(int type) const
{
    const auto row = findRow(type);
    return row ? int(row->get().opts.size()) : 0;
}

std::u16string SettingsMenu::optionText(int type) const
{
    const auto row = findRow(type);
    if (!row || row->get().chosen < 0 || row->get().chosen >= int(row->get().opts.size()))
    {
        return {};
    }
    return row->get().opts[size_t(row->get().chosen)];
}

void SettingsMenu::select(int type, int option)
{
    for (std::vector<SettingRow> &rows : lists_)
    {
        for (SettingRow &row : rows)
        {
            if (row.type == type)
            {
                row.chosen = option;
            }
        }
    }
}

void SettingsMenu::resetToDefaults(std::vector<SettingRow> &rows)
{
    for (SettingRow &row : rows)
    {
        row.chosen = row.defaultChoice;
    }
}

bool SettingsMenu::isViewerSetting(int type) const
{
    for (size_t which = 0; which < lists_.size(); ++which)
    {
        if (!isArgonList(SettingsList(which)))
        {
            continue;
        }
        for (const SettingRow &row : lists_[which])
        {
            if (row.type == type)
            {
                return true;
            }
        }
    }
    return false;
}

bool SettingsMenu::isChangeable(int type) const
{
    constexpr int kXboxHrtf = 0;
    return type != kKemarTaps || current(kHrtf) != kXboxHrtf;
}

float SettingsMenu::volumeMillibels(int type) const
{
    constexpr int kDefaultStep = 4;
    return stepMillibels(hasProfileLists() ? current(type) : kDefaultStep);
}

std::u16string SettingsMenu::label(const SettingRow &row, const GameStrings &strings)
{
    const auto own = viewerRowText(row.type);
    return own ? strings.viewer(own->get().label) : strings.frontEnd(row.label, "");
}

std::u16string SettingsMenu::description(const SettingRow &row, const GameStrings &strings)
{
    const auto own = viewerRowText(row.type);
    return own ? strings.viewer(own->get().description) : strings.frontEnd(row.desc, "");
}

std::string SettingsMenu::profileString() const
{
    std::string text;
    for (const SettingsList which : {SettingsList::Game, SettingsList::Sound})
    {
        for (const SettingRow &row : rows(which))
        {
            if (!row.opts.empty())
            {
                text += std::to_string(row.type) + "=" + std::to_string(row.chosen) + ";";
            }
        }
    }
    return text;
}

void SettingsMenu::readProfileString(const std::string &text)
{
    resetToDefaults(list(SettingsList::Game));
    resetToDefaults(list(SettingsList::Sound));
    size_t at = 0;
    while (at < text.size())
    {
        const size_t equals = text.find('=', at), end = text.find(';', at);
        if (equals == std::string::npos || end == std::string::npos || equals > end)
        {
            break;
        }
        const int type = std::atoi(text.substr(at, equals - at).c_str());
        const int option = std::atoi(text.substr(equals + 1, end - equals - 1).c_str());
        for (const SettingsList which : {SettingsList::Game, SettingsList::Sound})
        {
            for (SettingRow &row : list(which))
            {
                if (row.type == type && option >= 0 && option < int(row.opts.size()))
                {
                    row.chosen = option;
                }
            }
        }
        at = end + 1;
    }
}

ViewerSettings SettingsMenu::chosenViewerSettings(const ViewerChoices &choices) const
{
    ViewerSettings chosen = choices.settings;
    chosen.setLanguage(choices.languages[size_t(current(kLanguage))]);
    chosen.setRenderer(argon::gfx::BackendKind(current(kRenderer)));
    chosen.setWidescreen(current(kWidescreen) == 1);
    chosen.setHudSizeStep(current(kHudSize));
    chosen.setResolution(choices.resolutions[size_t(current(kResolution))]);
    chosen.setFullscreen(current(kFullscreen) == 1);
    chosen.setVsync(current(kVsync) == 1);
    chosen.setFpsMeter(current(kFpsMeter) == 1);
    chosen.setSkipIntroVideos(current(kFmv) == 0);
    chosen.setSurroundSound(current(kSpeakers) == 1);
    chosen.setHrtf(current(kHrtf) == 1 ? argon::hrtf::Set::Kemar : argon::hrtf::Set::Xbox);
    chosen.setKemarTaps(argon::hrtf::kKemarTapChoices[size_t(current(kKemarTaps))]);
    chosen.setMsaa(ViewerSettings::kMsaaChoices[size_t(current(kMsaa))]);
    chosen.setPostAntiAliasing(PostAntiAliasing(current(kPostAa)));
    chosen.setOrderIndependentTranslucency(current(kTranslucency) == 1);
    chosen.setSpeedBlur(current(kSpeedBlur) == 1);
    chosen.setMarsSpeedBlurFix(current(kMarsBlurFix) == 1);
    return chosen;
}
