// Argon: loading a track without holding up the frame. A worker thread reads everything from disk
// (readTrackSource, track_source.hpp); the main thread then hands it to the GPU and the audio engine one slice at a
// time (a texture, a model's meshes, the route, the sounds, ...) within a time budget per frame, so the loading
// screen keeps drawing. A blocking load (the editor, --track) runs the same steps without the thread.
#pragma once
#include <chrono>
#include <future>
#include <vector>

#include "track/track_source.hpp"
#include "render/upload_batches.hpp"
#include "gfx/gfx_types.hpp"

class ModelBuilder;
class PropModels;
class Renderer;
struct Track;

// what the load fills: the GPU side, the model builder, the props kept for the session, the track and its sky
struct TrackLoadSources
{
    UploadBatches &uploads;   // the models read at once go up in one submission
    const argon::gfx::SetPool &trackPool;
    ModelBuilder &models;
    PropModels &props;
    Track &track, &sky;
};

// what the load hands App, one slice each: the old track gone, then the read track's data, sounds, scenery, pickups and
// its first showing
class TrackLoadHost
{
  public:
    virtual void clearTrack() = 0;
    virtual void applyTrackData(TrackSource &source) = 0;      // the data, the grid, the route
    virtual void startTrackSounds(TrackSource &source) = 0;    // the track's, the music, the generic ones, the engines
    virtual void buildTrackScenery(TrackSource &source) = 0;   // the markers, the water, the spray texture
    virtual void placeTrackPickups() = 0;                     // the pickup models, the catch-up, the scoring
    virtual void showTrack(const std::string &name, double loadSeconds) = 0;

  protected:
    ~TrackLoadHost() = default;
};

class TrackLoad
{
  public:
    // policy: std::launch::async reads on a worker thread, std::launch::deferred on the first step()
    struct Use
    {
        TrackLoadHost &host;
        TrackLoadSources sources;
    };
    TrackLoad(const Use &use, TrackRequest request, std::launch policy);
    TrackLoad(const TrackLoad &) = delete;
    TrackLoad &operator=(const TrackLoad &) = delete;

    static constexpr double kFrameSliceMs = 4.0;   // main-thread time per frame for a load behind a loading screen

    // works until budgetMs has passed (at least one slice once the source is read); true when the track is in place
    bool step(double budgetMs);
    void finish();   // the rest at once
    float progress() const;   // 0 while the worker reads, then the share of the slices committed

  private:
    struct PendingModel
    {
        Track *target;
        ModelSource *src;
        ModelUse use;
    };
    using Stage = void (TrackLoad::*)();
    static constexpr size_t kStageCount = 5;   // applyTrackData .. showTrack (commitNextSlice)

    bool commitFor(double budgetMs);
    bool sourceReady();
    void receiveSource();
    void queueModels();
    using PropSlot = Track &(PropModels::*)(const std::string &folder);   // the model a read fills
    void queueProps(std::map<std::string, ModelSource> &reads, PropSlot slot, const ModelUse &use);
    bool commitNextSlice();   // true when nothing is left
    void commitModelSlice(PendingModel &pending);
    void applyTrackData();
    void startSounds();
    void buildScenery();
    void placePickups();
    void showTrack();

    TrackLoadHost &host;
    TrackLoadSources sources;
    const std::string name;
    std::future<TrackSource> reading;
    TrackSource src;
    bool received = false;
    std::vector<PendingModel> models;
    size_t modelAt = 0;
    size_t stageAt = 0;
    const std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
};
