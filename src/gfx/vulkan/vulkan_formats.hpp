// Argon: the backend-neutral blend, depth and texel enums (gfx\blend.hpp, gfx\texture_format.hpp,
// gfx\pipeline_spec.hpp) as Vulkan's.
#pragma once
#include "gfx/blend.hpp"
#include "gfx/pipeline_spec.hpp"
#include "gfx/texture_format.hpp"

#include <stdexcept>
#include <string>
#include <vulkan/vulkan.h>

namespace argon::gfx::vulkan
{

inline VkBlendFactor blendFactorOf(BlendFactor factor)
{
    switch (factor)
    {
    case BlendFactor::Zero:
        return VK_BLEND_FACTOR_ZERO;
    case BlendFactor::One:
        return VK_BLEND_FACTOR_ONE;
    case BlendFactor::SrcColor:
        return VK_BLEND_FACTOR_SRC_COLOR;
    case BlendFactor::OneMinusSrcColor:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case BlendFactor::SrcAlpha:
        return VK_BLEND_FACTOR_SRC_ALPHA;
    case BlendFactor::OneMinusSrcAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case BlendFactor::DstAlpha:
        return VK_BLEND_FACTOR_DST_ALPHA;
    case BlendFactor::OneMinusDstAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case BlendFactor::DstColor:
        return VK_BLEND_FACTOR_DST_COLOR;
    case BlendFactor::OneMinusDstColor:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    }
    return VK_BLEND_FACTOR_ONE;
}

inline VkBlendOp blendOpOf(BlendOp op)
{
    switch (op)
    {
    case BlendOp::Add:
        return VK_BLEND_OP_ADD;
    case BlendOp::Subtract:
        return VK_BLEND_OP_SUBTRACT;
    case BlendOp::ReverseSubtract:
        return VK_BLEND_OP_REVERSE_SUBTRACT;
    case BlendOp::Min:
        return VK_BLEND_OP_MIN;
    case BlendOp::Max:
        return VK_BLEND_OP_MAX;
    }
    return VK_BLEND_OP_ADD;
}

// the scene's depth is reversed (render\projection.hpp): nearer is greater
inline VkCompareOp compareOf(DepthCompare depth)
{
    return depth == DepthCompare::Always ? VK_COMPARE_OP_ALWAYS : VK_COMPARE_OP_GREATER_OR_EQUAL;
}

inline VkFormat formatOf(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::Bc1:
        return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case TextureFormat::Bc2:
        return VK_FORMAT_BC2_UNORM_BLOCK;
    case TextureFormat::Bc3:
        return VK_FORMAT_BC3_UNORM_BLOCK;
    case TextureFormat::Rgba8:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case TextureFormat::Bgra8:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case TextureFormat::Undefined:
        break;
    }
    throw std::invalid_argument("Vulkan: a texture without a format (" + std::to_string(int(format)) + ")");
}

}  // namespace argon::gfx::vulkan
