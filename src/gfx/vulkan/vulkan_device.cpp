// Argon: gfx::Device over Vulkan (gfx\vulkan\vulkan_device.hpp).
#include "gfx/vulkan/vulkan_device.hpp"
#include "gfx/vulkan/vulkan_formats.hpp"
#include "gfx/vulkan/vulkan_handles.hpp"
#include "gfx/vulkan/vulkan_shaders.hpp"
#include "model/cmm.hpp"
#include "track/marker_lines.hpp"

#include <algorithm>
#include <array>
#include <string>

namespace argon::gfx::vulkan
{
namespace
{
const VkShaderStageFlags kVertexAndFragment = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
const uint32_t kWireframeKey = 1u << 20;

template <size_t N> constexpr std::pair<const uint32_t *, size_t> blob(const uint32_t (&code)[N])
{
    return {code, sizeof code};
}

struct IsLayerTarget
{
    const VulkanLayerTarget *target;

    bool operator()(const std::unique_ptr<VulkanLayerTarget> &kept) const
    {
        return kept.get() == target;
    }
};

// the attributes the program's vertex shader reads (vertexInputsOf)
std::vector<VkVertexInputAttributeDescription> vertexAttributes(const PipelineSpec &spec)
{
    if (spec.topology == Topology::Lines)
    {
        return {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12}};
    }
    const VkVertexInputAttributeDescription all[5] = {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
                                                      {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12},
                                                      {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 24},
                                                      {3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 40},
                                                      {4, 0, VK_FORMAT_R8G8B8A8_UNORM, 56}};
    std::vector<VkVertexInputAttributeDescription> read;
    for (uint32_t location = 0; location < 5; ++location)
    {
        if (vertexInputsOf(spec.program) & (1u << location))
        {
            read.push_back(all[location]);
        }
    }
    return read;
}

VkPrimitiveTopology primitivesOf(Topology topology)
{
    if (topology == Topology::OcclusionPoints)
    {
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    }
    return topology == Topology::Lines ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

// colour blended by the mesh state; alpha kept (the game's framebuffer alpha is unused)
VkPipelineColorBlendAttachmentState colourBlend(const PipelineSpec &spec)
{
    VkPipelineColorBlendAttachmentState cb{};
    cb.colorWriteMask = spec.topology == Topology::OcclusionPoints ? 0u : 0xfu;
    if (!spec.blend || !spec.blend->blend)
    {
        return cb;
    }
    cb.blendEnable = VK_TRUE;
    cb.srcColorBlendFactor = blendFactorOf(spec.blend->src);
    cb.dstColorBlendFactor = blendFactorOf(spec.blend->dst);
    cb.colorBlendOp = blendOpOf(spec.blend->op);
    cb.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    cb.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cb.alphaBlendOp = VK_BLEND_OP_ADD;
    return cb;
}

// RenderTarget::Oit (pipeline_spec.hpp): the accumulated colour and alpha add up, the revealage is multiplied by
// 1 - alpha
std::array<VkPipelineColorBlendAttachmentState, kOitTargetCount> oitBlends()
{
    std::array<VkPipelineColorBlendAttachmentState, kOitTargetCount> blends{};
    for (VkPipelineColorBlendAttachmentState &blend : blends)
    {
        blend.blendEnable = VK_TRUE;
        blend.colorWriteMask = 0xfu;
        blend.colorBlendOp = blend.alphaBlendOp = VK_BLEND_OP_ADD;
    }
    blends[0].srcColorBlendFactor = blends[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blends[0].srcAlphaBlendFactor = blends[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blends[1].srcColorBlendFactor = blends[1].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blends[1].dstColorBlendFactor = blends[1].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    return blends;
}

VkRenderPass renderPassOf(RenderTarget target, const Swapchain &swapchain, VkRenderPass layerPass)
{
    switch (target)
    {
    case RenderTarget::Layer:
        return layerPass;
    case RenderTarget::Oit:
        return swapchain.oitPass;
    case RenderTarget::Scene:
        break;
    }
    return swapchain.renderPass;
}

VkDescriptorSetLayout createSingleBindingLayout(VkDevice device, VkDescriptorType type, uint32_t count, VkShaderStageFlags stages)
{
    const VkDescriptorSetLayoutBinding binding{0, type, count, stages};
    VkDescriptorSetLayoutCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    ci.bindingCount = 1;
    ci.pBindings = &binding;
    VkDescriptorSetLayout layout;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &ci, nullptr, &layout));
    return layout;
}

// the scene set: binding 0 the scene UBO, binding 1 the last frame's copy (the "Hud Lens" effect's echo texture),
// 2 / 3 the OIT targets (oit_composite.glsl; written only while they are kept), 4 the frame's mesh draw records
// (mesh_draws.glsl)
VkDescriptorSetLayout createSceneLayout(VkDevice device)
{
    const VkDescriptorSetLayoutBinding bindings[5] = {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, kVertexAndFragment},
                                                      {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
                                                      {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
                                                      {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
                                                      {4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, kVertexAndFragment}};
    VkDescriptorSetLayoutCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    ci.bindingCount = 5;
    ci.pBindings = bindings;
    VkDescriptorSetLayout layout;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &ci, nullptr, &layout));
    return layout;
}

VkBufferUsageFlags usageOf(BufferUse use)
{
    switch (use)
    {
    case BufferUse::Index:
        return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    case BufferUse::Storage:
        return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    case BufferUse::Uniform:
        return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    case BufferUse::Upload:
        return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    case BufferUse::Vertex:
        break;
    }
    return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
}

}  // namespace

void VulkanDevice::create(GpuContext &gpu, const Swapchain &swapchain, uint32_t pushConstantBytes)
{
    gpu_ = &gpu;
    swapchain_ = &swapchain;
    textureLayout_ = createSingleBindingLayout(gpu.device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, VK_SHADER_STAGE_FRAGMENT_BIT);
    sceneLayout_ = createSceneLayout(gpu.device);
    materialLayout_ = createSingleBindingLayout(gpu.device, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, kVertexAndFragment);
    if (gpu.properties.limits.maxPushConstantsSize < pushConstantBytes)
    {
        throw std::runtime_error("the GPU's push constants hold " + std::to_string(gpu.properties.limits.maxPushConstantsSize) +
                                 " bytes; the mesh draws need " + std::to_string(pushConstantBytes));
    }
    const VkDescriptorSetLayout sets[3] = {textureLayout_, sceneLayout_, materialLayout_};
    const VkPushConstantRange push{kVertexAndFragment, 0, pushConstantBytes};
    VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pl.setLayoutCount = 3;
    pl.pSetLayouts = sets;
    pl.pushConstantRangeCount = 1;
    pl.pPushConstantRanges = &push;
    VK_CHECK(vkCreatePipelineLayout(gpu.device, &pl, nullptr, &pipelineLayout_));
    createSamplers();
    layerPass_ = createLayerPass(gpu.device);
}

// by SamplerMode: bit 0 clamps U, bit 1 clamps V
void VulkanDevice::createSamplers()
{
    VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    si.magFilter = si.minFilter = VK_FILTER_LINEAR;
    si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    si.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    si.maxLod = 16.f;
    si.anisotropyEnable = gpu_->anisotropy;
    si.maxAnisotropy = gpu_->anisotropy ? std::min(8.f, gpu_->properties.limits.maxSamplerAnisotropy) : 1.f;
    for (int k = 0; k < 4; ++k)
    {
        si.addressModeU = (k & 1) ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
        si.addressModeV = (k & 2) ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VK_CHECK(vkCreateSampler(gpu_->device, &si, nullptr, &samplers_[k]));
    }
}

void VulkanDevice::destroy()
{
    if (!gpu_)
    {
        return;
    }
    const VkDevice device = gpu_->device;
    for (auto &[handle, memory] : buffers_)
    {
        gpu_->destroyBuffer(memory.buffer);   // freeing the memory also unmaps it
    }
    buffers_.clear();
    for (auto &[handle, texture] : textures_)
    {
        gpu_->destroyTexture(texture);
    }
    textures_.clear();
    adopted_.clear();
    for (const VkDescriptorPool pool : pools_)
    {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }
    pools_.clear();
    if (scenePool_)
    {
        vkDestroyDescriptorPool(device, scenePool_, nullptr);
        scenePool_ = VK_NULL_HANDLE;
    }
    for (const VkQueryPool pool : queryPools_)
    {
        vkDestroyQueryPool(device, pool, nullptr);
    }
    queryPools_.clear();
    for (const std::unique_ptr<VulkanLayerTarget> &target : layerTargets_)
    {
        argon::gfx::vulkan::destroyLayerTarget(device, *target);
    }
    layerTargets_.clear();
    vkDestroyRenderPass(device, layerPass_, nullptr);
    layerPass_ = VK_NULL_HANDLE;
    for (VkSampler &sampler : samplers_)
    {
        vkDestroySampler(device, sampler, nullptr);
        sampler = VK_NULL_HANDLE;
    }
    for (const VkPipeline pipeline : pipelines_)
    {
        vkDestroyPipeline(device, pipeline, nullptr);
    }
    pipelines_.clear();
    meshPipelines_.clear();
    for (const auto &[code, module] : modules_)
    {
        vkDestroyShaderModule(device, module, nullptr);
    }
    modules_.clear();
    vkDestroyPipelineLayout(device, pipelineLayout_, nullptr);
    for (const VkDescriptorSetLayout layout : {textureLayout_, sceneLayout_, materialLayout_})
    {
        vkDestroyDescriptorSetLayout(device, layout, nullptr);
    }
}

VkShaderModule VulkanDevice::moduleOf(const uint32_t *code, size_t bytes)
{
    const auto known = modules_.find(code);
    if (known != modules_.end())
    {
        return known->second;
    }
    return modules_[code] = gpu_->createShader(code, bytes);
}

VulkanDevice::ShaderPair VulkanDevice::shadersOf(ShaderProgram program)
{
    std::pair<const uint32_t *, size_t> vertex = blob(kMeshVert), fragment = blob(kMeshFrag);
    switch (program)
    {
    case ShaderProgram::Mesh:
        break;
    case ShaderProgram::Line:
        vertex = blob(kLineVert);
        fragment = blob(kLineFrag);
        break;
    case ShaderProgram::Water:
        vertex = blob(kWaterVert);
        fragment = blob(kWaterFrag);
        break;
    case ShaderProgram::Trail:
        vertex = blob(kTrailVert);
        fragment = blob(kTrailFrag);
        break;
    case ShaderProgram::Spray:
        vertex = blob(kTrailVert);
        fragment = blob(kSprayFrag);
        break;
    case ShaderProgram::QuadSprite:
        vertex = blob(kQuadSpriteVert);
        fragment = blob(kQuadSpriteFrag);
        break;
    case ShaderProgram::Occlusion:
        vertex = blob(kOcclVert);
        fragment = blob(kOcclFrag);
        break;
    case ShaderProgram::ShadowCast:
        fragment = blob(kShadowCastFrag);
        break;
    case ShaderProgram::ShadowSolid:
        fragment = blob(kShadowSolidFrag);
        break;
    case ShaderProgram::ShadowReceive:
        vertex = blob(kShadowVert);
        fragment = blob(kShadowFrag);
        break;
    case ShaderProgram::Fxaa:
        vertex = blob(kFullscreenVert);
        fragment = blob(kFxaaFrag);
        break;
    case ShaderProgram::SmaaEdges:
        vertex = blob(kFullscreenVert);
        fragment = blob(kSmaaEdgesFrag);
        break;
    case ShaderProgram::SmaaWeights:
        vertex = blob(kFullscreenVert);
        fragment = blob(kSmaaWeightsFrag);
        break;
    case ShaderProgram::SmaaBlend:
        vertex = blob(kFullscreenVert);
        fragment = blob(kSmaaBlendFrag);
        break;
    case ShaderProgram::MeshOit:
        fragment = blob(kMeshOitFrag);
        break;
    case ShaderProgram::MeshOitOpaque:
        fragment = blob(kMeshOitOpaqueFrag);
        break;
    case ShaderProgram::OitComposite:
        vertex = blob(kFullscreenVert);
        fragment = swapchain_->multisampled() ? blob(kOitCompositeMsFrag) : blob(kOitCompositeFrag);
        break;
    }
    return ShaderPair{moduleOf(vertex.first, vertex.second), moduleOf(fragment.first, fragment.second)};
}

// depth test / write, cull mode and front face are dynamic (Vulkan 1.3 core) and set per draw
Pipeline VulkanDevice::createPipeline(const PipelineSpec &spec)
{
    const ShaderPair shaders = shadersOf(spec.program);
    const bool lines = spec.topology == Topology::Lines;
    const VkPipelineShaderStageCreateInfo stages[2] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, shaders.vertex, "main"},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, shaders.fragment, "main"}};
    const VkVertexInputBindingDescription binding{0, lines ? uint32_t(sizeof(MarkerVertex)) : uint32_t(sizeof(Vertex)), VK_VERTEX_INPUT_RATE_VERTEX};
    const auto attributes = vertexAttributes(spec);
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vi.vertexBindingDescriptionCount = isFullscreenProgram(spec.program) ? 0u : 1u;
    vi.pVertexBindingDescriptions = &binding;
    vi.vertexAttributeDescriptionCount = uint32_t(attributes.size());
    vi.pVertexAttributeDescriptions = attributes.data();
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = primitivesOf(spec.topology);
    VkGraphicsPipelineCreateInfo gp{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    gp.stageCount = 2;
    gp.pStages = stages;
    gp.pVertexInputState = &vi;
    gp.pInputAssemblyState = &ia;
    const VkPipeline pipeline = build(spec, gp);
    pipelines_.push_back(pipeline);
    return pipelineOf(pipeline);
}

