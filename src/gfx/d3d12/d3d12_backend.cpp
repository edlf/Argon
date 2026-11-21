// Argon: gfx::Backend over Direct3D 12 (gfx\d3d12\d3d12_backend.hpp).
#include "gfx/d3d12/d3d12_backend.hpp"

#include <imgui_impl_glfw.h>

namespace argon::gfx::d3d12
{

D3D12Backend::D3D12Backend(const BackendOptions &options, GLFWwindow *window, uint32_t pushConstantBytes) : window_(window)
{
    context_.create(options.validate);
    SceneTargets targets;
    targets.samples = samplesFor(options.msaa);
    device_.create(context_, targets, pushConstantBytes);
    presenter_.create(device_, window);
    presenter_.setVsync(options.vsync);
}

// the highest sample count <= the one asked for that both scene targets take (--msaa N)
uint32_t D3D12Backend::samplesFor(int wanted) const
{
    for (const uint32_t samples : {8u, 4u, 2u})
    {
        if (int(samples) > wanted)
        {
            continue;
        }
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS colour{DXGI_FORMAT_B8G8R8A8_UNORM, samples};
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS depth{DXGI_FORMAT_D32_FLOAT, samples};
        context_.device()->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &colour, sizeof colour);
        context_.device()->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &depth, sizeof depth);
        if (colour.NumQualityLevels > 0 && depth.NumQualityLevels > 0)
        {
            return samples;
        }
    }
    return 1;
}

void D3D12Backend::startUi()
{
    ImGui_ImplGlfw_InitForOther(window_, true);
    ui_.create(device_, presenter_);
}

void D3D12Backend::stopUi()
{
    ui_.destroy();
    ImGui_ImplGlfw_Shutdown();
}

void D3D12Backend::destroy()
{
    context_.waitIdle();
    presenter_.destroy();
    device_.destroy();
    context_.destroy();
}

}  // namespace argon::gfx::d3d12
