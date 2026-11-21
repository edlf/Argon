// Argon: gfx::Backend over Direct3D 12 (Windows): the shared context, and the device, presenter and UI renderer made
// over it, with the MSAA samples the GPU takes for the scene's targets.
#pragma once
#include "gfx/backend.hpp"
#include "gfx/d3d12/d3d12_ui_renderer.hpp"

namespace argon::gfx::d3d12
{

class D3D12Backend final : public Backend
{
  public:
    D3D12Backend(const BackendOptions &options, GLFWwindow *window, uint32_t pushConstantBytes);

    Device &device() override
    {
        return device_;
    }
    Presenter &presenter() override
    {
        return presenter_;
    }
    UiRenderer &ui() override
    {
        return ui_;
    }
    void startUi() override;
    void stopUi() override;
    void destroy() override;

  private:
    uint32_t samplesFor(int wanted) const;

    GLFWwindow *window_;
    D3D12Context context_;
    D3D12Device device_;
    D3D12Presenter presenter_;
    D3D12UiRenderer ui_;
};

}  // namespace argon::gfx::d3d12
