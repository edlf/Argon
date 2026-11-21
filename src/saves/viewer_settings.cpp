#include "saves/viewer_settings.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using nlohmann::json;

namespace
{

struct LanguageName
{
    Language language;
    const char *name;
};

constexpr std::array<LanguageName, kLanguageCount> kLanguages{{{Language::English, "English"},
                                                  {Language::French, "French"},
                                                  {Language::German, "German"},
                                                  {Language::Italian, "Italian"},
                                                  {Language::Japanese, "Japanese"}}};

Language languageNamed(const std::string &name, const fs::path &file)
{
    for (const auto &language : kLanguages)
    {
        if (name == language.name)
        {
            return language.language;
        }
    }
    std::fprintf(
        stderr, "settings: %s: unknown language \"%s\" (English, French, German, Italian, Japanese): English\n", file.string().c_str(), name.c_str());
    return Language::English;
}

argon::gfx::BackendKind rendererNamed(const std::string &name, const fs::path &file)
{
    if (const std::optional<argon::gfx::BackendKind> kind = argon::gfx::backendNamed(name))
    {
        return *kind;
    }
    std::fprintf(stderr, "settings: %s: unknown renderer \"%s\" (vulkan, d3d12): vulkan\n", file.string().c_str(), name.c_str());
    return argon::gfx::BackendKind::Vulkan;
}

const char *hrtfNameOf(argon::hrtf::Set set)
{
    return set == argon::hrtf::Set::Kemar ? "kemar" : "xbox";
}

argon::hrtf::Set hrtfNamed(const std::string &name, const fs::path &file)
{
    if (name == "kemar")
    {
        return argon::hrtf::Set::Kemar;
    }
    if (name != "xbox")
    {
        std::fprintf(stderr, "settings: %s: unknown hrtf \"%s\" (xbox, kemar): xbox\n", file.string().c_str(), name.c_str());
    }
    return argon::hrtf::Set::Xbox;
}

PostAntiAliasing postAntiAliasingIn(const std::string &name, const fs::path &file)
{
    if (const std::optional<PostAntiAliasing> mode = postAntiAliasingNamed(name))
    {
        return *mode;
    }
    std::fprintf(stderr, "settings: %s: unknown postAntiAliasing \"%s\" (off, fxaa, smaa): smaa\n", file.string().c_str(), name.c_str());
    return PostAntiAliasing::Smaa;
}

int hudSizeStepOf(int percent, const fs::path &file)
{
    if (percent % 25 == 0 && percent >= 25 && percent <= 100)
    {
        return percent / 25 - 1;
    }
    std::fprintf(stderr, "settings: %s: hudSize %d is not 25, 50, 75 or 100: 100\n", file.string().c_str(), percent);
    return ViewerSettings::kHudSizeSteps - 1;
}

// none (the monitor's largest) when missing or unreadable
std::optional<argon::Resolution> resolutionIn(const json &root, const fs::path &file)
{
    if (!root.contains("resolution"))
    {
        return std::nullopt;
    }
    const std::string text = root.value("resolution", std::string());
    const std::optional<argon::Resolution> size = argon::parseResolution(text);
    if (!size)
    {
        std::fprintf(stderr, "settings: %s: resolution \"%s\" is not <width>x<height>: the largest\n", file.string().c_str(), text.c_str());
    }
    return size;
}

// the defaults with the keys settings.json names for its actions; an unknown name or a reserved key is no key
KeyBindings keyBindingsIn(const json &root, const fs::path &file)
{
    KeyBindings bindings;
    if (!root.contains("keyBindings") || !root["keyBindings"].is_object())
    {
        return bindings;
    }
    for (const auto &[actionName, keys] : root["keyBindings"].items())
    {
        const std::optional<KeyAction> action = keyActionNamed(actionName);
        if (!action || !keys.is_array())
        {
            std::fprintf(stderr, "settings: %s: keyBindings \"%s\" is no action with a list of keys\n", file.string().c_str(), actionName.c_str());
            continue;
        }
        for (int slot = 0; slot < kKeySlots; ++slot)
        {
            const size_t index = size_t(slot);
            const int key = index < keys.size() && keys[index].is_string() ? keyNamed(keys[index].get<std::string>()) : kUnboundKey;
            if (key == kUnboundKey || isReservedKey(key))
            {
                bindings.clear(*action, slot);
                continue;
            }
            bindings.bind(*action, slot, key);
        }
    }
    return bindings;
}

json keyBindingsJson(const KeyBindings &bindings)
{
    json actions = json::object();
    for (int action = 0; action < kKeyActionCount; ++action)
    {
        json keys = json::array();
        for (int slot = 0; slot < kKeySlots; ++slot)
        {
            keys.push_back(keyName(bindings.key(KeyAction(action), slot)));
        }
        actions[nameOf(KeyAction(action))] = keys;
    }
    return actions;
}

} // namespace

