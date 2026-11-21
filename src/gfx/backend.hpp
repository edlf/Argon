// Argon: the renderer a run draws with, one per window: a backend's device, presenter and UI renderer
// (gfx\vulkan\vulkan_backend.hpp). Chosen with --renderer or settings.json's "renderer"; Vulkan unless asked.
#pragma once
#include "gfx/backend_kind.hpp"
#include "gfx/device.hpp"
#include "gfx/presenter.hpp"
#include "gfx/ui_renderer.hpp"

#include <memory>

struct GLFWwindow;

namespace argon::gfx
{

struct BackendOptions
{
    BackendKind kind = BackendKind::Vulkan;
    bool validate = false;   // the API's validation / debug layer
    int msaa = 4;            // the samples asked for (the presenter's samples() has what the GPU gives)
    bool vsync = true;       // Presenter::setVsync
};

class Backend
{
  public:
    virtual ~Backend() = default;

    virtual Device &device() = 0;
    virtual Presenter &presenter() = 0;
    virtual UiRenderer &ui() = 0;
    // ImGui's platform and renderer backends (its context made first), and back
    virtual void startUi() = 0;
    virtual void stopUi() = 0;
    // everything it made, the device's objects too (the GPU idle and the UI stopped first)
    virtual void destroy() = 0;
};

// the GPU, the window's surface and the swapchain, the device with the push constants it needs; throws when the backend
// cannot run here
std::unique_ptr<Backend> createBackend(const BackendOptions &options, GLFWwindow *window, uint32_t pushConstantBytes);

}  // namespace argon::gfx
