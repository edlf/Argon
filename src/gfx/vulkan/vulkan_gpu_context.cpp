#include "gfx/vulkan/vulkan_gpu_context.hpp"
#include "gfx/vulkan/vulkan_formats.hpp"
#include <algorithm>
#include <cstring>

namespace
{

bool deviceHasExtension(VkPhysicalDevice gpu, const char *name)
{
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> exts(count);
    vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, exts.data());
    for (const VkExtensionProperties &extension : exts)
    {
        if (std::strcmp(extension.extensionName, name) == 0)
        {
            return true;
        }
    }
    return false;
}

VKAPI_ATTR VkBool32 VKAPI_CALL printValidationMessage(VkDebugUtilsMessageSeverityFlagBitsEXT sev, VkDebugUtilsMessageTypeFlagsEXT,
                                                      const VkDebugUtilsMessengerCallbackDataEXT *callbackData, void *)
{
    if (sev >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        std::fprintf(stderr, "[vk] %s\n", callbackData->pMessage);
    }
    return VK_FALSE;
}

std::vector<VkPhysicalDevice> physicalDevices(VkInstance instance)
{
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    std::vector<VkPhysicalDevice> gpus(count);
    vkEnumeratePhysicalDevices(instance, &count, gpus.data());
    return gpus;
}

bool meetsRequirements(VkPhysicalDevice gpu)
{
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(gpu, &properties);
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(gpu, &features);
    // independentBlend: the OIT targets' two blends (render\order_independent_translucency.hpp)
    return features.textureCompressionBC && features.independentBlend && properties.apiVersion >= VK_API_VERSION_1_3;
}

} // namespace

// ------------------------------------------------------------------ set-up
void GpuContext::create(GLFWwindow *window, bool validate)
{
    createInstance(validate);
    if (validate)
    {
        createDebugMessenger();
    }
    VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));
    pickPhysicalDevice();
    createDevice();
    createCommandPool();
}

void GpuContext::createInstance(bool validate)
{
    uint32_t count = 0;
    const char **glfwExt = glfwGetRequiredInstanceExtensions(&count);
    std::vector<const char *> exts(glfwExt, glfwExt + count);
    std::vector<const char *> layers;
    if (validate)
    {
        exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "Argon";
    app.apiVersion = VK_API_VERSION_1_3;
    VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &app;
    ici.enabledExtensionCount = uint32_t(exts.size());
    ici.ppEnabledExtensionNames = exts.data();
    ici.enabledLayerCount = uint32_t(layers.size());
    ici.ppEnabledLayerNames = layers.data();
    VK_CHECK(vkCreateInstance(&ici, nullptr, &instance));
}

void GpuContext::createDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT mi{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    mi.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    mi.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    mi.pfnUserCallback = printValidationMessage;
    auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (fn)
    {
        fn(instance, &mi, nullptr, &messenger);
    }
}

// a Vulkan 1.3 GPU with BC textures and a queue that draws and presents; discrete GPUs first
void GpuContext::pickPhysicalDevice()
{
    int bestScore = -1;
    for (VkPhysicalDevice candidate : physicalDevices(instance))
    {
        if (!meetsRequirements(candidate))
        {
            continue;
        }
        uint32_t family = 0;
        const int score = queueFamilyScore(candidate, family);
        if (score <= bestScore)
        {
            continue;
        }
        bestScore = score;
        physical = candidate;
        queueFamily = family;
    }
    if (!physical)
    {
        throw std::runtime_error("no Vulkan 1.3 GPU with BC texture support found");
    }
    vkGetPhysicalDeviceProperties(physical, &properties);
}

// -1: no queue family draws and presents to the surface
int GpuContext::queueFamilyScore(VkPhysicalDevice candidate, uint32_t &family) const
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, &count, families.data());
    for (uint32_t i = 0; i < count; ++i)
    {
        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface, &present);
        if (!(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) || !present)
        {
            continue;
        }
        VkPhysicalDeviceProperties candidateProperties;
        vkGetPhysicalDeviceProperties(candidate, &candidateProperties);
        family = i;
        return candidateProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 2 : 1;
    }
    return -1;
}

