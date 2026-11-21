// Argon: what the Direct3D 12 backend's parts share: the device and its direct queue, the fence the frames and uploads
// signal, the descriptor heaps, and the uploads (one-shot, or batched over a track load's frame slices).
#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace argon::gfx::d3d12
{

using Microsoft::WRL::ComPtr;

// throws with what failed and the HRESULT
void check(HRESULT result, const char *what);

// a descriptor heap handed out one slot at a time (freed again) or in blocks kept for the run
class DescriptorHeap
{
  public:
    void create(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count);
    void createShaderVisible(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count);
    uint32_t take();
    void give(uint32_t index);
    // `count` slots in a row, never given back
    uint32_t reserve(uint32_t count);
    D3D12_CPU_DESCRIPTOR_HANDLE cpu(uint32_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu(uint32_t index) const;
    ID3D12DescriptorHeap *heap() const
    {
        return heap_.Get();
    }

  private:
    void make(ID3D12Device *device, const D3D12_DESCRIPTOR_HEAP_DESC &desc);

    ComPtr<ID3D12DescriptorHeap> heap_;
    uint32_t stride_ = 0, next_ = 1, end_ = 0;   // slot 0 stays unused (an empty handle)
    std::vector<uint32_t> free_;
};

class D3D12Context
{
  public:
    // debugLayer: D3D12's debug layer (--validate)
    void create(bool debugLayer);
    void destroy();

    ID3D12Device *device() const
    {
        return device_.Get();
    }
    ID3D12CommandQueue *queue() const
    {
        return queue_.Get();
    }
    IDXGIFactory4 *factory() const
    {
        return factory_.Get();
    }
    IDXGIAdapter3 *adapter() const
    {
        return adapter_.Get();
    }

    // the fence: signalling the queue, waiting
    uint64_t signal();
    bool reached(uint64_t value) const;
    void waitFor(uint64_t value);
    void waitIdle();

    // upload commands: the open batch's, else a one-shot list run and waited for by endUpload
    ID3D12GraphicsCommandList *beginUpload();
    void endUpload();
    // kept until the commands that read it ran
    void keepUntilUploaded(const ComPtr<ID3D12Resource> &staging);
    void beginUploadBatch();
    void submitUploadBatch();
    void freeFinishedUploads();

    DescriptorHeap &shaderViews()
    {
        return shaderViews_;
    }
    DescriptorHeap &samplers()
    {
        return samplers_;
    }
    DescriptorHeap &cpuViews()
    {
        return cpuViews_;
    }
    DescriptorHeap &renderTargets()
    {
        return renderTargets_;
    }
    DescriptorHeap &depthTargets()
    {
        return depthTargets_;
    }

    // an upload-heap buffer (mapped by the caller) or a default-heap one
    ComPtr<ID3D12Resource> createBuffer(D3D12_HEAP_TYPE heap, uint64_t bytes);

  private:
    struct KeptStaging
    {
        ComPtr<ID3D12Resource> resource;
        uint64_t fence;
    };

    void pickAdapter();
    void openUploadList();
    void runUploadList();

    ComPtr<IDXGIFactory4> factory_;
    ComPtr<IDXGIAdapter3> adapter_;
    ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12CommandQueue> queue_;
    ComPtr<ID3D12Fence> fence_;
    HANDLE fenceEvent_ = nullptr;
    uint64_t nextFence_ = 1;
    ComPtr<ID3D12CommandAllocator> uploadAllocator_;
    ComPtr<ID3D12GraphicsCommandList> uploadList_;
    bool uploadOpen_ = false, batchOpen_ = false;
    uint64_t uploadDone_ = 0;   // the fence value the last upload list signals
    std::vector<KeptStaging> staging_;
    DescriptorHeap shaderViews_, samplers_, cpuViews_, renderTargets_, depthTargets_;
};

}  // namespace argon::gfx::d3d12
