// The viewer's own settings, <saves>\settings.json (written with the defaults when missing):
//   {"language": "English", "skipIntroVideos": false, "renderer": "vulkan", "hudSize": 100, "widescreen": true,
//    "fullscreen": true, "resolution": "2560x1440", "vsync": true, "fpsMeter": false, "surroundSound": false,
//    "hrtf": "xbox", "kemarTaps": 64, "msaa": 8, "postAntiAliasing": "smaa", "orderIndependentTranslucency": true,
//    "speedBlur": true, "marsSpeedBlurFix": true, "keyBindings": {"throttle": ["W", "Up"], ..., "changeView": ["C", "-"]}}
// language: English, French, German, Italian or Japanese (the game's: string tables 0x79030, intro 0x12770; anything
// else, Spanish included, is English). It picks Locale\<language>\ for the text and the intro video. On the USA disc
// only English and Japanese have real text: French, German and Italian hold placeholder strings ("xSélé¢t Profìlêöäß...",
// argon::isPlaceholderTranslation) and Argon Options offers them only with the EU release's tables. renderer: "vulkan" or "d3d12" (--renderer overrides it). hudSize: the race HUD's size in percent, 25,
// 50, 75 or 100 (the pause menu's HUD Size; docs\argon\differences.md). widescreen: the game's widescreen flag (Argon Options' Widescreen),
// on: the menus' and the HUD's margins follow a wide window; off: they keep the 4:3 layout in its centred 640 frame.
// fullscreen: Argon Options' Fullscreen (and F11), the monitor switched to the resolution. resolution: Argon
// Settings' Resolution, the monitor's mode in fullscreen and the window's size out of it ("<width>x<height>"; missing, or one the monitor does not offer,
// becomes the monitor's largest at start-up, window\resolution.hpp). vsync: Argon Options' VSync, frames shown on the monitor's refresh
// (off: as fast as they are drawn). fpsMeter: Argon Options' FPS Meter (and F2). surroundSound: Argon Options' Speakers,
// 5.1 output (on) or stereo, taken at the next start. hrtf: Argon Options' HRTF, "xbox" (the game's Sensaura filters)
// or "kemar" (MIT KEMAR, <project>\hrtf_kemar.bin). kemarTaps: Argon Options' KEMAR Taps, MIT KEMAR's filter length, 64,
// 128, 256 or 512 (hrtf_set.hpp; another value: the one below). msaa: Argon Options' MSAA, the scene's samples per pixel (1 = off, 2, 4
// or 8; --msaa overrides it), taken at the next start and capped to what the GPU takes. postAntiAliasing: Argon
// Settings' Post AA over the 3D view, "off", "fxaa" or "smaa" (render\post_anti_aliasing.hpp). orderIndependentTranslucency:
// Argon Options' Translucency, the alpha-blended translucent meshes blended without the game's draw order (true) or in
// it (render\order_independent_translucency.hpp; --oit overrides it). speedBlur: Argon Options' Speed Blur, the race's
// speed blur (above 720 km/h the last frame drawn back zoomed) or none (false). marsSpeedBlurFix: Argon Options' Mars
// Flicker Fix, on Mars and Terraformed Mars the blur's fade without its zoom, from a steady speed
// (app\app_effects.cpp), or the game's blur there too (false). keyBindings: Argon Options' Key Bindings, each craft
// action's two keys by name ("-": none; input\key_bindings.hpp); an action left out keeps its default keys. These are
// not the game's Options, which belong to the profile.
#pragma once
#include "audio/hrtf_set.hpp"
#include "gfx/backend_kind.hpp"
#include "input/key_bindings.hpp"
#include "render/post_anti_aliasing_mode.hpp"
#include "window/resolution.hpp"

#include <filesystem>
#include <string>

enum class Language
{
    English,
    French,
    German,
    Italian,
    Japanese
};
inline constexpr int kLanguageCount = 5;
const char *nameOf(Language language);   // "English" .. "Japanese", settings.json's name

class ViewerSettings
{
  public:
    static ViewerSettings load(const std::filesystem::path &file);   // the defaults (and the file written) when missing
    void save(const std::filesystem::path &file) const;
    bool operator==(const ViewerSettings &other) const = default;

    Language language() const
    {
        return language_;
    }
    void setLanguage(Language language)
    {
        language_ = language;
    }
    bool skipIntroVideos() const   // Argon Options' FMVs Off: start on the title screen
    {
        return skipIntroVideos_;
    }
    void setSkipIntroVideos(bool skip)
    {
        skipIntroVideos_ = skip;
    }
    std::string localeFolder() const;   // "Locale/<language>"
    argon::gfx::BackendKind renderer() const   // taken at the next start
    {
        return renderer_;
    }
    void setRenderer(argon::gfx::BackendKind kind)
    {
        renderer_ = kind;
    }

