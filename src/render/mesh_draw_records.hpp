// Argon: what each mesh draw reads besides its matrices (shaders\mesh_draws.glsl): a storage buffer per frame in
// flight, bound in every scene set of that frame (binding 4), indexed by the draw's instance index. The push
// constants keep only the matrices: Vulkan guarantees 128 bytes of them.
#pragma once
#include "gfx/device.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <span>
#include <vector>

struct MeshDraw
{
    glm::vec4 ambient{0.f};   // w = 1: this draw's scene ambient and light 0 instead of the frame's (a craft's, 0x59bc0)
    glm::vec4 light0{0.f};
    // per stage: the rate its animated texture's frame is picked with when an earlier draw's bind still holds it
    // (render\texture_bind_cache.hpp); < 0: the stage's own
    glm::vec4 stageRates{-1.f};
    int32_t material = 0;   // into the bound material set
    int32_t padding[3]{};
};
static_assert(sizeof(MeshDraw) == 64, "MeshDraw must match the std430 layout of shaders\\mesh_draws.glsl");

class MeshDrawRecords
{
  public:
    static constexpr uint32_t kCapacity = 65536;   // records per frame

    void create(argon::gfx::Device &device, int frames);
    void destroy(argon::gfx::Device &device);
    // empties the frame's records; its GPU work must be done
    void begin(int frame);
    // the record's index, the draw's firstInstance; throws when the frame holds kCapacity
    uint32_t add(const MeshDraw &draw);
    // one per frame in flight
    std::span<const argon::gfx::BufferRef> buffers() const;

  private:
    std::vector<argon::gfx::BufferRef> buffers_;
    std::vector<MeshDraw *> mapped_;
    MeshDraw *current_ = nullptr;
    uint32_t count_ = 0;
};