// the fixed-function state around the shaders and vertex input already in `info`
VkPipeline VulkanDevice::build(const PipelineSpec &spec, VkGraphicsPipelineCreateInfo &info) const
{
    const bool shadow = spec.target == RenderTarget::Layer;
    VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount = vp.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = spec.fill == Fill::Wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.f;
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = shadow ? VK_SAMPLE_COUNT_1_BIT : swapchain_->samples;
    ms.alphaToCoverageEnable = spec.blend && spec.blend->a2c ? VK_TRUE : VK_FALSE;
    VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    ds.depthTestEnable = VK_TRUE;    // dynamic
    ds.depthWriteEnable = VK_TRUE;   // dynamic
    ds.depthCompareOp = compareOf(spec.depth);
    const VkPipelineColorBlendAttachmentState cb = colourBlend(spec);
    const auto oit = oitBlends();
    VkPipelineColorBlendStateCreateInfo cbs{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cbs.attachmentCount = spec.target == RenderTarget::Oit ? kOitTargetCount : 1u;
    cbs.pAttachments = spec.target == RenderTarget::Oit ? oit.data() : &cb;
    const VkDynamicState dynamic[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                      VK_DYNAMIC_STATE_SCISSOR,
                                      VK_DYNAMIC_STATE_CULL_MODE,
                                      VK_DYNAMIC_STATE_FRONT_FACE,
                                      VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
                                      VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE};
    VkPipelineDynamicStateCreateInfo dy{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dy.dynamicStateCount = 6;
    dy.pDynamicStates = dynamic;
    info.pViewportState = &vp;
    info.pRasterizationState = &rs;
    info.pMultisampleState = &ms;
    info.pDepthStencilState = &ds;
    info.pColorBlendState = &cbs;
    info.pDynamicState = &dy;
    info.layout = pipelineLayout_;
    info.renderPass = renderPassOf(spec.target, *swapchain_, layerPass_);
    VkPipeline pipeline;
    VK_CHECK(vkCreateGraphicsPipelines(gpu_->device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline));
    return pipeline;
}

Pipeline VulkanDevice::meshPipeline(const MeshState &state, Fill fill)
{
    const uint32_t key = state.pipelineKey() | (fill == Fill::Wireframe ? kWireframeKey : 0u);
    const auto known = meshPipelines_.find(key);
    if (known != meshPipelines_.end())
    {
        return known->second;
    }
    PipelineSpec spec;
    spec.fill = fill;
    spec.blend = state.blend || state.a2c ? &state : nullptr;
    return meshPipelines_[key] = createPipeline(spec);
}

void VulkanDevice::waitForIdle()
{
    vkDeviceWaitIdle(gpu_->device);
    gpu_->freeFinishedUploads();
}

void VulkanDevice::beginUploadBatch()
{
    gpu_->beginUploadBatch();
}

void VulkanDevice::submitUploadBatch()
{
    gpu_->submitUploadBatch();
}

bool VulkanDevice::wireframeSupported() const
{
    return gpu_->wireframeOk;
}

// VK_EXT_memory_budget's usage of the device-local heaps
std::optional<uint64_t> VulkanDevice::videoMemoryInUse() const
{
    if (!gpu_->memoryBudgetOk)
    {
        return std::nullopt;
    }
    VkPhysicalDeviceMemoryBudgetPropertiesEXT budget{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT};
    VkPhysicalDeviceMemoryProperties2 props{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};
    props.pNext = &budget;
    vkGetPhysicalDeviceMemoryProperties2(gpu_->physical, &props);
    uint64_t used = 0;
    for (uint32_t i = 0; i < props.memoryProperties.memoryHeapCount; ++i)
    {
        if (props.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            used += budget.heapUsage[i];
        }
    }
    return used;
}

BufferRef VulkanDevice::keep(const BufferMemory &memory)
{
    const BufferRef buffer = bufferOf(memory.buffer.buf);
    buffers_[buffer.value] = memory;
    return buffer;
}

BufferRef VulkanDevice::createStreamBuffer(BufferUse use, size_t bytes)
{
    BufferMemory memory;
    memory.buffer = gpu_->createHostBuffer(VkDeviceSize(bytes), usageOf(use));
    VK_CHECK(vkMapMemory(gpu_->device, memory.buffer.mem, 0, VkDeviceSize(bytes), 0, &memory.mapped));
    return keep(memory);
}

BufferRef VulkanDevice::uploadBuffer(BufferUse use, std::span<const std::byte> data)
{
    BufferMemory memory;
    memory.buffer = gpu_->uploadBuffer(data.data(), VkDeviceSize(data.size()), usageOf(use));
    return keep(memory);
}

void *VulkanDevice::mapped(BufferRef buffer) const
{
    const auto known = buffers_.find(buffer.value);
    return known == buffers_.end() ? nullptr : known->second.mapped;
}

void VulkanDevice::destroyBuffer(BufferRef &buffer)
{
    const auto known = buffers_.find(buffer.value);
    if (known != buffers_.end())
    {
        gpu_->destroyBuffer(known->second.buffer);
        buffers_.erase(known);
    }
    buffer = {};
}

Texture VulkanDevice::createTexture(const argon::TextureData &data)
{
    const GpuTexture made = gpu_->createTexture(data);
    Texture texture;
    if (!made.valid)
    {
        return texture;
    }
    texture.handle = wrap<TextureRef>(made.view);
    texture.valid = true;
    texture.cube = made.cube;
    texture.layers = made.layers;
    texture.alpha = made.alpha;
    texture.cutout = made.cutout;
    texture.bump = made.bump;
    textures_[texture.handle.value] = made;
    return texture;
}

void VulkanDevice::destroyTexture(Texture &texture)
{
    const auto known = textures_.find(texture.handle.value);
    if (known != textures_.end())
    {
        gpu_->destroyTexture(known->second);
        textures_.erase(known);
    }
    texture = Texture{};
}

LayerTarget VulkanDevice::createLayerTarget(const LayerTargetSize &size)
{
    VulkanLayerTarget &target = *layerTargets_.emplace_back(std::make_unique<VulkanLayerTarget>());
    target.pass = layerPass_;
    argon::gfx::vulkan::createLayerTarget(*gpu_, size, target);
    adoptView(target.arrayView);
    return layerTargetOf(target);
}

void VulkanDevice::destroyLayerTarget(LayerTarget &target)
{
    if (!target)
    {
        return;
    }
    const auto kept = std::find_if(layerTargets_.begin(), layerTargets_.end(), IsLayerTarget{&recordOf(target)});
    if (kept != layerTargets_.end())
    {
        adopted_.erase(wrap<TextureRef>((*kept)->arrayView).value);
        argon::gfx::vulkan::destroyLayerTarget(gpu_->device, **kept);
        layerTargets_.erase(kept);
    }
    target = {};
}

Texture VulkanDevice::layerTexture(LayerTarget target) const
{
    const VulkanLayerTarget &record = recordOf(target);
    Texture texture;
    texture.handle = wrap<TextureRef>(record.arrayView);
    texture.valid = true;
    texture.layers = uint32_t(record.layerViews.size());
    return texture;
}

TextureRef VulkanDevice::adoptView(VkImageView view)
{
    const TextureRef texture = wrap<TextureRef>(view);
    adopted_[texture.value] = view;
    return texture;
}

VkImageView VulkanDevice::viewOf(TextureRef texture) const
{
    if (const auto owned = textures_.find(texture.value); owned != textures_.end())
    {
        return owned->second.view;
    }
    const auto adopted = adopted_.find(texture.value);
    if (adopted == adopted_.end())
    {
        throw std::runtime_error("a texture set names a texture the device does not know");
    }
    return adopted->second;
}

const GpuTexture &VulkanDevice::gpuTexture(TextureRef texture) const
{
    const auto owned = textures_.find(texture.value);
    if (owned == textures_.end())
    {
        throw std::runtime_error("a UI image names a texture the device did not make");
    }
    return owned->second;
}

SetPool VulkanDevice::createSetPool(const SetPoolSize &size)
{
    const VkDescriptorPoolSize sizes[2] = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * size.sets},
                                           {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, size.materialSets}};
    VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    dp.maxSets = size.sets;
    dp.poolSizeCount = 2;
    dp.pPoolSizes = sizes;
    VkDescriptorPool pool;
    VK_CHECK(vkCreateDescriptorPool(gpu_->device, &dp, nullptr, &pool));
    pools_.push_back(pool);
    return wrap<SetPool>(pool);
}

