// Argon: gfx::Presenter over a DXGI swapchain (gfx\d3d12\d3d12_presenter.hpp).
#include "gfx/d3d12/d3d12_presenter.hpp"
#include "gfx/d3d12/d3d12_ui_image.hpp"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <algorithm>
#include <cstring>

namespace argon::gfx::d3d12
{
namespace
{
const UINT kBackBuffers = 3;
const DXGI_FORMAT kWindowFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
const D3D12_RESOURCE_STATES kShaderRead = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
// the frame copies read alpha as 1 (the copy is drawn opaque whatever the window's alpha holds)
const UINT kOpaqueMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 1, 2, D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);

D3D12_RESOURCE_BARRIER transition(ID3D12Resource *resource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
{
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition = {resource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, from, to};
    return barrier;
}

struct IsCopy
{
    const D3D12FrameCopy *copy;

    bool operator()(const std::unique_ptr<D3D12FrameCopy> &kept) const
    {
        return kept.get() == copy;
    }
};

WindowSize framebufferSize(GLFWwindow *window)
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    return WindowSize{uint32_t(std::max(width, 0)), uint32_t(std::max(height, 0))};
}

struct TargetDesc
{
    DXGI_FORMAT format;
    WindowSize size;
    uint32_t samples;
    D3D12_RESOURCE_FLAGS flags;
};

ComPtr<ID3D12Resource> createTarget(ID3D12Device *device, const TargetDesc &target, const D3D12_CLEAR_VALUE &clear)
{
    D3D12_HEAP_PROPERTIES heap{D3D12_HEAP_TYPE_DEFAULT};
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = target.size.width;
    desc.Height = target.size.height;
    desc.DepthOrArraySize = desc.MipLevels = 1;
    desc.Format = target.format;
    desc.SampleDesc.Count = target.samples;
    desc.Flags = target.flags;
    const D3D12_RESOURCE_STATES state =
        target.flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL ? D3D12_RESOURCE_STATE_DEPTH_WRITE : D3D12_RESOURCE_STATE_RENDER_TARGET;
    ComPtr<ID3D12Resource> made;
    check(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, state, &clear, IID_PPV_ARGS(&made)), "CreateCommittedResource (target)");
    return made;
}
}  // namespace

void D3D12Presenter::create(D3D12Device &device, GLFWwindow *window)
{
    device_ = &device;
    context_ = &device.context();
    window_ = window;
    size_ = framebufferSize(window);
    for (ComPtr<ID3D12CommandAllocator> &allocator : allocators_)
    {
        check(context_->device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)), "CreateCommandAllocator");
    }
    check(context_->device()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocators_[0].Get(), nullptr, IID_PPV_ARGS(&list_)), "CreateCommandList");
    check(list_->Close(), "Close");
    colourView_ = context_->renderTargets().take();
    depthView_ = context_->depthTargets().take();
    createSwapchain();
    createSceneTargets();
}

// DXGI 1.5's tearing (variable refresh displays, unsynced frames in a window); none on older systems
bool D3D12Presenter::tearingSupported() const
{
    ComPtr<IDXGIFactory5> factory;
    BOOL allowed = FALSE;
    if (FAILED(context_->factory()->QueryInterface(IID_PPV_ARGS(&factory))) ||
        FAILED(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowed, sizeof(allowed))))
    {
        return false;
    }
    return allowed == TRUE;
}

UINT D3D12Presenter::swapchainFlags() const
{
    return tearing_ ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
}

void D3D12Presenter::createSwapchain()
{
    tearing_ = tearingSupported();
    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = size_.width;
    desc.Height = size_.height;
    desc.Format = kWindowFormat;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = kBackBuffers;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.Flags = swapchainFlags();
    const HWND hwnd = glfwGetWin32Window(window_);
    ComPtr<IDXGISwapChain1> swapchain;
    check(context_->factory()->CreateSwapChainForHwnd(context_->queue(), hwnd, &desc, nullptr, nullptr, &swapchain), "CreateSwapChainForHwnd");
    check(swapchain.As(&swapchain_), "IDXGISwapChain3");
    context_->factory()->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);   // F11 is the viewer's fullscreen
    for (UINT i = 0; i < kBackBuffers; ++i)
    {
        ComPtr<ID3D12Resource> buffer;
        check(swapchain_->GetBuffer(i, IID_PPV_ARGS(&buffer)), "GetBuffer");
        backBuffers_.push_back(buffer);
    }
}

