// Argon: the post-process anti-aliasing Argon Options' Post AA picks (settings.json "postAntiAliasing", --post-aa);
// drawn by render\post_anti_aliasing.hpp. MSAA is a separate setting, so either can go with it.
#pragma once
#include <array>
#include <optional>
#include <string_view>

enum class PostAntiAliasing
{
    Off,
    Fxaa,   // FXAA: one pass, the softest
    Smaa    // SMAA 1x: three passes, sharper edges
};

struct PostAntiAliasingName
{
    PostAntiAliasing mode;
    const char *name;
};

inline constexpr std::array<PostAntiAliasingName, 3> kPostAntiAliasingNames{
    {{PostAntiAliasing::Off, "off"}, {PostAntiAliasing::Fxaa, "fxaa"}, {PostAntiAliasing::Smaa, "smaa"}}};

inline const char *nameOf(PostAntiAliasing mode)
{
    for (const PostAntiAliasingName &known : kPostAntiAliasingNames)
    {
        if (known.mode == mode)
        {
            return known.name;
        }
    }
    return "off";
}

// none for a name that is not off, fxaa or smaa
inline std::optional<PostAntiAliasing> postAntiAliasingNamed(std::string_view name)
{
    for (const PostAntiAliasingName &known : kPostAntiAliasingNames)
    {
        if (name == known.name)
        {
            return known.mode;
        }
    }
    return std::nullopt;
}
