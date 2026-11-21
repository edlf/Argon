// Argon: the flares' occlusion tests on the GPU (0xb6e60 -> 0xa64f0 / 0xa6750): before a surface-flag-0x40000 mesh,
// the first N of 256 points in a unit sphere (0xa63b0) scaled by the surface radius are drawn depth tested inside a
// visibility test; the last result gives the intensity (instance +0x138) = min(1, 2 x visible / N); 0 skips the draw,
// and so does a bone without a result yet (render\flare_occlusion.hpp). The queries of a frame: every view's tested
// meshes (alt_bracken 180 + the grid's flares, x 4 in split screen).
#pragma once
#include "gfx/command_list.hpp"
#include "gfx/device.hpp"
#include "gfx/presenter.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

class FlareQueries
{
  public:
    static const int kMaxPerFrame = 2048;

    // a tested mesh's key: its model, its bone (bone < 0: per draw) and its instance
    static uint64_t key(const void *model, int bone, int drawIndex, int instance);

    void create(argon::gfx::Device &device);
    void destroy();
    bool active() const;

    // at the start of a frame's recording (its fence has been waited on): the frame slot's last results ->
    // intensities (0xa6750 GetVisibilityTestResult), then its queries reset. samples: the scene's samples per pixel
    void readResults(argon::gfx::CommandList &commands, int frame, int samples);

    bool hasRoom(int frame) const;
    // the frame slot's next query, for the key's first points points
    argon::gfx::OcclusionQuery queue(int frame, uint64_t key, int points);
    // a tested mesh's intensity: the last result, 0 before the first (flare_occlusion.hpp)
    float testedIntensity(uint64_t key);
    std::optional<float> knownIntensity(uint64_t key) const;

    argon::gfx::Pipeline pipeline() const;
    argon::gfx::BufferRef points() const;

  private:
    argon::gfx::Device *device_ = nullptr;
    argon::gfx::QueryPool pool_;
    argon::gfx::Pipeline pipeline_;
    argon::gfx::BufferRef points_;
    std::array<std::vector<std::pair<uint64_t, int>>, argon::gfx::kFramesInFlight> queued_;   // per query: key, point count
    std::vector<uint64_t> results_;   // readResults' read-back, its storage kept between frames
    std::unordered_map<uint64_t, float> intensities_;   // key -> intensity (bone record +0x138)
};