// the scene pass's colour and depth, at the window's size with the MSAA samples
void D3D12Presenter::createSceneTargets()
{
    const SceneTargets &targets = device_->sceneTargets();
    D3D12_CLEAR_VALUE colourClear{targets.colour};
    D3D12_CLEAR_VALUE depthClear{targets.depth};
    sceneColour_ = createTarget(context_->device(), {targets.colour, size_, targets.samples, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET}, colourClear);
    sceneDepth_ = createTarget(context_->device(), {targets.depth, size_, targets.samples, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL}, depthClear);
    sceneState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
    context_->device()->CreateRenderTargetView(sceneColour_.Get(), nullptr, context_->renderTargets().cpu(colourView_));
    context_->device()->CreateDepthStencilView(sceneDepth_.Get(), nullptr, context_->depthTargets().cpu(depthView_));
    if (oitKept_)
    {
        createOitTargets();
    }
}

// the accumulated colour cleared to 0 and the revealage to 1 (beginOitPass); their shader views 2D or, with MSAA, 2DMS
void D3D12Presenter::createOitTargets()
{
    const uint32_t samples = device_->sceneTargets().samples;
    const D3D12_CLEAR_VALUE zero{kOitAccumulatedFormat, {0.f, 0.f, 0.f, 0.f}};
    const D3D12_CLEAR_VALUE one{kOitRevealageFormat, {1.f, 1.f, 1.f, 1.f}};
    oit_.accumulated = createTarget(context_->device(), {kOitAccumulatedFormat, size_, samples, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET}, zero);
    oit_.revealage = createTarget(context_->device(), {kOitRevealageFormat, size_, samples, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET}, one);
    oit_.state = D3D12_RESOURCE_STATE_RENDER_TARGET;
    if (!oit_.accumulatedView)
    {
        oit_.accumulatedView = context_->renderTargets().take();
        oit_.revealageView = context_->renderTargets().take();
    }
    context_->device()->CreateRenderTargetView(oit_.accumulated.Get(), nullptr, context_->renderTargets().cpu(oit_.accumulatedView));
    context_->device()->CreateRenderTargetView(oit_.revealage.Get(), nullptr, context_->renderTargets().cpu(oit_.revealageView));
    D3D12_SHADER_RESOURCE_VIEW_DESC view{kOitAccumulatedFormat, samples > 1 ? D3D12_SRV_DIMENSION_TEXTURE2DMS : D3D12_SRV_DIMENSION_TEXTURE2D,
                                         D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING};
    view.Texture2D.MipLevels = samples > 1 ? 0 : 1;
    oit_.views.accumulated = device_->adoptTexture(oit_.accumulated.Get(), view);
    view.Format = kOitRevealageFormat;
    oit_.views.revealage = device_->adoptTexture(oit_.revealage.Get(), view);
}

void D3D12Presenter::releaseOitTargets()
{
    for (const TextureRef view : {oit_.views.accumulated, oit_.views.revealage})
    {
        if (view)
        {
            device_->forgetTexture(view);
        }
    }
    oit_.views = {};
    oit_.accumulated.Reset();
    oit_.revealage.Reset();
}

void D3D12Presenter::moveOitTargets(D3D12_RESOURCE_STATES to)
{
    if (oit_.state == to)
    {
        return;
    }
    const D3D12_RESOURCE_BARRIER barriers[2] = {transition(oit_.accumulated.Get(), oit_.state, to), transition(oit_.revealage.Get(), oit_.state, to)};
    list_->ResourceBarrier(2, barriers);
    oit_.state = to;
}

void D3D12Presenter::releaseWindowImages()
{
    backBuffers_.clear();
    sceneColour_.Reset();
    sceneDepth_.Reset();
    readBack_.Reset();
    pictureKept_ = false;
    releaseOitTargets();
}

void D3D12Presenter::destroy()
{
    if (!context_)
    {
        return;
    }
    context_->waitIdle();
    releaseWindowImages();
    swapchain_.Reset();
}

// a minimised window waits until it has a size again
void D3D12Presenter::remakeWindow()
{
    WindowSize size = framebufferSize(window_);
    while (size.width == 0 || size.height == 0)
    {
        glfwWaitEvents();
        size = framebufferSize(window_);
    }
    context_->waitIdle();
    releaseWindowImages();
    size_ = size;
    check(swapchain_->ResizeBuffers(kBackBuffers, size.width, size.height, kWindowFormat, swapchainFlags()), "ResizeBuffers");
    for (UINT i = 0; i < kBackBuffers; ++i)
    {
        ComPtr<ID3D12Resource> buffer;
        check(swapchain_->GetBuffer(i, IID_PPV_ARGS(&buffer)), "GetBuffer");
        backBuffers_.push_back(buffer);
    }
    createSceneTargets();
    remade_ = true;
    if (oitKept_)
    {
        ++oitMade_;
    }
}

