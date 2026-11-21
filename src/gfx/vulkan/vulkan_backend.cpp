// Argon: gfx::Backend over Vulkan (gfx\vulkan\vulkan_backend.hpp).
#include "gfx/vulkan/vulkan_backend.hpp"

#include <imgui_impl_glfw.h>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <delayimp.h>
#endif

#include <stdexcept>

namespace argon::gfx::vulkan
{

namespace
{

// MSVC builds delay-load vulkan-1.dll (CMakeLists.txt): a missing loader is an exception here, not a crash at the first
// Vulkan call
void requireVulkanLoader()
{
#ifdef _MSC_VER
    if (FAILED(__HrLoadAllImportsForDll("vulkan-1.dll")))
    {
        throw std::runtime_error("no Vulkan driver installed (vulkan-1.dll not found)");
    }
#endif
}

}  // namespace

VulkanBackend::VulkanBackend(const BackendOptions &options, GLFWwindow *window, uint32_t pushConstantBytes) : window_(window)
{
    requireVulkanLoader();
    try
    {
        gpu_.create(window, options.validate);
        swapchain_.vsync = options.vsync;
        swapchain_.create(gpu_, window, options.msaa);
        device_.create(gpu_, swapchain_, pushConstantBytes);
        presenter_.create(device_, swapchain_, window);
    }
    catch (...)
    {
        destroy();   // the window's surface and swapchain released: another backend may present to it next
        throw;
    }
}

void VulkanBackend::startUi()
{
    ImGui_ImplGlfw_InitForVulkan(window_, true);
    ui_.create(device_, presenter_, swapchain_);
}

void VulkanBackend::stopUi()
{
    ui_.destroy();
    ImGui_ImplGlfw_Shutdown();
}

// also what a failed constructor built so far: each part skips what it never made
void VulkanBackend::destroy()
{
    device_.destroy();
    swapchain_.destroy();
    presenter_.destroy();
    gpu_.destroy();
}

}  // namespace argon::gfx::vulkan