void GpuContext::createDevice()
{
    VkPhysicalDeviceFeatures have;
    vkGetPhysicalDeviceFeatures(physical, &have);
    anisotropy = have.samplerAnisotropy;
    wireframeOk = have.fillModeNonSolid;
    occlusionPrecise = have.occlusionQueryPrecise;
    float priority = 1.f;
    VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = queueFamily;
    qci.queueCount = 1;
    qci.pQueuePriorities = &priority;
    VkPhysicalDeviceFeatures features{};
    features.textureCompressionBC = VK_TRUE;
    features.independentBlend = VK_TRUE;
    features.samplerAnisotropy = anisotropy;
    features.fillModeNonSolid = wireframeOk;
    features.occlusionQueryPrecise = occlusionPrecise;
    std::vector<const char *> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    memoryBudgetOk = deviceHasExtension(physical, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
    if (memoryBudgetOk)
    {
        extensions.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
    }
    VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = uint32_t(extensions.size());
    dci.ppEnabledExtensionNames = extensions.data();
    dci.pEnabledFeatures = &features;
    VK_CHECK(vkCreateDevice(physical, &dci, nullptr, &device));
    vkGetDeviceQueue(device, queueFamily, 0, &queue);
}

void GpuContext::createCommandPool()
{
    VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    cp.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cp.queueFamilyIndex = queueFamily;
    VK_CHECK(vkCreateCommandPool(device, &cp, nullptr, &cmdPool));
}

void GpuContext::destroy()
{
    if (!instance)
    {
        return;
    }
    if (device)
    {
        vkDestroyCommandPool(device, cmdPool, nullptr);
        vkDestroyDevice(device, nullptr);
    }
    if (surface)
    {
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }
    if (messenger)
    {
        auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (fn)
        {
            fn(instance, messenger, nullptr);
        }
    }
    vkDestroyInstance(instance, nullptr);
    device = VK_NULL_HANDLE;
    surface = VK_NULL_HANDLE;
    messenger = VK_NULL_HANDLE;
    instance = VK_NULL_HANDLE;
}

// ------------------------------------------------------------------ memory and buffers
std::optional<uint32_t> GpuContext::findMemoryType(uint32_t bits, VkMemoryPropertyFlags props) const
{
    VkPhysicalDeviceMemoryProperties mp;
    vkGetPhysicalDeviceMemoryProperties(physical, &mp);
    for (uint32_t i = 0; i < mp.memoryTypeCount; ++i)
    {
        if ((bits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & props) == props)
        {
            return i;
        }
    }
    return std::nullopt;
}

uint32_t GpuContext::memoryType(uint32_t bits, VkMemoryPropertyFlags props) const
{
    const std::optional<uint32_t> type = findMemoryType(bits, props);
    if (!type)
    {
        throw std::runtime_error("no suitable memory type");
    }
    return *type;
}

VkDeviceMemory GpuContext::allocateImageMemory(VkImage image) const
{
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(device, image, &req);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = memoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkDeviceMemory memory;
    VK_CHECK(vkAllocateMemory(device, &ai, nullptr, &memory));
    VK_CHECK(vkBindImageMemory(device, image, memory, 0));
    return memory;
}

static Buffer createUnboundBuffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage)
{
    Buffer buffer;
    buffer.size = size;
    VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bi.size = std::max<VkDeviceSize>(size, 16);
    bi.usage = usage;
    VK_CHECK(vkCreateBuffer(device, &bi, nullptr, &buffer.buf));
    return buffer;
}

static void bindBufferMemory(VkDevice device, Buffer &buffer, VkDeviceSize bytes, uint32_t memoryTypeIndex)
{
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize = bytes;
    ai.memoryTypeIndex = memoryTypeIndex;
    VK_CHECK(vkAllocateMemory(device, &ai, nullptr, &buffer.mem));
    VK_CHECK(vkBindBufferMemory(device, buffer.buf, buffer.mem, 0));
}

Buffer GpuContext::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props) const
{
    Buffer buffer = createUnboundBuffer(device, size, usage);
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(device, buffer.buf, &req);
    bindBufferMemory(device, buffer, req.size, memoryType(req.memoryTypeBits, props));
    return buffer;
}