void VulkanDevice::resetSetPool(SetPool pool)
{
    vkResetDescriptorPool(gpu_->device, unwrap<VkDescriptorPool>(pool), 0);
}

VkDescriptorSet VulkanDevice::allocateSet(SetPool pool, VkDescriptorSetLayout layout) const
{
    VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    da.descriptorPool = unwrap<VkDescriptorPool>(pool);
    da.descriptorSetCount = 1;
    da.pSetLayouts = &layout;
    VkDescriptorSet set;
    VK_CHECK(vkAllocateDescriptorSets(gpu_->device, &da, &set));
    return set;
}

TextureSet VulkanDevice::createTextureSet(SetPool pool, const TextureSetDesc &desc)
{
    const VkDescriptorSet set = allocateSet(pool, textureLayout_);
    VkDescriptorImageInfo info[4];
    for (size_t i = 0; i < 4; ++i)
    {
        info[i] = {sampler(desc.samplers[i]), viewOf(desc.textures[i]), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    }
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = set;
    write.descriptorCount = 4;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = info;
    vkUpdateDescriptorSets(gpu_->device, 1, &write, 0, nullptr);
    return textureSetOf(set);
}

MaterialSet VulkanDevice::createMaterialSet(SetPool pool, BufferRef materials)
{
    const VkDescriptorSet set = allocateSet(pool, materialLayout_);
    const VkDescriptorBufferInfo info{unwrap<VkBuffer>(materials), 0, VK_WHOLE_SIZE};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = set;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo = &info;
    vkUpdateDescriptorSets(gpu_->device, 1, &write, 0, nullptr);
    return materialSetOf(set);
}

SceneSet VulkanDevice::createSceneSet(BufferRef constants, BufferRef meshDraws, TextureRef frameCopy)
{
    if (!scenePool_)
    {
        const uint32_t kSceneSets = 32;
        const VkDescriptorPoolSize sizes[3] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, kSceneSets},
                                               {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kSceneSets * 3},
                                               {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, kSceneSets}};
        VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        dp.maxSets = kSceneSets;
        dp.poolSizeCount = 3;
        dp.pPoolSizes = sizes;
        VK_CHECK(vkCreateDescriptorPool(gpu_->device, &dp, nullptr, &scenePool_));
    }
    VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    da.descriptorPool = scenePool_;
    da.descriptorSetCount = 1;
    da.pSetLayouts = &sceneLayout_;
    VkDescriptorSet set;
    VK_CHECK(vkAllocateDescriptorSets(gpu_->device, &da, &set));
    const VkDescriptorBufferInfo constantsInfo{unwrap<VkBuffer>(constants), 0, VK_WHOLE_SIZE};
    const VkDescriptorBufferInfo drawsInfo{unwrap<VkBuffer>(meshDraws), 0, VK_WHOLE_SIZE};
    VkWriteDescriptorSet writes[2] = {{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}, {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}};
    writes[0].dstSet = set;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &constantsInfo;
    writes[1].dstSet = set;
    writes[1].dstBinding = 4;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &drawsInfo;
    vkUpdateDescriptorSets(gpu_->device, 2, writes, 0, nullptr);
    const SceneSet scene = sceneSetOf(set);
    useFrameCopy(scene, frameCopy);
    return scene;
}

