// Argon: choosing the renderer's backend (gfx\backend.hpp).
#include "gfx/backend.hpp"
#include "gfx/vulkan/vulkan_backend.hpp"
#ifdef _WIN32
#include "gfx/d3d12/d3d12_backend.hpp"
#endif

#include <stdexcept>

namespace argon::gfx
{

std::unique_ptr<Backend> createBackend(const BackendOptions &options, GLFWwindow *window, uint32_t pushConstantBytes)
{
    if (options.kind == BackendKind::Direct3D12)
    {
#ifdef _WIN32
        return std::make_unique<d3d12::D3D12Backend>(options, window, pushConstantBytes);
#else
        throw std::runtime_error("the Direct3D 12 renderer is Windows only (use --renderer vulkan)");
#endif
    }
    return std::make_unique<vulkan::VulkanBackend>(options, window, pushConstantBytes);
}

}  // namespace argon::gfx
