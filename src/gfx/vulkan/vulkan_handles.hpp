// Argon: the Vulkan backend's objects behind the gfx handles (gfx\gfx_types.hpp): a handle holds the Vulkan handle's
// 64 bits.
#pragma once
#include "gfx/gfx_types.hpp"

#include <vulkan/vulkan.h>

namespace argon::gfx::vulkan
{

template <class Handle, class VulkanHandle> Handle wrap(VulkanHandle handle)
{
    return Handle{uint64_t(handle)};
}

template <class VulkanHandle, class Handle> VulkanHandle unwrap(Handle handle)
{
    return VulkanHandle(handle.value);
}

inline Pipeline pipelineOf(VkPipeline pipeline)
{
    return wrap<Pipeline>(pipeline);
}
inline TextureSet textureSetOf(VkDescriptorSet set)
{
    return wrap<TextureSet>(set);
}
inline MaterialSet materialSetOf(VkDescriptorSet set)
{
    return wrap<MaterialSet>(set);
}
inline SceneSet sceneSetOf(VkDescriptorSet set)
{
    return wrap<SceneSet>(set);
}
inline BufferRef bufferOf(VkBuffer buffer)
{
    return wrap<BufferRef>(buffer);
}
inline QueryPool queryPoolOf(VkQueryPool pool)
{
    return wrap<QueryPool>(pool);
}

inline Rect rectOf(const VkRect2D &area)
{
    return Rect{area.offset.x, area.offset.y, area.extent.width, area.extent.height};
}
inline VkRect2D toVulkan(const Rect &area)
{
    return VkRect2D{{area.x, area.y}, {area.width, area.height}};
}

}  // namespace argon::gfx::vulkan
