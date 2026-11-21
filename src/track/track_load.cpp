// Argon: committing a read track to the GPU and the audio engine in slices (track_load.hpp).
#include "track/track_load.hpp"

#include <iterator>
#include <stdexcept>

#include "render/model_builder.hpp"
#include "render/prop_models.hpp"

TrackLoad::TrackLoad(const Use &use, TrackRequest request, std::launch policy)
    : host(use.host), sources(use.sources), name(request.name), reading(std::async(policy, readTrackSource, std::move(request)))
{
}

// the slice's uploads go to the GPU together, without waiting for it (GpuContext::beginUploadBatch)
bool TrackLoad::step(double budgetMs)
{
    if (!received && !sourceReady())
    {
        return false;
    }
    sources.uploads.beginUploadBatch();
    const bool done = commitFor(budgetMs);
    sources.uploads.submitUploadBatch();
    return done;
}

bool TrackLoad::commitFor(double budgetMs)
{
    const auto until = std::chrono::steady_clock::now() + std::chrono::duration<double, std::milli>(budgetMs);
    if (!received)
    {
        receiveSource();
    }
    bool done = false;
    do
    {
        done = commitNextSlice();
    } while (!done && std::chrono::steady_clock::now() < until);
    return done;
}

void TrackLoad::finish()
{
    if (!received)
    {
        receiveSource();
    }
    while (!commitNextSlice())
    {
    }
}

// a deferred read reports "deferred": it runs on get()
bool TrackLoad::sourceReady()
{
    return reading.wait_for(std::chrono::seconds(0)) != std::future_status::timeout;
}

// the old track goes (nothing draws it behind the loading screen), the new one's cube maps first: its materials
// look them up
void TrackLoad::receiveSource()
{
    try
    {
        src = reading.get();
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("reading track " + name + ": " + e.what());
    }
    received = true;
    host.clearTrack();
    sources.track.name = name;
    for (int k = 0; k < argon::kEnvCubeSlots; ++k)
    {
        sources.models.setCube(sources.track, k, src.envCubes[size_t(k)]);
    }
    queueModels();
}

// the track, its sky, then the craft and pickups not loaded yet (kept for the session, App::props)
void TrackLoad::queueModels()
{
    models.push_back({&sources.track, &src.model, ModelUse{false, sources.trackPool, false}});
    if (src.sky.loaded || !src.sky.warnings.empty())
    {
        models.push_back({&sources.sky, &src.sky, ModelUse{true, sources.trackPool, false}});
    }
    queueProps(src.craft, &PropModels::craftSlot, sources.props.craftUse());
    queueProps(src.pickups, &PropModels::pickupSlot, sources.props.pickupUse());
}

void TrackLoad::queueProps(std::map<std::string, ModelSource> &reads, PropSlot slot, const ModelUse &use)
{
    for (auto &[folder, model] : reads)
    {
        models.push_back({&(sources.props.*slot)(folder), &model, use});
    }
}

bool TrackLoad::commitNextSlice()
{
    if (modelAt < models.size())
    {
        commitModelSlice(models[modelAt]);
        return false;
    }
    static constexpr Stage kStages[kStageCount] = {
        &TrackLoad::applyTrackData, &TrackLoad::startSounds, &TrackLoad::buildScenery, &TrackLoad::placePickups, &TrackLoad::showTrack};
    if (stageAt < std::size(kStages))
    {
        (this->*kStages[stageAt++])();
    }
    return stageAt >= std::size(kStages);
}

float TrackLoad::progress() const
{
    if (!received)
    {
        return 0.f;
    }
    return float(modelAt + stageAt) / float(models.size() + kStageCount);
}

// one texture, or once they are all up the meshes
void TrackLoad::commitModelSlice(PendingModel &pending)
{
    if (pending.target->textures.size() < pending.src->textures.size())
    {
        sources.models.uploadTexture(*pending.target, *pending.src);
        return;
    }
    sources.models.buildMeshes(*pending.target, *pending.src, pending.use);
    ++modelAt;
}

void TrackLoad::applyTrackData()
{
    host.applyTrackData(src);
}

void TrackLoad::startSounds()
{
    host.startTrackSounds(src);
}

void TrackLoad::buildScenery()
{
    host.buildTrackScenery(src);
}

void TrackLoad::placePickups()
{
    host.placeTrackPickups();
}

void TrackLoad::showTrack()
{
    host.showTrack(name, std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count());
}