Buffer GpuContext::createHostBuffer(VkDeviceSize size, VkBufferUsageFlags usage) const
{
    return createBuffer(size, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

Buffer GpuContext::createReadBackBuffer(VkDeviceSize size) const
{
    Buffer buffer = createUnboundBuffer(device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(device, buffer.buf, &req);
    const std::optional<uint32_t> cached = findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    const uint32_t type = cached ? *cached : memoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    bindBufferMemory(device, buffer, req.size, type);
    return buffer;
}

void GpuContext::destroyBuffer(Buffer &buffer) const
{
    if (buffer.buf)
    {
        vkDestroyBuffer(device, buffer.buf, nullptr);
    }
    if (buffer.mem)
    {
        vkFreeMemory(device, buffer.mem, nullptr);
    }
    buffer = {};
}

Buffer GpuContext::createStaging(const void *data, VkDeviceSize size) const
{
    Buffer staging = createHostBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    void *mapped;
    vkMapMemory(device, staging.mem, 0, size, 0, &mapped);
    std::memcpy(mapped, data, size_t(size));
    vkUnmapMemory(device, staging.mem);
    return staging;
}

Buffer GpuContext::uploadBuffer(const void *data, VkDeviceSize size, VkBufferUsageFlags usage)
{
    Buffer staging = createStaging(data, size);
    Buffer buffer = createBuffer(size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    const VkBufferCopy region{0, 0, size};
    oneShot([&](VkCommandBuffer commandBuffer) { vkCmdCopyBuffer(commandBuffer, staging.buf, buffer.buf, 1, &region); });
    releaseStaging(staging);
    return buffer;
}

// ------------------------------------------------------------------ textures and shaders
void GpuContext::transition(VkCommandBuffer commandBuffer, VkImage image, const ImageTransition &change)
{
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.oldLayout = change.from;
    barrier.newLayout = change.to;
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, change.mips, 0, change.layers};
    barrier.srcAccessMask = change.srcAccess;
    barrier.dstAccessMask = change.dstAccess;
    vkCmdPipelineBarrier(commandBuffer, change.srcStage, change.dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

GpuTexture GpuContext::createTexture(const argon::TextureData &texture)
{
    GpuTexture tx;
    tx.alpha = texture.hasAlpha;
    tx.cutout = texture.looksLikeCutout();
    tx.cube = texture.cube;
    tx.layers = texture.layers;
    tx.format = argon::gfx::vulkan::formatOf(texture.format);
    tx.levels = uint32_t(texture.levels.size());
    tx.image = createTextureImage(texture);
    tx.mem = allocateImageMemory(tx.image);
    Buffer staging = createStaging(texture.bytes.data(), texture.bytes.size());
    oneShot([&](VkCommandBuffer commandBuffer) { copyStagingToTexture(commandBuffer, staging, texture, tx.image); });
    releaseStaging(staging);
    tx.view = createTextureView(texture, tx.image);
    tx.valid = true;
    return tx;
}

VkImage GpuContext::createTextureImage(const argon::TextureData &texture) const
{
    VkImageCreateInfo ii{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ii.flags = texture.cube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.format = argon::gfx::vulkan::formatOf(texture.format);
    ii.extent = {texture.width, texture.height, 1};
    ii.mipLevels = uint32_t(texture.levels.size());
    ii.arrayLayers = texture.layers;
    ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling = VK_IMAGE_TILING_OPTIMAL;
    ii.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    VkImage image;
    VK_CHECK(vkCreateImage(device, &ii, nullptr, &image));
    return image;
}

void GpuContext::copyStagingToTexture(VkCommandBuffer commandBuffer, const Buffer &staging, const argon::TextureData &texture, VkImage image) const
{
    const uint32_t mips = uint32_t(texture.levels.size());
    transition(commandBuffer,
               image,
               {VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                0,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                mips,
                texture.layers});
    std::vector<VkBufferImageCopy> regions;
    for (uint32_t layer = 0; layer < texture.layers; ++layer)
    {
        for (uint32_t i = 0; i < mips; ++i)
        {
            VkBufferImageCopy region{};
            region.bufferOffset = layer * texture.layerStride + texture.levels[i].offset;
            region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i, layer, 1};
            region.imageExtent = {texture.levels[i].w, texture.levels[i].h, 1};
            regions.push_back(region);
        }
    }
    vkCmdCopyBufferToImage(commandBuffer, staging.buf, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, uint32_t(regions.size()), regions.data());
    transition(commandBuffer,
               image,
               {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                mips,
                texture.layers});
}

VkImageView GpuContext::createTextureView(const argon::TextureData &texture, VkImage image) const
{
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vi.image = image;
    vi.viewType = texture.cube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    vi.format = argon::gfx::vulkan::formatOf(texture.format);
    vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, uint32_t(texture.levels.size()), 0, texture.layers};
    VkImageView view;
    VK_CHECK(vkCreateImageView(device, &vi, nullptr, &view));
    return view;
}

void GpuContext::destroyTexture(GpuTexture &texture) const
{
    if (texture.view)
    {
        vkDestroyImageView(device, texture.view, nullptr);
    }
    if (texture.image)
    {
        vkDestroyImage(device, texture.image, nullptr);
    }
    if (texture.mem)
    {
        vkFreeMemory(device, texture.mem, nullptr);
    }
    texture = {};
}

VkShaderModule GpuContext::createShader(const uint32_t *code, size_t bytes) const
{
    VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    ci.codeSize = bytes;
    ci.pCode = code;
    VkShaderModule module;
    VK_CHECK(vkCreateShaderModule(device, &ci, nullptr, &module));
    return module;
}

// ------------------------------------------------------------------ command buffers and upload batches
VkCommandBuffer GpuContext::allocateCommandBuffer() const
{
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool = cmdPool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    VK_CHECK(vkAllocateCommandBuffers(device, &ai, &commandBuffer));
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &bi));
    return commandBuffer;
}

VkCommandBuffer GpuContext::beginOneShot() const
{
    return allocateCommandBuffer();
}

void GpuContext::endOneShot(VkCommandBuffer commandBuffer) const
{
    vkEndCommandBuffer(commandBuffer);
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &commandBuffer;
    VK_CHECK(vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE));
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer);
}

