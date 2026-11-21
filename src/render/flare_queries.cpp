// Argon: the flares' occlusion tests on the GPU (flare_queries.hpp).
#include "render/flare_queries.hpp"

#include "model/cmm.hpp"
#include "render/flare_occlusion.hpp"

#include <span>

uint64_t FlareQueries::key(const void *model, int bone, int drawIndex, int instance)
{
    const uint64_t id = bone >= 0 ? uint64_t(bone) : (uint64_t(1) << 20) | uint64_t(drawIndex);
    return (uint64_t(uintptr_t(model)) << 24) ^ (id << 8) ^ uint64_t(instance & 0xff);
}

void FlareQueries::create(argon::gfx::Device &device)
{
    device_ = &device;
    std::vector<argon::Vertex> vertices;
    for (const glm::vec3 &point : argon::flare::testPoints())
    {
        argon::Vertex vertex{};
        vertex.pos[0] = point.x;
        vertex.pos[1] = point.y;
        vertex.pos[2] = point.z;
        vertices.push_back(vertex);
    }
    points_ = device.uploadBuffer(argon::gfx::BufferUse::Vertex, std::as_bytes(std::span(vertices)));
    pipeline_ = device.createPipeline({.program = argon::gfx::ShaderProgram::Occlusion, .topology = argon::gfx::Topology::OcclusionPoints});
    pool_ = device.createOcclusionQueries(uint32_t(kMaxPerFrame * argon::gfx::kFramesInFlight));
    for (auto &keys : queued_)
    {
        keys.clear();
    }
    intensities_.clear();
}

void FlareQueries::destroy()
{
    device_->destroyBuffer(points_);
    pool_ = {};   // the device's, freed with it
}

bool FlareQueries::active() const
{
    return bool(pool_);
}

void FlareQueries::readResults(argon::gfx::CommandList &commands, int frame, int samples)
{
    if (!pool_)
    {
        return;
    }
    auto &keys = queued_[size_t(frame)];
    if (!keys.empty())
    {
        results_.assign(keys.size(), 0);
        if (device_->readQueries({pool_, uint32_t(frame * kMaxPerFrame), uint32_t(keys.size())}, results_))
        {
            const int samplesPerPixel = device_->preciseOcclusion() ? samples : 0;   // 0: any-sample booleans
            for (size_t i = 0; i < keys.size(); ++i)
            {
                const int points = keys[i].second;
                intensities_[keys[i].first] = argon::flare::intensity(argon::flare::visiblePoints(results_[i], samplesPerPixel, points), points);
            }
        }
    }
    keys.clear();
    commands.resetQueries({pool_, uint32_t(frame * kMaxPerFrame), uint32_t(kMaxPerFrame)});
}

bool FlareQueries::hasRoom(int frame) const
{
    return queued_[size_t(frame)].size() < size_t(kMaxPerFrame);
}

argon::gfx::OcclusionQuery FlareQueries::queue(int frame, uint64_t key, int points)
{
    auto &keys = queued_[size_t(frame)];
    const uint32_t index = uint32_t(frame * kMaxPerFrame + int(keys.size()));
    keys.push_back({key, points});
    return argon::gfx::OcclusionQuery{pool_, index, device_->preciseOcclusion()};
}

float FlareQueries::testedIntensity(uint64_t key)
{
    return intensities_.try_emplace(key, argon::flare::kUntestedIntensity).first->second;
}

std::optional<float> FlareQueries::knownIntensity(uint64_t key) const
{
    auto known = intensities_.find(key);
    if (known == intensities_.end())
    {
        return std::nullopt;
    }
    return known->second;
}

argon::gfx::Pipeline FlareQueries::pipeline() const
{
    return pipeline_;
}

argon::gfx::BufferRef FlareQueries::points() const
{
    return points_;
}
