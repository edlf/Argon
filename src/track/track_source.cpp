// Argon: reading a track off the render thread (track_source.hpp).
#include "track/track_source.hpp"
#include "files/game_path.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

namespace
{

bool contains(const std::vector<std::string> &names, const std::string &name)
{
    return std::find(names.begin(), names.end(), name) != names.end();
}

// the grid's craft (one per start slot) and the extra ones, each once
std::vector<std::string> neededCraft(const TrackRequest &rq, const TrackSource &src)
{
    std::vector<std::string> need;
    for (size_t slot = 0; slot < src.data.starts.size(); ++slot)
    {
        need.push_back(gridPilotOf(rq.gridRule, src.grid, slot));
    }
    need.insert(need.end(), rq.extraCraft.begin(), rq.extraCraft.end());
    std::vector<std::string> unique;
    for (const std::string &file : need)
    {
        if (!file.empty() && !contains(unique, file) && !contains(rq.loadedCraft, file))
        {
            unique.push_back(file);
        }
    }
    return unique;
}

// every pickup's model and its taken (_NS) model; the power chip has no taken model
std::vector<std::string> neededPickups(const TrackRequest &rq, const argon::TrackData &data)
{
    std::string (*const folderOf)(const std::string &) = rq.fruityFruit ? fruityPickupFolder : pickupFolder;
    std::vector<std::string> need;
    for (const argon::Pickup &pickup : data.pickups)
    {
        need.push_back(folderOf(pickup.type));
        if (pickup.type != "POWERCHIP")
        {
            need.push_back(folderOf(pickup.type + "_NS"));
        }
    }
    std::vector<std::string> unique;
    for (const std::string &file : need)
    {
        if (!contains(unique, file) && !contains(rq.loadedPickups, file))
        {
            unique.push_back(file);
        }
    }
    return unique;
}

void readModels(const std::string &root, const std::vector<std::string> &folders, std::map<std::string, ModelSource> &out)
{
    for (const std::string &folder : folders)
    {
        if (argon::gamePathExists(root + folder + "/model.cmm"))
        {
            out[folder] = readModel(root + folder);
        }
    }
}

bool isMars(const std::string &track)
{
    return track == "mars" || track == "alt_mars";
}

// 0x1a8454 (entry = track id + 1) gives Mars and its Terraformed route mount_music, and 0xded40 leaves them its default
// of 5 segments where Mount's own entry has 6: mount_music6 never plays on Mars
constexpr size_t kMarsMusicSegments = 5;

std::string musicFolderOf(const std::string &track)
{
    return isMars(track) ? "mount" : track;
}

void limitMarsMusic(const std::string &track, argon::MusicFolder &music)
{
    if (isMars(track) && music.segments.size() > kMarsMusicSegments)
    {
        music.segments.resize(kMarsMusicSegments);
    }
}

void readSounds(const TrackRequest &rq, TrackSource &src)
{
    src.sounds = argon::readTrackSounds(rq.game, trackAudioFolder(rq.name), musicFolderOf(rq.name), rq.decodeSound);
    limitMarsMusic(rq.name, src.sounds.music);
    if (!rq.decodeSound)
    {
        return;
    }
    src.engine = argon::readEngineSounds(rq.game, gridPilotOf(rq.gridRule, src.grid, 0));
    for (size_t slot = 0; slot < src.data.starts.size(); ++slot)
    {
        const std::string pilot = gridPilotOf(rq.gridRule, src.grid, slot);
        src.ships.push_back(argon::readShipSounds(rq.game, pilot));
        if (slot > 0)
        {
            src.otherEngines.push_back(argon::readOtherEngineSounds(rq.game, pilot));
        }
    }
    src.genericSounds = argon::readGenericSounds(rq.game);
    src.impactSettings = argon::readImpactSettings(rq.game);
}

void readTextures(const TrackRequest &rq, TrackSource &src)
{
    for (int k = 0; k < argon::kEnvCubeSlots; ++k)
    {
        src.envCubes[size_t(k)] = readTrackCube(rq.game, rq.name, k);
    }
    if (!src.data.water.empty())
    {
        src.waterBump = readWaterBump(rq.game);
    }
    src.spray = readSprayTexture(rq.game, rq.name);
}

}  // namespace

RouteBuild buildRoute(const std::vector<glm::vec3> &collTris, const argon::TrackData &data, float vmax)
{
    RouteBuild rb;
    std::vector<glm::vec3> tris = collTris;
    for (auto &box : data.water)
    {
        float y = box.max[1];
        glm::vec3 a(box.min[0], y, box.min[2]), b(box.max[0], y, box.min[2]), c(box.max[0], y, box.max[2]), d(box.min[0], y, box.max[2]);
        tris.insert(tris.end(), {a, b, c, a, c, d});
    }
    rb.grid.build(tris);
    try
    {
        rb.route.build(data);
    }
    catch (const std::runtime_error &e)
    {
        rb.routeError = e.what();
        return rb;
    }
    rb.routeBuilt = true;
    rb.route.planSpeeds(vmax, 45.f, 25.f, 50.f);
    if (!data.starts.empty())
    {
        auto &start = data.starts[0];
        argon::GroundHit ground;
        if (rb.grid.rayDown(glm::vec3(start.pos[0], start.pos[1] + 5.f, start.pos[2]), 50.f, ground))
        {
            rb.hover = std::clamp(start.pos[1] - ground.y, 0.2f, 5.f);
        }
    }
    rb.issues = rb.route.check(rb.grid, rb.hover);
    rb.simGrid.build(collTris);
    rb.aiGraph.build(data);
    return rb;
}