// ------------------------------------------------------------------ the frame
bool D3D12Presenter::beginFrame()
{
    const WindowSize now = framebufferSize(window_);
    if (now.width != size_.width || now.height != size_.height)
    {
        remakeWindow();
        return false;
    }
    context_->waitFor(slotFence_[slot_]);
    check(allocators_[slot_]->Reset(), "allocator Reset");
    check(list_->Reset(allocators_[slot_].Get(), nullptr), "list Reset");
    backState_ = D3D12_RESOURCE_STATE_PRESENT;
    commands_.emplace(list_.Get(), *device_);
    commands_->bindRoot();
    return true;
}

CommandList &D3D12Presenter::commands()
{
    return *commands_;
}

void D3D12Presenter::moveScene(D3D12_RESOURCE_STATES to)
{
    if (sceneState_ != to)
    {
        const D3D12_RESOURCE_BARRIER barrier = transition(sceneColour_.Get(), sceneState_, to);
        list_->ResourceBarrier(1, &barrier);
        sceneState_ = to;
    }
}

void D3D12Presenter::moveBackBuffer(D3D12_RESOURCE_STATES to)
{
    if (backState_ != to)
    {
        const D3D12_RESOURCE_BARRIER barrier = transition(backBuffers_[swapchain_->GetCurrentBackBufferIndex()].Get(), backState_, to);
        list_->ResourceBarrier(1, &barrier);
        backState_ = to;
    }
}

void D3D12Presenter::bindSceneTargets()
{
    moveScene(D3D12_RESOURCE_STATE_RENDER_TARGET);
    const D3D12_CPU_DESCRIPTOR_HANDLE colour = context_->renderTargets().cpu(colourView_), depth = context_->depthTargets().cpu(depthView_);
    list_->OMSetRenderTargets(1, &colour, FALSE, &depth);
    commands_->useTargets({colour, depth});
}

void D3D12Presenter::beginScenePass(const SceneClear &clear)
{
    bindSceneTargets();
    const Rect window{0, 0, size_.width, size_.height};
    commands_->clear(window, clear);
    commands_->setViewport(window);
    lastClear_ = clear;
}