    static constexpr int kHudSizeSteps = 4;   // 25, 50, 75, 100 %
    int hudSizeStep() const                   // 0..3
    {
        return hudSizeStep_;
    }
    void setHudSizeStep(int step);
    float hudScale() const   // 0.25 .. 1
    {
        return float(hudSizeStep_ + 1) / float(kHudSizeSteps);
    }
    int hudSizePercent() const
    {
        return 100 * (hudSizeStep_ + 1) / kHudSizeSteps;
    }

    bool widescreen() const   // g_widescreen 0x298ecc, the dashboard's flag; off: the 4:3 layout in a wide window
    {
        return widescreen_;
    }
    void setWidescreen(bool on)
    {
        widescreen_ = on;
    }

    std::optional<argon::Resolution> resolution() const   // none: the monitor's largest
    {
        return resolution_;
    }
    void setResolution(const argon::Resolution &size)
    {
        resolution_ = size;
    }

    bool fullscreen() const   // the monitor in the resolution's mode (F11)
    {
        return fullscreen_;
    }
    void setFullscreen(bool on)
    {
        fullscreen_ = on;
    }

    bool fpsMeter() const   // the FPS meter, top right (F2)
    {
        return fpsMeter_;
    }
    void setFpsMeter(bool on)
    {
        fpsMeter_ = on;
    }

    bool vsync() const
    {
        return vsync_;
    }
    void setVsync(bool on)
    {
        vsync_ = on;
    }

    bool surroundSound() const   // 5.1 output (the dashboard's Dolby Digital); off: stereo. Read when the audio opens
    {
        return surroundSound_;
    }
    void setSurroundSound(bool on)
    {
        surroundSound_ = on;
    }

    argon::hrtf::Set hrtf() const   // the 3D voices' HRTF
    {
        return hrtf_;
    }
    void setHrtf(argon::hrtf::Set set)
    {
        hrtf_ = set;
    }

    int kemarTaps() const   // MIT KEMAR's ears cut to this many taps
    {
        return kemarTaps_;
    }
    void setKemarTaps(int taps);   // one of argon::hrtf::kKemarTapChoices, else the nearest below

    static constexpr int kMsaaChoices[] = {1, 2, 4, 8};   // samples per pixel, 1: off
    int msaa() const
    {
        return msaa_;
    }
    void setMsaa(int samples);   // one of kMsaaChoices, else the nearest below

    PostAntiAliasing postAntiAliasing() const
    {
        return postAntiAliasing_;
    }
    void setPostAntiAliasing(PostAntiAliasing mode)
    {
        postAntiAliasing_ = mode;
    }

    bool orderIndependentTranslucency() const
    {
        return orderIndependentTranslucency_;
    }
    void setOrderIndependentTranslucency(bool on)
    {
        orderIndependentTranslucency_ = on;
    }

    bool speedBlur() const
    {
        return speedBlur_;
    }
    void setSpeedBlur(bool on)
    {
        speedBlur_ = on;
    }

    bool marsSpeedBlurFix() const
    {
        return marsSpeedBlurFix_;
    }
    void setMarsSpeedBlurFix(bool on)
    {
        marsSpeedBlurFix_ = on;
    }

    const KeyBindings &keyBindings() const
    {
        return keyBindings_;
    }
    void setKeyBindings(const KeyBindings &bindings)
    {
        keyBindings_ = bindings;
    }

  private:
    Language language_ = Language::English;
    bool skipIntroVideos_ = false;
    argon::gfx::BackendKind renderer_ = argon::gfx::BackendKind::Vulkan;
    int hudSizeStep_ = kHudSizeSteps - 1;
    bool widescreen_ = true;
    std::optional<argon::Resolution> resolution_;
    bool fullscreen_ = true;
    bool vsync_ = true;
    bool fpsMeter_ = false;
    bool surroundSound_ = false;
    argon::hrtf::Set hrtf_ = argon::hrtf::Set::Xbox;
    int kemarTaps_ = argon::hrtf::kDefaultKemarTaps;
    int msaa_ = 8;
    PostAntiAliasing postAntiAliasing_ = PostAntiAliasing::Smaa;
    bool orderIndependentTranslucency_ = true;
    bool speedBlur_ = true;
    bool marsSpeedBlurFix_ = true;
    KeyBindings keyBindings_;
};
