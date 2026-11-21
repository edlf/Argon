// Argon: replaced race tracks waiting out the frames in flight (retired_tracks.hpp).
#include "render/retired_tracks.hpp"

void RetiredTracks::create(ModelBuilder &builder, argon::gfx::Device &device)
{
    builder_ = &builder;
    device_ = &device;
}

bool RetiredTracks::empty() const
{
    return retired_.empty();
}

void RetiredTracks::retire(Retired &&retired)
{
    retired_.push_back(std::move(retired));
}

// retired at frame n (between frames): frames n-1 and n-2 may read it; their fences were waited by frame n + kFrames
void RetiredTracks::freeFinished(uint64_t framesSubmitted)
{
    if (!retired_.empty() && framesSubmitted >= retired_.front().frame + argon::gfx::kFramesInFlight)
    {
        destroyAll();
    }
}

void RetiredTracks::destroyAll()
{
    for (Retired &retired : retired_)
    {
        builder_->release(retired.track);
        builder_->release(retired.sky);
        device_->resetSetPool(retired.pool);
    }
    retired_.clear();
}