// the scene so far into the back buffer (resolved when multisampled)
void D3D12Presenter::resolveIntoBackBuffer()
{
    ID3D12Resource *back = backBuffers_[swapchain_->GetCurrentBackBufferIndex()].Get();
    if (device_->sceneTargets().samples > 1)
    {
        moveScene(D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
        moveBackBuffer(D3D12_RESOURCE_STATE_RESOLVE_DEST);
        list_->ResolveSubresource(back, 0, sceneColour_.Get(), 0, kWindowFormat);
        return;
    }
    moveScene(D3D12_RESOURCE_STATE_COPY_SOURCE);
    moveBackBuffer(D3D12_RESOURCE_STATE_COPY_DEST);
    list_->CopyResource(back, sceneColour_.Get());
}

void D3D12Presenter::endScenePass()
{
    resolveIntoBackBuffer();
}

void D3D12Presenter::endFrame(FrameEnd end)
{
    device_->resolveEndedQueries(list_.Get());
    if (end == FrameEnd::ShowAndKeep)
    {
        recordReadBack();
    }
    moveBackBuffer(D3D12_RESOURCE_STATE_PRESENT);
    check(list_->Close(), "Close");
    commands_.reset();
    ID3D12CommandList *lists[] = {list_.Get()};
    context_->queue()->ExecuteCommandLists(1, lists);
    const UINT presentFlags = !vsync_ && tearing_ ? DXGI_PRESENT_ALLOW_TEARING : 0;
    check(swapchain_->Present(vsync_ ? 1 : 0, presentFlags), "Present");
    slotFence_[slot_] = context_->signal();
    device_->framesSubmitted(slotFence_[slot_]);
    ++submitted_;
    if (end == FrameEnd::ShowAndKeep)
    {
        pictureKept_ = true;
        keptFence_ = slotFence_[slot_];
    }
    slot_ = (slot_ + 1) % kFramesInFlight;
}

// the finished back buffer into the read-back buffer (made at the first kept frame, the window's size then)
void D3D12Presenter::recordReadBack()
{
    ID3D12Resource *back = backBuffers_[swapchain_->GetCurrentBackBufferIndex()].Get();
    const D3D12_RESOURCE_DESC desc = back->GetDesc();
    UINT64 bytes = 0;
    context_->device()->GetCopyableFootprints(&desc, 0, 1, 0, &readBackLayout_, nullptr, nullptr, &bytes);
    if (!readBack_)
    {
        readBack_ = context_->createBuffer(D3D12_HEAP_TYPE_READBACK, bytes);
    }
    moveBackBuffer(D3D12_RESOURCE_STATE_COPY_SOURCE);
    D3D12_TEXTURE_COPY_LOCATION to{readBack_.Get(), D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
    to.PlacedFootprint = readBackLayout_;
    D3D12_TEXTURE_COPY_LOCATION from{back, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
    list_->CopyTextureRegion(&to, 0, 0, 0, &from, nullptr);
}

bool D3D12Presenter::pictureReady() const
{
    return !pictureKept_ || context_->reached(keptFence_);
}

// the back buffer's BGRA rows, packed (the read-back rows are pitched)
std::optional<Picture> D3D12Presenter::takePicture()
{
    if (!pictureKept_)
    {
        return std::nullopt;
    }
    pictureKept_ = false;
    context_->waitFor(keptFence_);
    Picture picture;
    picture.width = readBackLayout_.Footprint.Width;
    picture.height = readBackLayout_.Footprint.Height;
    picture.bgra = true;
    const size_t rowBytes = size_t(picture.width) * 4;
    picture.pixels.resize(rowBytes * picture.height);
    uint8_t *mapped = nullptr;
    check(readBack_->Map(0, nullptr, reinterpret_cast<void **>(&mapped)), "Map (picture)");
    for (uint32_t y = 0; y < picture.height; ++y)
    {
        const uint8_t *row = mapped + readBackLayout_.Offset + size_t(y) * readBackLayout_.Footprint.RowPitch;
        std::memcpy(&picture.pixels[y * rowBytes], row, rowBytes);
    }
    const D3D12_RANGE nothingWritten{0, 0};
    readBack_->Unmap(0, &nothingWritten);
    return picture;
}

void D3D12Presenter::waitForSubmittedWork()
{
    for (const uint64_t fence : slotFence_)
    {
        context_->waitFor(fence);
    }
    context_->freeFinishedUploads();
}

bool D3D12Presenter::takeRemade()
{
    const bool remade = remade_;
    remade_ = false;
    return remade;
}

// ------------------------------------------------------------------ frame copies
D3D12FrameCopy &D3D12Presenter::copyOf(FrameCopy copy) const
{
    return *reinterpret_cast<D3D12FrameCopy *>(copy.value);
}

FrameCopy D3D12Presenter::createFrameCopy()
{
    copies_.push_back(std::make_unique<D3D12FrameCopy>());
    return FrameCopy{reinterpret_cast<uint64_t>(copies_.back().get())};
}

void D3D12Presenter::destroyFrameCopy(FrameCopy &copy)
{
    D3D12FrameCopy &kept = copyOf(copy);
    if (kept.sceneView)
    {
        device_->forgetTexture(kept.sceneView);
    }
    context_->shaderViews().give(kept.uiView);
    std::erase_if(copies_, IsCopy{&kept});
    copy = {};
}

bool D3D12Presenter::frameCopyFits(FrameCopy copy) const
{
    const D3D12FrameCopy &kept = copyOf(copy);
    return kept.resource && kept.size.width == size_.width && kept.size.height == size_.height;
}

// made black at the window's size: its scene view (a 2D array) and its ImGui view, both reading alpha as 1
bool D3D12Presenter::fitFrameCopy(FrameCopy copy)
{
    if (frameCopyFits(copy))
    {
        return true;
    }
    context_->waitIdle();
    D3D12FrameCopy &kept = copyOf(copy);
    if (kept.sceneView)
    {
        device_->forgetTexture(kept.sceneView);
    }
    const D3D12_CLEAR_VALUE black{kWindowFormat, {0.f, 0.f, 0.f, 1.f}};
    kept.resource = createTarget(context_->device(), {kWindowFormat, size_, 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET}, black);
    kept.size = size_;
    const uint32_t clearView = context_->renderTargets().take();
    context_->device()->CreateRenderTargetView(kept.resource.Get(), nullptr, context_->renderTargets().cpu(clearView));
    ID3D12GraphicsCommandList *commands = context_->beginUpload();
    commands->ClearRenderTargetView(context_->renderTargets().cpu(clearView), black.Color, 0, nullptr);
    const D3D12_RESOURCE_BARRIER readable = transition(kept.resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, kShaderRead);
    commands->ResourceBarrier(1, &readable);
    context_->endUpload();
    context_->renderTargets().give(clearView);
    D3D12_SHADER_RESOURCE_VIEW_DESC view{kWindowFormat, D3D12_SRV_DIMENSION_TEXTURE2DARRAY, kOpaqueMapping};
    view.Texture2DArray.MipLevels = view.Texture2DArray.ArraySize = 1;
    kept.sceneView = device_->adoptTexture(kept.resource.Get(), view);
    if (!kept.uiView)
    {
        kept.uiView = context_->shaderViews().take();
    }
    D3D12_SHADER_RESOURCE_VIEW_DESC flat{kWindowFormat, D3D12_SRV_DIMENSION_TEXTURE2D, kOpaqueMapping};
    flat.Texture2D.MipLevels = 1;
    context_->device()->CreateShaderResourceView(kept.resource.Get(), &flat, context_->shaderViews().cpu(kept.uiView));
    return true;
}

TextureRef D3D12Presenter::frameCopyTexture(FrameCopy copy)
{
    return copyOf(copy).sceneView;
}

ImTextureID D3D12Presenter::frameCopyImage(FrameCopy copy) const
{
    const D3D12FrameCopy &kept = copyOf(copy);
    return kept.resource ? uiImageOf(kept.uiView, SamplerMode::Clamp) : ImTextureID{};
}

void D3D12Presenter::copyBackBufferInto(const D3D12FrameCopy &copy)
{
    moveBackBuffer(D3D12_RESOURCE_STATE_COPY_SOURCE);
    const D3D12_RESOURCE_BARRIER writable = transition(copy.resource.Get(), kShaderRead, D3D12_RESOURCE_STATE_COPY_DEST);
    list_->ResourceBarrier(1, &writable);
    list_->CopyResource(copy.resource.Get(), backBuffers_[swapchain_->GetCurrentBackBufferIndex()].Get());
    const D3D12_RESOURCE_BARRIER readable = transition(copy.resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, kShaderRead);
    list_->ResourceBarrier(1, &readable);
}

void D3D12Presenter::suspendScenePass(FrameCopy copy)
{
    resolveIntoBackBuffer();
    copyBackBufferInto(copyOf(copy));
}

// the scene goes on over what it drew (the Hud Lens copy, the post-process anti-aliasing)
void D3D12Presenter::resumeScenePass()
{
    bindSceneTargets();
}

void D3D12Presenter::copySceneAndRestart(FrameCopy copy, const SceneClear &clear)
{
    resolveIntoBackBuffer();
    copyBackBufferInto(copyOf(copy));
    beginScenePass(clear);
}

void D3D12Presenter::copyFinishedFrame(FrameCopy copy)
{
    copyBackBufferInto(copyOf(copy));
}

// ------------------------------------------------------------------ order-independent translucency
void D3D12Presenter::keepOitTargets(bool keep)
{
    if (keep == oitKept_)
    {
        return;
    }
    context_->waitIdle();
    oitKept_ = keep;
    if (keep)
    {
        createOitTargets();
    }
    else
    {
        releaseOitTargets();
    }
    ++oitMade_;
}

// the two targets cleared and drawn into over the scene's depth (the viewport kept)
void D3D12Presenter::beginOitPass()
{
    moveOitTargets(D3D12_RESOURCE_STATE_RENDER_TARGET);
    const D3D12_CPU_DESCRIPTOR_HANDLE targets[kOitTargetCount] = {context_->renderTargets().cpu(oit_.accumulatedView),
                                                                   context_->renderTargets().cpu(oit_.revealageView)};
    const float zero[4] = {0.f, 0.f, 0.f, 0.f}, one[4] = {1.f, 1.f, 1.f, 1.f};
    list_->ClearRenderTargetView(targets[0], zero, 0, nullptr);
    list_->ClearRenderTargetView(targets[1], one, 0, nullptr);
    const D3D12_CPU_DESCRIPTOR_HANDLE depth = context_->depthTargets().cpu(depthView_);
    list_->OMSetRenderTargets(kOitTargetCount, targets, FALSE, &depth);
}

void D3D12Presenter::endOitPass()
{
    moveOitTargets(kShaderRead);
    bindSceneTargets();
}

}  // namespace argon::gfx::d3d12
