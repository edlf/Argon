// Argon: the Direct3D 12 backend's shared parts (gfx\d3d12\d3d12_context.hpp).
#include "gfx/d3d12/d3d12_context.hpp"

#include <algorithm>
#include <cstdio>

namespace argon::gfx::d3d12
{
namespace
{
// the shader-visible heaps: every texture / material / scene set and UI image; the sampler tables
const uint32_t kShaderViews = 1u << 18, kSamplers = 2048;
const uint32_t kCpuViews = 1u << 16, kRenderTargets = 256, kDepthTargets = 16;

// the debug layer's messages, as the Vulkan validation's are printed ([vk])
void CALLBACK printDebugMessage(D3D12_MESSAGE_CATEGORY, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID, LPCSTR description, void *)
{
    if (severity <= D3D12_MESSAGE_SEVERITY_WARNING)
    {
        std::fprintf(stderr, "[d3d12] %s\n", description);
    }
}
}  // namespace

void check(HRESULT result, const char *what)
{
    if (FAILED(result))
    {
        char code[16];
        std::snprintf(code, sizeof code, "0x%08lx", static_cast<unsigned long>(result));
        throw std::runtime_error(std::string("Direct3D 12: ") + what + " failed (" + code + ")");
    }
}

// ------------------------------------------------------------------ descriptor heaps
void DescriptorHeap::make(ID3D12Device *device, const D3D12_DESCRIPTOR_HEAP_DESC &desc)
{
    check(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap_)), "CreateDescriptorHeap");
    stride_ = device->GetDescriptorHandleIncrementSize(desc.Type);
    end_ = desc.NumDescriptors;
}

void DescriptorHeap::create(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count)
{
    make(device, D3D12_DESCRIPTOR_HEAP_DESC{type, count, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0});
}

void DescriptorHeap::createShaderVisible(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count)
{
    make(device, D3D12_DESCRIPTOR_HEAP_DESC{type, count, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0});
}

uint32_t DescriptorHeap::take()
{
    if (!free_.empty())
    {
        const uint32_t index = free_.back();
        free_.pop_back();
        return index;
    }
    return reserve(1);
}

void DescriptorHeap::give(uint32_t index)
{
    if (index)
    {
        free_.push_back(index);
    }
}

uint32_t DescriptorHeap::reserve(uint32_t count)
{
    if (next_ + count > end_)
    {
        throw std::runtime_error("Direct3D 12: a descriptor heap is full");
    }
    const uint32_t first = next_;
    next_ += count;
    return first;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::cpu(uint32_t index) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = heap_->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += SIZE_T(index) * stride_;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::gpu(uint32_t index) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = heap_->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += UINT64(index) * stride_;
    return handle;
}

// ------------------------------------------------------------------ device, queue, fence
void D3D12Context::create(bool debugLayer)
{
    UINT factoryFlags = 0;
    if (debugLayer)
    {
        ComPtr<ID3D12Debug> debug;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
        {
            debug->EnableDebugLayer();
            factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
        }
        else
        {
            std::fprintf(stderr, "d3d12: the debug layer is not installed (Graphics Tools)\n");
        }
    }
    check(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory_)), "CreateDXGIFactory2");
    pickAdapter();
    ComPtr<ID3D12InfoQueue1> messages;
    if (factoryFlags && SUCCEEDED(device_.As(&messages)))
    {
        // clearing to another colour than the target's creation value is only slower (the scene's fog colour changes)
        D3D12_MESSAGE_ID quiet[] = {D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE, D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE};
        D3D12_INFO_QUEUE_FILTER filter{};
        filter.DenyList.NumIDs = 2;
        filter.DenyList.pIDList = quiet;
        messages->AddStorageFilterEntries(&filter);
        DWORD cookie = 0;
        messages->RegisterMessageCallback(printDebugMessage, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &cookie);
    }
    const D3D12_COMMAND_QUEUE_DESC queue{D3D12_COMMAND_LIST_TYPE_DIRECT};
    check(device_->CreateCommandQueue(&queue, IID_PPV_ARGS(&queue_)), "CreateCommandQueue");
    check(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)), "CreateFence");
    fenceEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    check(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&uploadAllocator_)), "CreateCommandAllocator");
    check(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, uploadAllocator_.Get(), nullptr, IID_PPV_ARGS(&uploadList_)), "CreateCommandList");
    check(uploadList_->Close(), "Close");
    shaderViews_.createShaderVisible(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kShaderViews);
    samplers_.createShaderVisible(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, kSamplers);
    cpuViews_.create(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kCpuViews);
    renderTargets_.create(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kRenderTargets);
    depthTargets_.create(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, kDepthTargets);
}

