// Argon: an ImGui image on Direct3D 12 (ImTextureID): its view's index in the shader-visible heap and the SamplerMode
// it is drawn with (D3D12UiRenderer binds both).
#pragma once
#include "gfx/gfx_types.hpp"

#include <imgui.h>

namespace argon::gfx::d3d12
{

inline ImTextureID uiImageOf(uint32_t viewIndex, SamplerMode sampling)
{
    return ImTextureID(uint64_t(viewIndex) << 2 | uint64_t(sampling));
}
inline uint32_t viewIndexOf(ImTextureID image)
{
    return uint32_t(uint64_t(image) >> 2);
}
inline SamplerMode samplingOf(ImTextureID image)
{
    return SamplerMode(uint64_t(image) & 3);
}

}  // namespace argon::gfx::d3d12
