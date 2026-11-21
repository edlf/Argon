#include "app/track_arrival.hpp"

#include <span>
#include <utility>

#include "app/race_sounds.hpp"
#include "audio/audio.hpp"
#include "craft/drive_tuning.hpp"
#include "logic/race.hpp"
#include "logic/race_eggs.hpp"
#include "logic/race_scoring.hpp"
#include "logic/start_grid.hpp"
#include "render/renderer.hpp"
#include "render/retired_tracks.hpp"
#include "track/marker_lines.hpp"
#include "track/pickup_folders.hpp"
#include "track/route_report.hpp"

TrackLoad::Use TrackArrival::use()
{
    Renderer &renderer = sources_.renderer;
    return TrackLoad::Use{*this, TrackLoadSources{renderer, renderer.trackPool, sources_.models, sources_.props, sources_.track, sources_.sky}};
}

// the grid (built by the read once it knows the start slots) and the models already loaded, which it skips
TrackRequest TrackArrival::request(int listIndex) const
{
    TrackRequest rq;
    rq.game = sources_.game;
    rq.name = sources_.trackList[size_t(listIndex)];
    rq.grid = sources_.startGrid.kept();
    rq.gridRule = sources_.startGrid.rule();
    rq.loadedCraft = sources_.props.craftFolders();
    rq.loadedPickups = sources_.props.pickupFolders();
    rq.vmax = sources_.drive.topSpeed;
    rq.decodeSound = sources_.audio.ok();
    rq.fruityFruit = sources_.raceEggs.on(Egg::FruityFruit);
    return rq;
}

void TrackArrival::loadNow(int listIndex)
{
    if (listIndex < 0 || listIndex >= int(sources_.trackList.size()))
    {
        clearTrack();
        return;
    }
    TrackLoad load(use(), request(listIndex), std::launch::deferred);
    load.finish();
}

// the track and sky retire with their descriptor pool (freed once no frame in flight reads them); the next track
// takes the spare pool. A second change within kFrames frames waits for the GPU instead.
void TrackArrival::clearTrack()
{
    Renderer &renderer = sources_.renderer;
    if (!sources_.retiredTracks.empty())
    {
        renderer.presenter().waitForSubmittedWork();
        sources_.retiredTracks.destroyAll();
    }
    sources_.retiredTracks.retire({std::move(sources_.track), std::move(sources_.sky), renderer.trackPool, renderer.presenter().framesSubmitted()});
    sources_.track = Track{};
    sources_.sky = Track{};
    std::swap(renderer.trackPool, renderer.spareTrackPool);
    sources_.trackIndex = -1;
    ++sources_.trackSerial;   // the props' borrowed cube sets are made again for the next track (ModelBuilder::refreshCubes)
}

void TrackArrival::applyTrackData(TrackSource &source)
{
    sources_.track.data = std::move(source.data);
    sources_.startGrid.order = std::move(source.grid);
    host_.applyRoute(std::move(source.route));
}

// the track's sounds, the custom music, the generic sounds, the engine
void TrackArrival::startTrackSounds(TrackSource &source)
{
    argon::Audio &audio = sources_.audio;
    audio.playTrackSounds(std::move(source.sounds));
    host_.applyRaceMusic();
    host_.loadParticleTextures();
    audio.raceSfx().setBank(std::move(source.genericSounds));
    audio.loadListenerFilters(sources_.game);
    sources_.raceSounds.useImpactSettings(source.impactSettings);
    sources_.enginePilot = sources_.startGrid.pilotOf(0);
    audio.playEngineSounds(std::move(source.engine));
    audio.raceSfx().setShipSounds(std::move(source.ships));
    audio.playOtherEngines(std::move(source.otherEngines));
    for (const std::string &warning : audio.warnings())
    {
        sources_.track.warnings.push_back("audio: " + warning);
    }
}

void TrackArrival::buildTrackScenery(TrackSource &source)
{
    buildMarkers();
    sources_.models.buildWater(sources_.track, source.waterBump, sources_.renderer.trackPool);
    sources_.renderer.useSprayTexture(source.spray, sources_.track.name);
}

// the editor's markings (track\marker_lines.hpp), each kind a range of the one line buffer
void TrackArrival::buildMarkers()
{
    Track &track = sources_.track;
    MarkerLines lines;
    track.lineStarts[0] = lines.count();
    lines.addStarts(track.data);
    track.lineStarts[1] = lines.count() - track.lineStarts[0];
    track.lineAi[0] = lines.count();
    lines.addAiRoute(track.data);
    track.lineAi[1] = lines.count() - track.lineAi[0];
    track.linePups[0] = lines.count();
    lines.addPickups(track.data);
    track.linePups[1] = lines.count() - track.linePups[0];
    track.lineSounds[0] = lines.count();
    lines.addEmitters(sources_.audio.emitters());
    track.lineSounds[1] = lines.count() - track.lineSounds[0];
    track.lineIssues[0] = lines.count();
    lines.addIssues(sources_.routeReport.issues());
    track.lineIssues[1] = lines.count() - track.lineIssues[0];
    if (lines.count() > 0)
    {
        track.lines = sources_.renderer.device().uploadBuffer(argon::gfx::BufferUse::Vertex, std::as_bytes(std::span(lines.vertices())));
    }
}

Found<Track> TrackArrival::pickupModel(const std::string &type)
{
    return sources_.props.pickup(sources_.raceEggs.on(Egg::FruityFruit) ? fruityPickupFolder(type) : pickupFolder(type));
}

void TrackArrival::placeTrackPickups()
{
    sources_.pickupModels.clear();
    for (const argon::Pickup &pickup : sources_.track.data.pickups)
    {
        sources_.pickupModels[pickup.type] = pickupModel(pickup.type);
    }
    sources_.takenPickupModels.clear();
    for (const argon::Pickup &pickup : sources_.track.data.pickups)
    {
        if (pickup.type != "POWERCHIP")
        {
            sources_.takenPickupModels[pickup.type] = pickupModel(pickup.type + "_NS");
        }
    }
    host_.resetPickups();
    sources_.race.loadCatchUp();
    sources_.scoring.load(sources_.game, sources_.track.name);
}

void TrackArrival::showTrack(const std::string &name, double loadSeconds)
{
    sources_.track.loadSeconds = loadSeconds;
    for (const std::string &warning : sources_.sky.warnings)
    {
        sources_.track.warnings.push_back("sky: " + warning);
    }
    host_.cameraToStart();
    for (int i = 0; i < int(sources_.trackList.size()); ++i)
    {
        if (sources_.trackList[size_t(i)] == name)
        {
            sources_.trackIndex = i;
        }
    }
}
