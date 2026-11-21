// Argon: the renderer's backends by name (--renderer, settings.json's "renderer"; gfx\backend.hpp makes them).
#pragma once
#include <optional>
#include <string_view>

namespace argon::gfx
{

enum class BackendKind
{
    Vulkan,
    Direct3D12
};

// "vulkan" or "d3d12"; none for anything else
inline std::optional<BackendKind> backendNamed(std::string_view name)
{
    if (name == "vulkan")
    {
        return BackendKind::Vulkan;
    }
    if (name == "d3d12")
    {
        return BackendKind::Direct3D12;
    }
    return std::nullopt;
}

inline const char *nameOf(BackendKind kind)
{
    return kind == BackendKind::Direct3D12 ? "d3d12" : "vulkan";
}

}  // namespace argon::gfx