// the first hardware adapter that makes a feature level 12.0 device
void D3D12Context::pickAdapter()
{
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; factory_->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 && SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device_))))
        {
            check(adapter.As(&adapter_), "IDXGIAdapter3");
            std::printf("d3d12: %ls\n", desc.Description);
            return;
        }
    }
    throw std::runtime_error("Direct3D 12: no GPU makes a feature level 12.0 device");
}

void D3D12Context::destroy()
{
    if (!device_)
    {
        return;
    }
    waitIdle();
    staging_.clear();
    CloseHandle(fenceEvent_);
}

uint64_t D3D12Context::signal()
{
    const uint64_t value = nextFence_++;
    check(queue_->Signal(fence_.Get(), value), "Signal");
    return value;
}

bool D3D12Context::reached(uint64_t value) const
{
    return fence_->GetCompletedValue() >= value;
}

void D3D12Context::waitFor(uint64_t value)
{
    if (reached(value))
    {
        return;
    }
    check(fence_->SetEventOnCompletion(value, fenceEvent_), "SetEventOnCompletion");
    WaitForSingleObject(fenceEvent_, INFINITE);
}

void D3D12Context::waitIdle()
{
    waitFor(signal());
    freeFinishedUploads();
}

// ------------------------------------------------------------------ uploads
// the list's allocator is free again once its last run is done
void D3D12Context::openUploadList()
{
    waitFor(uploadDone_);
    check(uploadAllocator_->Reset(), "upload allocator Reset");
    check(uploadList_->Reset(uploadAllocator_.Get(), nullptr), "upload list Reset");
    uploadOpen_ = true;
}

void D3D12Context::runUploadList()
{
    check(uploadList_->Close(), "upload list Close");
    ID3D12CommandList *lists[] = {uploadList_.Get()};
    queue_->ExecuteCommandLists(1, lists);
    uploadDone_ = signal();
    uploadOpen_ = false;
    for (KeptStaging &staging : staging_)
    {
        staging.fence = std::min(staging.fence, uploadDone_);
    }
}

ID3D12GraphicsCommandList *D3D12Context::beginUpload()
{
    if (!uploadOpen_)
    {
        openUploadList();
    }
    return uploadList_.Get();
}

void D3D12Context::endUpload()
{
    if (batchOpen_)
    {
        return;
    }
    runUploadList();
    waitFor(uploadDone_);
    freeFinishedUploads();
}

void D3D12Context::keepUntilUploaded(const ComPtr<ID3D12Resource> &staging)
{
    staging_.push_back({staging, UINT64_MAX});   // its fence: the upload list's, when it runs
}

void D3D12Context::beginUploadBatch()
{
    batchOpen_ = true;
}

void D3D12Context::submitUploadBatch()
{
    batchOpen_ = false;
    if (uploadOpen_)
    {
        runUploadList();
    }
    freeFinishedUploads();
}

void D3D12Context::freeFinishedUploads()
{
    std::vector<KeptStaging> kept;
    for (KeptStaging &staging : staging_)
    {
        if (!reached(staging.fence))
        {
            kept.push_back(std::move(staging));
        }
    }
    staging_ = std::move(kept);
}

ComPtr<ID3D12Resource> D3D12Context::createBuffer(D3D12_HEAP_TYPE heap, uint64_t bytes)
{
    D3D12_HEAP_PROPERTIES props{heap};
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = bytes ? bytes : 1;
    desc.Height = desc.DepthOrArraySize = desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    const D3D12_RESOURCE_STATES state = heap == D3D12_HEAP_TYPE_UPLOAD     ? D3D12_RESOURCE_STATE_GENERIC_READ
                                        : heap == D3D12_HEAP_TYPE_READBACK ? D3D12_RESOURCE_STATE_COPY_DEST
                                                                           : D3D12_RESOURCE_STATE_COMMON;
    ComPtr<ID3D12Resource> buffer;
    check(device_->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr, IID_PPV_ARGS(&buffer)), "CreateCommittedResource (buffer)");
    return buffer;
}

}  // namespace argon::gfx::d3d12
