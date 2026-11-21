// Argon: a track taken into the game (track\track_load.hpp TrackLoadHost): the request a load starts from, the old
// track retired, then one slice of the load each for the read track's data, sounds, scenery (the editor's markers,
// the water, the spray), pickups and first showing. The steps that are the race's own go back through
// TrackArrivalHost: the route and the drive over it, the race music, the particle textures, the pickups' state and the
// start camera.
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "render/prop_models.hpp"
#include "track/track_load.hpp"

namespace argon
{
class Audio;
class RouteReport;
}  // namespace argon
class Race;
class RaceEggs;
class RaceSounds;
class RetiredTracks;
class StartGrid;
struct DriveTuning;
struct RaceScoring;

using PickupModels = std::map<std::string, Found<Track>>;   // <track>pup.txt type -> model

class TrackArrivalHost
{
  public:
    virtual void applyRoute(RouteBuild &&route) = 0;   // the grid, route and AI graph, the drive placed on them
    virtual void applyRaceMusic() = 0;
    virtual void loadParticleTextures() = 0;
    virtual void resetPickups() = 0;
    virtual void cameraToStart() = 0;

  protected:
    ~TrackArrivalHost() = default;
};

struct TrackArrivalSources
{
    Renderer &renderer;                          // the pools, the device, the spray texture
    ModelBuilder &models;                        // the water
    PropModels &props;                           // the craft and pickup models kept across tracks
    Track &track, &sky;
    RetiredTracks &retiredTracks;                // the tracks replaced, until no frame in flight reads them
    std::uint64_t &trackSerial;                  // a new one each time the track is cleared
    const std::vector<std::string> &trackList;
    int &trackIndex;                             // the loaded track's row in trackList, -1 none
    StartGrid &startGrid;
    argon::Audio &audio;
    RaceSounds &raceSounds;                      // the impact settings
    std::string &enginePilot;                    // the player's engine sound
    PickupModels &pickupModels, &takenPickupModels;   // the _ns model drawn once the player took it this lap
    const RaceEggs &raceEggs;                    // Fruity Fruit's pickups
    Race &race;                                  // the catch-up
    RaceScoring &scoring;                        // the track's lap goals
    const DriveTuning &drive;                    // the top speed the load reads with
    const argon::RouteReport &routeReport;       // the issues the editor's markers show
    const std::string &game;
};

class TrackArrival : public TrackLoadHost
{
  public:
    TrackArrival(const TrackArrivalSources &sources, TrackArrivalHost &host) : sources_(sources), host_(host)
    {
    }

    TrackLoad::Use use();                        // what a TrackLoad fills and hands back
    TrackRequest request(int listIndex) const;   // what a load of trackList[listIndex] needs to know
    void loadNow(int listIndex);                 // blocking (the editor, --track, the menus); out of the list: cleared

    void clearTrack() override;
    void applyTrackData(TrackSource &source) override;
    void startTrackSounds(TrackSource &source) override;
    void buildTrackScenery(TrackSource &source) override;
    void placeTrackPickups() override;
    void showTrack(const std::string &name, double loadSeconds) override;

  private:
    Found<Track> pickupModel(const std::string &type);
    void buildMarkers();

    TrackArrivalSources sources_;
    TrackArrivalHost &host_;
};