TrackSource readTrackSource(const TrackRequest &rq)
{
    TrackSource src;
    src.name = rq.name;
    const std::string dir = rq.game + "/data/tracks/" + rq.name;
    src.model = readModel(dir);
    if (argon::gamePathExists(dir + "/sky/model.cmm"))
    {
        src.sky = readModel(dir + "/sky");
    }
    src.data = argon::loadTrackData(rq.game, rq.name);
    src.grid = rq.grid.empty() ? raceGrid(rq.gridRule, src.data.starts.size()) : rq.grid;
    src.route = buildRoute(src.model.collTris, src.data, rq.vmax);
    readSounds(rq, src);
    readTextures(rq, src);
    readModels(rq.game + "/data/craft/", neededCraft(rq, src), src.craft);
    readModels(rq.game + "/data/objects/", neededPickups(rq, src.data), src.pickups);
    return src;
}

std::vector<std::string> raceGrid(const GridRule &rule, size_t starts)
{
    if (rule.pilots.empty())
    {
        return {};
    }
    std::string byId[16];
    for (auto &pilot : rule.pilots)
    {
        int id = argon::pilotIndex(pilot);
        if (argon::pilotName(id) && byId[id].empty() && pilot == argon::pilotFolder(id))
        {
            byId[id] = pilot;
        }
    }
    const std::string &me = rule.pilots[rule.player % rule.pilots.size()];
    int myId = argon::pilotIndex(me);
    std::vector<std::string> grid{me};
    int nem = myId < 8 ? myId + 8 : myId - 8;
    if (!byId[nem].empty())
    {
        grid.push_back(byId[nem]);
    }
    size_t avail = grid.size();
    for (auto &pilot : byId)
    {
        avail += rule.allowsRandomPilot(pilot) && std::find(grid.begin(), grid.end(), pilot) == grid.end();
    }
    size_t want = std::min<size_t>(std::max<size_t>(starts, 6), avail);
    uint32_t seed = rule.seed;
    while (grid.size() < want)
    {
        seed = seed * 214013u + 2531011u;                       // MSVC rand(): 0..0x7fff
        int id = int((seed >> 16) & 0x7fff) / 0x888;
        if (id > 15 || !rule.allowsRandomPilot(byId[id]))
        {
            continue;
        }
        if (std::find(grid.begin(), grid.end(), byId[id]) != grid.end())
        {
            continue;
        }
        grid.push_back(byId[id]);
    }
    return grid;
}

bool GridRule::allowsRandomPilot(const std::string &folder) const
{
    return randomPilotAllowed(unlockedCharacters, folder);
}

bool randomPilotAllowed(uint16_t unlockedCharacters, const std::string &folder)
{
    if (folder.empty())
    {
        return false;
    }
    const int id = argon::pilotIndex(folder);
    return id >= 0 && id < 16 && ((unlockedCharacters >> id) & 1) != 0;
}

// past the grid's end: the pilots in folder order (App::gridPilot)
std::string gridPilotOf(const GridRule &rule, const std::vector<std::string> &grid, size_t slot)
{
    if (rule.pilots.empty())
    {
        return {};
    }
    if (slot < grid.size())
    {
        return grid[slot];
    }
    return rule.pilots[slot % rule.pilots.size()];
}

std::string trackAudioFolder(const std::string &track)
{
    return track.rfind("alt_", 0) == 0 ? track.substr(4) : track;
}

// the slot's envdata file (else data\defaultcubemap's, 0xa2040) as a 6-layer array; no levels when neither is a cube
argon::TextureData readTrackCube(const std::string &game, const std::string &track, int slot)
{
    static const char *const kFiles[argon::kEnvCubeSlots] = {
        "default.dds", "waterref1.dds", "waterref2.dds", "custom1.dds", "custom2.dds", "custom3.dds", "custom4.dds", "custom5.dds"};
    const char *name = kFiles[slot];
    for (const std::string &path : {game + "/data/tracks/" + track + "/envdata/" + name, game + "/data/defaultcubemap/" + name})
    {
        argon::TextureData env = argon::loadOptionalDds(path);
        if (env.cube && env.layers == 6)
        {
            env.cube = false;
            return env;
        }
    }
    return {};
}

// data\testdata\waterbump000..059 as one 60-layer array (no levels when a frame is missing or differs)
argon::TextureData readWaterBump(const std::string &game)
{
    argon::TextureData bump;
    for (int k = 0; k < 60; ++k)
    {
        char name[64];
        std::snprintf(name, sizeof name, "/data/testdata/waterbump%03d.dds", k);
        const argon::TextureData texture = argon::loadOptionalDds(game + name);
        if (texture.levels.empty())
        {
            return {};
        }
        if (k == 0)
        {
            bump = texture;
            bump.layerStride = texture.bytes.size();
            bump.layers = 1;
            continue;
        }
        if (texture.format != bump.format || texture.width != bump.width || texture.bytes.size() != bump.layerStride)
        {
            return {};
        }
        bump.bytes.insert(bump.bytes.end(), texture.bytes.begin(), texture.bytes.end());
        bump.layers++;
    }
    return bump;
}

argon::TextureData readSprayTexture(const std::string &game, const std::string &track)
{
    return argon::loadOptionalDds(game + "/data/tracks/" + track + "/envdata/quadsprite.dds");
}
