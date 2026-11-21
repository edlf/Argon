// Argon: gfx::Device over Vulkan: the renderer's descriptor set layouts (0 the four stage textures, 1 the scene UBO and
// frame copy, 2 the materials' SSBO), its pipeline layout (with one push constant range for both stages), the shader
// modules and the pipelines, all kept until destroy().
#pragma once
#include "gfx/device.hpp"
#include "gfx/vulkan/vulkan_layer_target.hpp"
#include "gfx/vulkan/vulkan_swapchain.hpp"

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

namespace argon::gfx::vulkan
{

class VulkanDevice final : public Device
{
  public:
    // the layouts; throws when the GPU's push constants are smaller than pushConstantBytes
    void create(GpuContext &gpu, const Swapchain &swapchain, uint32_t pushConstantBytes);
    void destroy();

    Pipeline createPipeline(const PipelineSpec &spec) override;
    Pipeline meshPipeline(const MeshState &state, Fill fill) override;
    void waitForIdle() override;
    void beginUploadBatch() override;
    void submitUploadBatch() override;
    bool wireframeSupported() const override;
    std::optional<uint64_t> videoMemoryInUse() const override;
    uint32_t uploadRowPitch(uint32_t width) const override
    {
        return width * 4;
    }
    BufferRef createStreamBuffer(BufferUse use, size_t bytes) override;
    BufferRef uploadBuffer(BufferUse use, std::span<const std::byte> data) override;
    void *mapped(BufferRef buffer) const override;
    void destroyBuffer(BufferRef &buffer) override;
    LayerTarget createLayerTarget(const LayerTargetSize &size) override;
    void destroyLayerTarget(LayerTarget &target) override;
    Texture layerTexture(LayerTarget target) const override;
    Texture createTexture(const argon::TextureData &data) override;
    void destroyTexture(Texture &texture) override;
    SetPool createSetPool(const SetPoolSize &size) override;
    void resetSetPool(SetPool pool) override;
    TextureSet createTextureSet(SetPool pool, const TextureSetDesc &desc) override;
    MaterialSet createMaterialSet(SetPool pool, BufferRef materials) override;
    SceneSet createSceneSet(BufferRef constants, BufferRef meshDraws, TextureRef frameCopy) override;
    void useFrameCopy(SceneSet set, TextureRef frameCopy) override;
    void useOitTargets(SceneSet set, const OitTargets &targets) override;
    QueryPool createOcclusionQueries(uint32_t count) override;
    bool readQueries(const QueryRange &range, std::span<uint64_t> results) const override;
    bool preciseOcclusion() const override;

    // a view the frame code made (a frame copy), sampled through texture sets; the caller keeps destroying it
    TextureRef adoptView(VkImageView view);
    VkSampler sampler(SamplerMode mode) const
    {
        return samplers_[size_t(mode)];
    }
    VkImageView viewOf(TextureRef texture) const;
    // a texture createTexture made
    const GpuTexture &gpuTexture(TextureRef texture) const;

    GpuContext &gpu() const
    {
        return *gpu_;
    }
    VkPipelineLayout pipelineLayout() const
    {
        return pipelineLayout_;
    }

  private:
    struct BufferMemory
    {
        Buffer buffer;
        void *mapped = nullptr;
    };

    BufferRef keep(const BufferMemory &memory);
    void createSamplers();
    VkDescriptorSet allocateSet(SetPool pool, VkDescriptorSetLayout layout) const;

    struct ShaderPair
    {
        VkShaderModule vertex, fragment;
    };

    ShaderPair shadersOf(ShaderProgram program);
    VkShaderModule moduleOf(const uint32_t *code, size_t bytes);
    VkPipeline build(const PipelineSpec &spec, VkGraphicsPipelineCreateInfo &info) const;

    GpuContext *gpu_ = nullptr;
    const Swapchain *swapchain_ = nullptr;
    VkRenderPass layerPass_ = VK_NULL_HANDLE;              // the RenderTarget::Layer pipelines'
    VkDescriptorSetLayout textureLayout_ = VK_NULL_HANDLE, sceneLayout_ = VK_NULL_HANDLE, materialLayout_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    std::map<const uint32_t *, VkShaderModule> modules_;   // by SPIR-V blob
    std::vector<VkPipeline> pipelines_;
    std::map<uint32_t, Pipeline> meshPipelines_;           // MeshState::pipelineKey() | wireframe << 20
    std::unordered_map<uint64_t, BufferMemory> buffers_;   // by BufferRef
    std::unordered_map<uint64_t, GpuTexture> textures_;    // by TextureRef (its view)
    std::unordered_map<uint64_t, VkImageView> adopted_;    // views the frame code owns
    std::vector<VkDescriptorPool> pools_;
    VkDescriptorPool scenePool_ = VK_NULL_HANDLE;          // the scene sets'
    std::vector<VkQueryPool> queryPools_;
    std::vector<std::unique_ptr<VulkanLayerTarget>> layerTargets_;   // by address (their handles)
    VkSampler samplers_[4]{};                              // by SamplerMode
};

}  // namespace argon::gfx::vulkan