void GpuContext::beginUploadBatch()
{
    freeFinishedUploads();
    openUploads_.cmd = allocateCommandBuffer();
}

// the buffer copies' writes made visible to every later read (the images carry their own barriers)
void GpuContext::submitUploadBatch()
{
    VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    mb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    mb.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    vkCmdPipelineBarrier(openUploads_.cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
    VK_CHECK(vkEndCommandBuffer(openUploads_.cmd));
    VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VK_CHECK(vkCreateFence(device, &fi, nullptr, &openUploads_.fence));
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &openUploads_.cmd;
    VK_CHECK(vkQueueSubmit(queue, 1, &si, openUploads_.fence));
    submittedUploads_.push_back(std::move(openUploads_));
    openUploads_ = UploadBatch{};
}

void GpuContext::freeFinishedUploads()
{
    for (size_t i = 0; i < submittedUploads_.size();)
    {
        UploadBatch &batch = submittedUploads_[i];
        if (vkGetFenceStatus(device, batch.fence) != VK_SUCCESS)
        {
            ++i;
            continue;
        }
        for (Buffer &staging : batch.staging)
        {
            destroyBuffer(staging);
        }
        vkDestroyFence(device, batch.fence, nullptr);
        vkFreeCommandBuffers(device, cmdPool, 1, &batch.cmd);
        submittedUploads_.erase(submittedUploads_.begin() + std::ptrdiff_t(i));
    }
}

void GpuContext::waitForUploads()
{
    for (UploadBatch &batch : submittedUploads_)
    {
        vkWaitForFences(device, 1, &batch.fence, VK_TRUE, UINT64_MAX);
    }
    freeFinishedUploads();
}

void GpuContext::releaseStaging(Buffer &staging)
{
    if (!openUploads_.cmd)
    {
        destroyBuffer(staging);
        return;
    }
    openUploads_.staging.push_back(staging);
    staging = Buffer{};
}
