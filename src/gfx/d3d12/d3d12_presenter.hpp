// Argon: gfx::Presenter over a DXGI flip-model swapchain. The scene pass draws into the presenter's own colour and
// depth targets (with the MSAA samples), resolved (or copied) into the back buffer when the pass ends or a frame copy
// is taken; the frame copies and the screenshot read the back buffer. A command allocator and fence value per frame
// slot; the window made again (ResizeBuffers) when its size changed.
#pragma once
#include "gfx/d3d12/d3d12_command_list.hpp"
#include "gfx/presenter.hpp"

#include <memory>
#include <optional>
#include <vector>

struct GLFWwindow;

namespace argon::gfx::d3d12
{

struct D3D12FrameCopy
{
    ComPtr<ID3D12Resource> resource;
    WindowSize size;
    TextureRef sceneView;   // the device's (a 2D array, for the scene sets)
    uint32_t uiView = 0;    // a 2D view in the shader-visible heap (ImGui)
};

class D3D12Presenter final : public Presenter
{
  public:
    void create(D3D12Device &device, GLFWwindow *window);
    void destroy();

    bool beginFrame() override;
    CommandList &commands() override;
    void beginScenePass(const SceneClear &clear) override;
    void endScenePass() override;
    void endFrame(FrameEnd end) override;
    bool pictureReady() const override;
    std::optional<Picture> takePicture() override;
    FrameCopy createFrameCopy() override;
    void destroyFrameCopy(FrameCopy &copy) override;
    bool fitFrameCopy(FrameCopy copy) override;
    bool frameCopyFits(FrameCopy copy) const override;
    TextureRef frameCopyTexture(FrameCopy copy) override;
    ImTextureID frameCopyImage(FrameCopy copy) const override;
    void suspendScenePass(FrameCopy copy) override;
    void resumeScenePass() override;
    void copySceneAndRestart(FrameCopy copy, const SceneClear &clear) override;
    void copyFinishedFrame(FrameCopy copy) override;
    void keepOitTargets(bool keep) override;
    void beginOitPass() override;
    void endOitPass() override;
    OitTargets oitTargets() override
    {
        return oit_.views;
    }
    uint64_t oitTargetsMade() const override
    {
        return oitMade_;
    }
    WindowSize windowSize() const override
    {
        return size_;
    }
    uint32_t samples() const override
    {
        return device_->sceneTargets().samples;
    }
    int frameSlot() const override
    {
        return slot_;
    }
    uint64_t framesSubmitted() const override
    {
        return submitted_;
    }
    void waitForSubmittedWork() override;
    bool takeRemade() override;
    void setVsync(bool on) override
    {
        vsync_ = on;
    }

    // for the UI renderer: the frame's command list
    D3D12CommandList &frameCommands()
    {
        return *commands_;
    }

  private:
    // the order-independent translucency's targets (keepOitTargets), at the window's size with the scene's samples
    struct OitResources
    {
        ComPtr<ID3D12Resource> accumulated, revealage;
        uint32_t accumulatedView = 0, revealageView = 0;   // render target views
        OitTargets views;                                  // the device's records of their shader views
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_RENDER_TARGET;
    };

    bool tearingSupported() const;
    UINT swapchainFlags() const;
    void createSwapchain();
    void createSceneTargets();
    void createOitTargets();
    void releaseOitTargets();
    void moveOitTargets(D3D12_RESOURCE_STATES to);
    void releaseWindowImages();
    void remakeWindow();
    void bindSceneTargets();
    void moveScene(D3D12_RESOURCE_STATES to);
    void moveBackBuffer(D3D12_RESOURCE_STATES to);
    void resolveIntoBackBuffer();
    void copyBackBufferInto(const D3D12FrameCopy &copy);
    void recordReadBack();
    D3D12FrameCopy &copyOf(FrameCopy copy) const;

    D3D12Device *device_ = nullptr;
    D3D12Context *context_ = nullptr;
    GLFWwindow *window_ = nullptr;
    WindowSize size_;
    ComPtr<IDXGISwapChain3> swapchain_;
    std::vector<ComPtr<ID3D12Resource>> backBuffers_;
    D3D12_RESOURCE_STATES backState_ = D3D12_RESOURCE_STATE_PRESENT;
    ComPtr<ID3D12Resource> sceneColour_, sceneDepth_;
    D3D12_RESOURCE_STATES sceneState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
    uint32_t colourView_ = 0, depthView_ = 0;
    ComPtr<ID3D12CommandAllocator> allocators_[kFramesInFlight];
    uint64_t slotFence_[kFramesInFlight]{};
    ComPtr<ID3D12GraphicsCommandList> list_;
    std::optional<D3D12CommandList> commands_;
    SceneClear lastClear_;
    int slot_ = 0;
    uint64_t submitted_ = 0;
    bool remade_ = false;
    bool vsync_ = true;
    bool tearing_ = false;   // the swapchain made with ALLOW_TEARING: unsynced frames are shown at once (vsync_ off)
    ComPtr<ID3D12Resource> readBack_;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT readBackLayout_{};
    bool pictureKept_ = false;
    uint64_t keptFence_ = 0;   // the fence value the kept frame signals
    std::vector<std::unique_ptr<D3D12FrameCopy>> copies_;   // by address (their handles)
    OitResources oit_;
    bool oitKept_ = false;
    uint64_t oitMade_ = 0;   // oitTargetsMade
};

}  // namespace argon::gfx::d3d12
