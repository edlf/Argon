// Argon: replaced race tracks that frames in flight may still read; each goes kFramesInFlight frames later, with
// its descriptor pool.
#pragma once
#include "gfx/device.hpp"
#include "gfx/presenter.hpp"
#include "render/gpu_model.hpp"
#include "render/model_builder.hpp"

#include <cstdint>
#include <vector>

class RetiredTracks
{
  public:
    // frame: the frames submitted when it was replaced
    struct Retired
    {
        Track track, sky;
        argon::gfx::SetPool pool;
        uint64_t frame = 0;
    };

    void create(ModelBuilder &builder, argon::gfx::Device &device);
    bool empty() const;
    void retire(Retired &&retired);
    // the ones no frame in flight can read now (call between frames)
    void freeFinished(uint64_t framesSubmitted);
    void destroyAll();

  private:
    ModelBuilder *builder_ = nullptr;
    argon::gfx::Device *device_ = nullptr;
    std::vector<Retired> retired_;
};