// binding 1, sampled clamped
void VulkanDevice::useFrameCopy(SceneSet set, TextureRef frameCopy)
{
    const VkDescriptorImageInfo info{sampler(SamplerMode::Clamp), viewOf(frameCopy), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = unwrap<VkDescriptorSet>(set);
    write.dstBinding = 1;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &info;
    vkUpdateDescriptorSets(gpu_->device, 1, &write, 0, nullptr);
}

// bindings 2 and 3, read with texelFetch (the sampler unused)
void VulkanDevice::useOitTargets(SceneSet set, const OitTargets &targets)
{
    const VkDescriptorImageInfo infos[2] = {{sampler(SamplerMode::Clamp), viewOf(targets.accumulated), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                                            {sampler(SamplerMode::Clamp), viewOf(targets.revealage), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = unwrap<VkDescriptorSet>(set);
    write.dstBinding = 2;
    write.descriptorCount = 2;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = infos;
    vkUpdateDescriptorSets(gpu_->device, 1, &write, 0, nullptr);
}

QueryPool VulkanDevice::createOcclusionQueries(uint32_t count)
{
    VkQueryPoolCreateInfo qi{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    qi.queryType = VK_QUERY_TYPE_OCCLUSION;
    qi.queryCount = count;
    VkQueryPool pool;
    VK_CHECK(vkCreateQueryPool(gpu_->device, &qi, nullptr, &pool));
    queryPools_.push_back(pool);
    return queryPoolOf(pool);
}

bool VulkanDevice::readQueries(const QueryRange &range, std::span<uint64_t> results) const
{
    const VkResult read = vkGetQueryPoolResults(gpu_->device,
                                                unwrap<VkQueryPool>(range.pool),
                                                range.first,
                                                range.count,
                                                results.size_bytes(),
                                                results.data(),
                                                sizeof(uint64_t),
                                                VK_QUERY_RESULT_64_BIT);
    return read == VK_SUCCESS;
}

bool VulkanDevice::preciseOcclusion() const
{
    return gpu_->occlusionPrecise;
}

}  // namespace argon::gfx::vulkan
