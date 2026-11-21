// Argon: everything a race on one track needs, read and decoded from disk (models, textures, track data,
// the route and collision grids, sounds). Pure CPU work with no Vulkan or audio device calls, so the loading screen
// runs it on a worker thread; TrackLoad (track_load.hpp) then hands it to the GPU and the audio engine.
#pragma once
#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "audio/audio.hpp"
#include "audio/generic_sounds.hpp"
#include "craft/craftsim.hpp"
#include "files/dds.hpp"
#include "craft/drive.hpp"
#include "model/model_source.hpp"
#include "track/pickup_folders.hpp"
#include "track/trackdata.hpp"

// the AI route, its collision grids and the route check (App::applyRoute takes them)
struct RouteBuild
{
    argon::CollisionGrid grid;      // collision + the water.dat box tops (ships hover over water too)
    argon::CollisionGrid simGrid;   // collision only (the craft sim handles water as water)
    argon::Route route;
    bool routeBuilt = false;
    std::string routeError;
    float hover = 0.6f;          // start slot 0 above the road (the game parks ships at their hover height)
    std::vector<argon::RouteIssue> issues;
    argon::AiGraph aiGraph;
};
RouteBuild buildRoute(const std::vector<glm::vec3> &collTris, const argon::TrackData &data, float vmax);

// the grid as the game builds a quick race (0x79250 + 0x82960): slot 0 the player, slot 1 the player's nemesis
// (pilot id +-8), the others random: id = rand / 0x888 until it is unlocked, not the player and not on the grid yet
// (slot 5's optional extra pilot +0x3948 is not used)
struct GridRule
{
    std::vector<std::string> pilots;   // data\craft folders
    size_t player = 0;                 // into pilots
    uint32_t seed = 1;
    uint16_t unlockedCharacters = 0xffff;   // profile +0x5b0 (App::unlockMask), bit = pilot id
    bool allowsRandomPilot(const std::string &folder) const;
};
// a random grid slot may take this pilot: one of the 16 characters, unlocked (GridRule::allowsRandomPilot)
bool randomPilotAllowed(uint16_t unlockedCharacters, const std::string &folder);
std::vector<std::string> raceGrid(const GridRule &rule, size_t starts);   // at least 6 slots, at most every open pilot
std::string gridPilotOf(const GridRule &rule, const std::vector<std::string> &grid, size_t slot);

// what the main thread knows when the load starts
struct TrackRequest
{
    std::string game, name;                   // name: the data\tracks folder
    std::vector<std::string> grid;            // the kept grid order; empty: raceGrid for the track's start slots
    GridRule gridRule;
    std::vector<std::string> extraCraft;      // also needed (the loaded ghost's craft)
    std::vector<std::string> loadedCraft;     // craft folders already on the GPU (skipped)
    std::vector<std::string> loadedPickups;   // pickup folders already on the GPU (skipped)
    float vmax = 110.f;                       // the route's speed plan
    bool decodeSound = false;                 // an audio device is open
    bool fruityFruit = false;                 // the Fruity Fruit egg: the pickups' _ee models
};

struct TrackSource
{
    std::string name;
    std::vector<std::string> grid;   // pilot folder per grid slot (App::gridOrder)
    ModelSource model, sky;   // sky: not loaded when the track has no sky folder
    argon::TrackData data;
    RouteBuild route;
    argon::TrackSounds sounds;
    argon::EngineSounds engine;                              // the player's craft
    std::vector<argon::EngineSounds> otherEngines;           // grid slots 1..n (craft_sound_bind 0xc3220)
    std::vector<argon::ShipSounds> ships;                    // every grid slot's ship sounds
    argon::GenericSoundBank genericSounds;                   // audio\generic by the game's id
    argon::sound::ImpactSettings impactSettings;             // audio\f_settings.txt
    std::array<argon::TextureData, argon::kEnvCubeSlots> envCubes;   // by environment cube slot
    argon::TextureData waterBump;                            // read when the track has water
    argon::TextureData spray;                                // envdata\quadsprite
    std::map<std::string, ModelSource> craft, pickups;    // by folder, only those not loaded yet
};
TrackSource readTrackSource(const TrackRequest &request);

// the single readers, also used by the main thread's lazy loads
std::string trackAudioFolder(const std::string &track);   // alt_ tracks share the normal track's sounds
argon::TextureData readTrackCube(const std::string &game, const std::string &track, int slot);   // slot 0..7
argon::TextureData readWaterBump(const std::string &game);
argon::TextureData readSprayTexture(const std::string &game, const std::string &track);