const char *nameOf(Language language)
{
    for (const auto &entry : kLanguages)
    {
        if (entry.language == language)
        {
            return entry.name;
        }
    }
    return "English";
}

void ViewerSettings::setHudSizeStep(int step)
{
    hudSizeStep_ = std::clamp(step, 0, kHudSizeSteps - 1);
}

namespace
{
template <size_t kCount>
int choiceAtOrBelow(const int (&choices)[kCount], int value)
{
    int chosen = choices[0];
    for (const int choice : choices)
    {
        if (choice <= value)
        {
            chosen = choice;
        }
    }
    return chosen;
}
}  // namespace

void ViewerSettings::setKemarTaps(int taps)
{
    kemarTaps_ = choiceAtOrBelow(argon::hrtf::kKemarTapChoices, taps);
}

void ViewerSettings::setMsaa(int samples)
{
    msaa_ = choiceAtOrBelow(kMsaaChoices, samples);
}

ViewerSettings ViewerSettings::load(const fs::path &file)
{
    ViewerSettings settings;
    std::ifstream f(file);
    if (!f)
    {
        settings.save(file);
        return settings;
    }
    try
    {
        const json root = json::parse(f);
        settings.language_ = languageNamed(root.value("language", std::string("English")), file);
        settings.skipIntroVideos_ = root.value("skipIntroVideos", false);
        settings.renderer_ = rendererNamed(root.value("renderer", std::string("vulkan")), file);
        settings.hudSizeStep_ = hudSizeStepOf(root.value("hudSize", 100), file);
        settings.widescreen_ = root.value("widescreen", true);
        settings.resolution_ = resolutionIn(root, file);
        settings.fullscreen_ = root.value("fullscreen", true);
        settings.vsync_ = root.value("vsync", true);
        settings.fpsMeter_ = root.value("fpsMeter", false);
        settings.surroundSound_ = root.value("surroundSound", false);
        settings.hrtf_ = hrtfNamed(root.value("hrtf", std::string("xbox")), file);
        settings.setKemarTaps(root.value("kemarTaps", argon::hrtf::kDefaultKemarTaps));
        settings.setMsaa(root.value("msaa", ViewerSettings().msaa_));
        settings.postAntiAliasing_ = postAntiAliasingIn(root.value("postAntiAliasing", std::string(nameOf(PostAntiAliasing::Smaa))), file);
        settings.orderIndependentTranslucency_ = root.value("orderIndependentTranslucency", true);
        settings.speedBlur_ = root.value("speedBlur", true);
        settings.marsSpeedBlurFix_ = root.value("marsSpeedBlurFix", true);
        settings.keyBindings_ = keyBindingsIn(root, file);
    }
    catch (const json::exception &e)
    {
        std::fprintf(stderr, "settings: %s: %s (using the defaults)\n", file.string().c_str(), e.what());
    }
    return settings;
}

void ViewerSettings::save(const fs::path &file) const
{
    std::error_code ec;
    fs::create_directories(file.parent_path(), ec);
    json root{{"language", nameOf(language_)}, {"skipIntroVideos", skipIntroVideos_}, {"renderer", argon::gfx::nameOf(renderer_)},
              {"hudSize", hudSizePercent()}, {"widescreen", widescreen_}, {"fullscreen", fullscreen_}, {"vsync", vsync_},
              {"fpsMeter", fpsMeter_}, {"surroundSound", surroundSound_}, {"hrtf", hrtfNameOf(hrtf_)}, {"kemarTaps", kemarTaps_},
              {"msaa", msaa_},
              {"postAntiAliasing", nameOf(postAntiAliasing_)}, {"orderIndependentTranslucency", orderIndependentTranslucency_},
              {"speedBlur", speedBlur_}, {"marsSpeedBlurFix", marsSpeedBlurFix_}, {"keyBindings", keyBindingsJson(keyBindings_)}};
    if (resolution_)
    {
        root["resolution"] = resolution_->text();
    }
    std::ofstream(file, std::ios::trunc) << root.dump(2) << '\n';
}

std::string ViewerSettings::localeFolder() const
{
    return std::string("Locale/") + nameOf(language_);
}
