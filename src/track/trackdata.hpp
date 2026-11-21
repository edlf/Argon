// Per-track markers and settings (C++ port of the readers in tools/cmm/trackdata.py).
// All positions here are already converted to right-handed (z mirrored).
#pragma once
#include "track/track_sprites.hpp"
#include "track/track_visibility.hpp"

#include <array>
#include <string>
#include <vector>

namespace argon
{

struct StartSlot
{
    std::string name;
    std::array<float, 3> pos, forward;
    float matrix[16];
};   // matrix: D3D row-vector
struct AiNode
{
    int id;
    std::array<float, 3> pos;
    std::array<float, 3> down{0.f, 1.f, 0.f};      // <down=>: the node's up in fact (0, 1, 0 in the files)
    std::vector<int> forward;
    unsigned turboGood = 0;                        // bit s: turbo_good_<slow .. ultrafast> (node flag 0x2000 << s)
    bool restart = false, first = false;           // flag_restart, flag_first
    bool cornerSlow = false, cornerHairpin = false, cornerStraight = false;
    bool envInside = false;                        // env_inside: under a roof (no rain)
};
struct Pickup
{
    std::string type;
    std::array<float, 3> pos;
};
// the track's environment cube slots (0xcdcf0 -> 0xa2110, table 0x50f880): 0 envdata\default, 1 waterref1,
// 2 waterref2, 3..7 custom1..5; water.dat's b and a mesh's +0xa8 pick one
constexpr int kEnvCubeSlots = 8;

// water.dat volume; the water surface is the top face (max y)
// flags & 1 = physics only (no water surface drawn), else the box gets bump-water patches; cubeSlot = environment cube slot
struct WaterBox
{
    std::array<float, 3> min, max;
    uint32_t flags = 0, cubeSlot = 0;
};

struct TrackProps
{
    std::string name;
    std::array<float, 3> fogColour{0.5f, 0.5f, 0.5f};
    float fogDensity = 0.f, fogRange = 0.f;
    int fogMode = 0;
    std::array<float, 3> ambient{0.25f, 0.25f, 0.25f}, sunColour{0.8f, 0.8f, 0.8f};
    std::array<float, 3> sunVector{-0.5f, -1.f, 1.f};
};

// config\camera_<t>.txt: replay / TV cameras
struct TrackCamera
{
    std::string type, name;                 // static, moving, heli, ship
    std::array<float, 3> pos{}, target{}, pos2{}, target2{};   // moving: start / end
    float fov = 60, fov2 = 60, range = 1000, hold = 2, height = 100;
    float panFrom = 0, panTo = 100;         // lap-progress window in %
    bool dynamicFov = false, dynamicTracking = false, onlyForward = false, kerb = false;
    bool onlyInFrame = false, presetFov = false, tightFov = false;   // the replay director's (logic/replay_director.cpp)
};

struct TrackData
{
    std::vector<TrackCamera> cameras;
    std::vector<StartSlot> starts;
    std::vector<AiNode> ai;
    std::vector<Pickup> pickups;
    std::vector<WaterBox> water;
    float waterAmp = 1.f;          // water.dat header +8: wave height scale (vertex shader c4.y)
    TrackVisibility visibility;    // visdata.vis (none on Mars)
    std::vector<TrackSprite> sprites;   // sprite.dat (Bracken, Mount, Silver and their alt versions)
    TrackProps props;
    bool hasProps = false;
};

// game = game root (contains data\ and config\), track = folder name, e.g. "bracken"
TrackData loadTrackData(const std::string &game, const std::string &track);

} // namespace argon
