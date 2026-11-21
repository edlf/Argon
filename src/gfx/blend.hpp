// Argon: a colour blend's factors and operation, the same for every backend (the game's D3D blend states; each backend
// maps them: gfx\vulkan\vulkan_formats.hpp, gfx\d3d12\d3d12_device.cpp). Small values: MeshState::pipelineKey packs
// them in five bits each.
#pragma once
#include <cstdint>

namespace argon::gfx
{

enum class BlendFactor : uint8_t
{
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    DstColor,
    OneMinusDstColor
};

enum class BlendOp : uint8_t
{
    Add,
    Subtract,          // source - destination
    ReverseSubtract,   // destination - source
    Min,
    Max
};

}  // namespace argon::gfx
