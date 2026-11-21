// The Vulkan instance, device and queue, and the resource helpers built on them: buffers, textures, shader modules,
// one-shot command buffers and batched uploads.
#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstdio>
#include <optional>
#include <stdexcept>
#include <vector>

#include "files/dds.hpp"

#define VK_CHECK(x)                                                                                                                                            \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        VkResult r_ = (x);                                                                                                                                     \
        if (r_ != VK_SUCCESS)                                                                                                                                  \
        {                                                                                                                                                      \
            char b_[256];                                                                                                                                      \
            std::snprintf(b_, sizeof b_, "%s failed: %d (%s:%d)", #x, int(r_), __FILE__, __LINE__);                                                            \
            throw std::runtime_error(b_);                                                                                                                      \
        }                                                                                                                                                      \
    } while (0)

struct Buffer
{
    VkBuffer buf = VK_NULL_HANDLE;
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
};

struct GpuTexture
{
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    bool valid = false;
    bool cube = false;
    uint32_t layers = 1;   // animation frames
    bool alpha = false;    // format has alpha
    bool cutout = false;   // alpha looks like a stencil (used by "Auto")
    bool bump = false;     // texture table type 3: normal map
    VkFormat format = VK_FORMAT_UNDEFINED;
    uint32_t levels = 1;   // mip levels
};

// an image layout change and the accesses / stages it orders
struct ImageTransition
{
    VkImageLayout from, to;
    VkAccessFlags srcAccess, dstAccess;
    VkPipelineStageFlags srcStage, dstStage;
    uint32_t mips = 1, layers = 1;

    // a sampled image rewritten by transfers and back
    static ImageTransition sampledToTransferDst()
    {
        return {VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_ACCESS_SHADER_READ_BIT,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT};
    }
    static ImageTransition transferDstToSampled()
    {
        return {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT};
    }
    // a swapchain image the scene pass left (PRESENT_SRC) drawn on again by the resumed pass, keeping what it holds
    static ImageTransition presentToColourAttachment()
    {
        return {VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    }
    // a finished swapchain image made a copy source, and back to `after` (PRESENT_SRC, or COLOR_ATTACHMENT_OPTIMAL)
    static ImageTransition presentToTransferSrc()
    {
        return {VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_TRANSFER_READ_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT};
    }
    static ImageTransition transferSrcTo(VkImageLayout after)
    {
        if (after == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        {
            return {VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    after,
                    VK_ACCESS_TRANSFER_READ_BIT,
                    0,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};
        }
        return {VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                after,
                VK_ACCESS_TRANSFER_READ_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    }
};

class GpuContext
{
  public:
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties properties{};
    VkDevice device = VK_NULL_HANDLE;
    uint32_t queueFamily = 0;
    VkQueue queue = VK_NULL_HANDLE;
    VkCommandPool cmdPool = VK_NULL_HANDLE;
    bool anisotropy = false, wireframeOk = false;
    bool occlusionPrecise = false;   // occlusionQueryPrecise: sample counts (else any-sample booleans)
    bool memoryBudgetOk = false;     // VK_EXT_memory_budget (the FPS meter's VRAM line)

    void create(GLFWwindow *window, bool validate);
    void destroy();

    uint32_t memoryType(uint32_t bits, VkMemoryPropertyFlags props) const;
    std::optional<uint32_t> findMemoryType(uint32_t bits, VkMemoryPropertyFlags props) const;
    VkDeviceMemory allocateImageMemory(VkImage image) const;   // device local, bound to the image
    Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props) const;
    Buffer createHostBuffer(VkDeviceSize size, VkBufferUsageFlags usage) const;   // host visible + coherent
    // a copy target the CPU reads: host cached when the GPU has such memory (reads from uncached memory crawl), so
    // vkInvalidateMappedMemoryRanges before reading it
    Buffer createReadBackBuffer(VkDeviceSize size) const;
    void destroyBuffer(Buffer &buffer) const;
    Buffer uploadBuffer(const void *data, VkDeviceSize size, VkBufferUsageFlags usage);
    // 2D textures are created as 2D arrays (one layer, or one per animation frame); cubes as cubes.
    GpuTexture createTexture(const argon::TextureData &texture);
    void destroyTexture(GpuTexture &texture) const;
    VkShaderModule createShader(const uint32_t *code, size_t bytes) const;
    static void transition(VkCommandBuffer commandBuffer, VkImage image, const ImageTransition &change);

    // Uploads between beginUploadBatch and submitUploadBatch (a track load's frame slice) record into one command
    // buffer submitted without waiting; its staging buffers are freed once its fence has signalled. Later
    // submissions on the queue see the uploads (submission order + the uploads' own barriers).
    void beginUploadBatch();
    void submitUploadBatch();
    void freeFinishedUploads();
    void waitForUploads();
    void releaseStaging(Buffer &staging);   // now, or after the open batch ran

    // record and run GPU commands now (inside an upload batch: record them into it)
    template <class F> void oneShot(F &&record)
    {
        if (openUploads_.cmd)
        {
            record(openUploads_.cmd);
            return;
        }
        VkCommandBuffer commandBuffer = beginOneShot();
        record(commandBuffer);
        endOneShot(commandBuffer);
    }

  private:
    struct UploadBatch
    {
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        VkFence fence = VK_NULL_HANDLE;
        std::vector<Buffer> staging;
    };

    void createInstance(bool validate);
    void createDebugMessenger();
    void pickPhysicalDevice();
    int queueFamilyScore(VkPhysicalDevice candidate, uint32_t &family) const;
    void createDevice();
    void createCommandPool();
    Buffer createStaging(const void *data, VkDeviceSize size) const;
    VkImage createTextureImage(const argon::TextureData &texture) const;
    void copyStagingToTexture(VkCommandBuffer commandBuffer, const Buffer &staging, const argon::TextureData &texture, VkImage image) const;
    VkImageView createTextureView(const argon::TextureData &texture, VkImage image) const;
    VkCommandBuffer allocateCommandBuffer() const;
    VkCommandBuffer beginOneShot() const;
    void endOneShot(VkCommandBuffer commandBuffer) const;

    UploadBatch openUploads_;   // cmd: none when not batching
    std::vector<UploadBatch> submittedUploads_;
};
