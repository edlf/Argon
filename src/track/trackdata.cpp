#include "track/trackdata.hpp"
#include "model/cmm.hpp"
#include "files/game_path.hpp"
#include "original/record_read.hpp"
#include "original/track_records.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

namespace argon
{

using original::readAt;

// game coordinates are used as-is (right-handed, Y up); kept as a single conversion point
static std::array<float, 3> rh(float x, float y, float z)
{
    return {x, y, z};
}

static std::vector<std::string> lines(const std::string &path)
{
    std::vector<std::string> out;
    std::ifstream f(findGamePath(path));
    std::string text;
    while (std::getline(f, text))
    {
        while (!text.empty() && (text.back() == '\r' || text.back() == ' ' || text.back() == '\t'))
        {
            text.pop_back();
        }
        size_t first = text.find_first_not_of(" \t");
        out.push_back(first == std::string::npos ? std::string() : text.substr(first));
    }
    return out;
}

// "<key=value>" -> value, or empty if the line isn't that key
static bool tag(const std::string &line, const char *key, std::string &val)
{
    size_t keyLength = std::strlen(key);
    if (line.size() < keyLength + 3 || line[0] != '<' || line.compare(1, keyLength, key) != 0 || line[keyLength + 1] != '=')
    {
        return false;
    }
    size_t close = line.find('>', keyLength + 2);
    val = line.substr(keyLength + 2, close == std::string::npos ? std::string::npos : close - keyLength - 2);
    return true;
}

namespace
{

std::array<float, 3> readVec3(const std::string &text)
{
    std::array<float, 3> values{};
    std::sscanf(text.c_str(), "%f %f %f", &values[0], &values[1], &values[2]);
    return values;
}

float readFloat(const std::string &text)
{
    return float(std::atof(text.c_str()));
}

void readStartSlots(const std::vector<uint8_t> &bytes, TrackData &td)
{
    const auto header = readAt<original::LocatorHeader>(bytes, 0);
    constexpr size_t kFirst = sizeof(original::LocatorHeader), kSize = sizeof(original::Locator);
    for (uint32_t i = 0; i < header.count && kFirst + (i + 1) * kSize <= bytes.size(); ++i)
    {
        const auto locator = readAt<original::Locator>(bytes, kFirst + i * kSize);
        const float *world = locator.world;
        StartSlot start;
        start.name.assign(locator.name, strnlen(locator.name, sizeof locator.name));
        std::copy(std::begin(locator.world), std::end(locator.world), start.matrix);
        start.pos = rh(world[12], world[13], world[14]);
        start.forward = rh(world[8], world[9], world[10]);     // row 2 = local +Z (forward)
        td.starts.push_back(start);
    }
}

void readWaterBoxes(const std::vector<uint8_t> &bytes, TrackData &td)
{
    const auto header = readAt<original::WaterHeader>(bytes, 0);
    constexpr size_t kFirst = sizeof(original::WaterHeader), kSize = sizeof(original::WaterBox);
    td.waterAmp = header.waveAmplitude;
    for (uint32_t i = 0; i < header.count && kFirst + (i + 1) * kSize <= bytes.size(); ++i)
    {
        const auto box = readAt<original::WaterBox>(bytes, kFirst + i * kSize);
        WaterBox waterBox;
        waterBox.flags = box.flags;
        waterBox.cubeSlot = box.cubeMap;
        waterBox.min = {box.min[0], box.min[1], box.min[2]};
        waterBox.max = {box.max[0], box.max[1], box.max[2]};
        td.water.push_back(waterBox);
    }
}

void readProperties(const std::vector<uint8_t> &bytes, TrackData &td)
{
    const auto record = readAt<original::TrackProperties>(bytes, 0);
    auto &props = td.props;
    td.hasProps = true;
    for (int k = 0; k < 3; ++k)
    {
        props.fogColour[k] = record.fogColour[k] / 255.f;
        props.ambient[k] = record.ambient[k] / 255.f;
        props.sunColour[k] = record.sunColour[k] / 255.f;
    }
    props.fogDensity = record.fogDensity;
    props.name.assign(record.trackName, strnlen(record.trackName, sizeof record.trackName));
    props.sunVector = rh(record.sunVector[0], record.sunVector[1], record.sunVector[2]);
    props.fogMode = record.fogMode;
    props.fogRange = record.fogRange;
}

void readSprites(const std::vector<uint8_t> &bytes, TrackData &td)
{
    td.sprites = readTrackSprites(bytes);
}

// a binary file read by `read` when it holds at least its header
template <class Header> void readBinary(const std::string &path, TrackData &td, void (*read)(const std::vector<uint8_t> &, TrackData &))
{
    std::vector<uint8_t> bytes;
    if (readFile(path, bytes) && bytes.size() >= sizeof(Header))
    {
        read(bytes, td);
    }
}

bool isForwardLink(const std::string &line)
{
    return line.rfind("<forward", 0) == 0 && line.rfind("<forwardn", 0) != 0 && line.find("numforward") == std::string::npos;
}

// <turbo_good_<speed>=true> -> bit s (the parser 0xe2520 sets node flag 0x2000 << s)
unsigned turboGoodBit(const std::string &line)
{
    static const char *const kSpeeds[] = {"slow", "leisurely", "normal", "fast", "ultrafast"};
    for (unsigned s = 0; s < 5; ++s)
    {
        if (line == std::string("<turbo_good_") + kSpeeds[s] + "=true>")
        {
            return 1u << s;
        }
    }
    return 0;
}

// one line inside <node> ... </node>
void readAiNodeLine(const std::string &line, AiNode &node)
{
    std::string value;
    if (tag(line, "id", value))
    {
        node.id = std::atoi(value.c_str());
    }
    else if (tag(line, "pos", value))
    {
        const auto position = readVec3(value);
        node.pos = rh(position[0], position[1], position[2]);
    }
    else if (tag(line, "down", value))
    {
        const auto down = readVec3(value);
        node.down = rh(down[0], down[1], down[2]);
    }
    else if (line.rfind("<turbo_good_", 0) == 0)
    {
        node.turboGood |= turboGoodBit(line);
    }
    else if (isForwardLink(line))
    {
        const size_t equals = line.find('=');
        if (equals != std::string::npos)
        {
            node.forward.push_back(std::atoi(line.c_str() + equals + 1));
        }
    }
    else if (line == "<flag_restart=true>")
    {
        node.restart = true;
    }
    else if (line == "<flag_first=true>")
    {
        node.first = true;
    }
    else if (line == "<corner_slow=true>")
    {
        node.cornerSlow = true;
    }
    else if (line == "<corner_hairpin=true>")
    {
        node.cornerHairpin = true;
    }
    else if (line == "<corner_straight=true>")
    {
        node.cornerStraight = true;
    }
    else if (line == "<env_inside=true>")
    {
        node.envInside = true;
    }
}

// config\ai_<t>.txt: <nodes> ... <node><id=><pos=x y z><forwardN=id>...</node>
std::vector<AiNode> readAiNodes(const std::string &path)
{
    std::vector<AiNode> nodes;
    AiNode node{};
    bool inNode = false, inNodes = false;
    for (auto &line : lines(path))
    {
        if (line == "<nodes>")
        {
            inNodes = true;
        }
        else if (line == "</nodes>")
        {
            break;
        }
        else if (!inNodes)
        {
            continue;
        }
        else if (line == "<node>")
        {
            node = AiNode{};
            inNode = true;
        }
        else if (line == "</node>")
        {
            if (inNode)
            {
                nodes.push_back(node);
            }
            inNode = false;
        }
        else if (inNode)
        {
            readAiNodeLine(line, node);
        }
    }
    return nodes;
}

void readCameraFlag(const std::string &value, TrackCamera &cam)
{
    cam.dynamicFov |= value == "DYNAMICFOV";
    cam.dynamicTracking |= value == "DYNAMICTRACKING";
    cam.onlyForward |= value == "ONLYFORWARD";
    cam.kerb |= value == "KERB";
    cam.onlyInFrame |= value == "ONLYINFRAME";
    cam.presetFov |= value == "PRESETFOV";
    cam.tightFov |= value == "TIGHTFOV";
}

void readCameraKey(const std::string &key, const std::string &value, TrackCamera &cam)
{
    if (key == "type")
    {
        cam.type = value;
    }
    else if (key == "name")
    {
        cam.name = value;
    }
    else if (key == "pos" || key == "start pos")
    {
        cam.pos = readVec3(value);
    }
    else if (key == "target" || key == "start target")
    {
        cam.target = readVec3(value);
    }
    else if (key == "end pos")
    {
        cam.pos2 = readVec3(value);
    }
    else if (key == "end target")
    {
        cam.target2 = readVec3(value);
    }
    else if (key == "fov" || key == "fov1")
    {
        cam.fov = readFloat(value);
    }
    else if (key == "fov2")
    {
        cam.fov2 = readFloat(value);
    }
    else if (key == "range")
    {
        cam.range = readFloat(value);
    }
    else if (key == "holdtime")
    {
        cam.hold = readFloat(value);
    }
    else if (key == "height")
    {
        cam.height = readFloat(value);
    }
    else if (key == "panrange")
    {
        std::sscanf(value.c_str(), "%f %f", &cam.panFrom, &cam.panTo);
    }
    else if (key == "flag")
    {
        readCameraFlag(value, cam);
    }
}

// "<key=value>" inside a camera block
void readCameraLine(const std::string &line, TrackCamera &cam)
{
    if (line.size() < 3 || line[0] != '<')
    {
        return;
    }
    const size_t eq = line.find('='), e = line.rfind('>');
    if (eq == std::string::npos || e == std::string::npos || e < eq)
    {
        return;
    }
    readCameraKey(line.substr(1, eq - 1), line.substr(eq + 1, e - eq - 1), cam);
}

// config\camera_<t>.txt: <camera> ... </camera> blocks of <key=value>
std::vector<TrackCamera> readCameras(const std::string &path)
{
    std::vector<TrackCamera> cameras;
    TrackCamera cam;
    bool inCamera = false;
    for (auto &line : lines(path))
    {
        if (line == "<camera>")
        {
            cam = TrackCamera{};
            inCamera = true;
        }
        else if (line == "</camera>")
        {
            if (inCamera)
            {
                cameras.push_back(cam);
            }
            inCamera = false;
        }
        else if (inCamera)
        {
            readCameraLine(line, cam);
        }
    }
    return cameras;
}

// config\<t>pup.txt: PUPSTART / TYPE / X v / Y v / Z v / PUPEND
std::vector<Pickup> readPickups(const std::string &path)
{
    std::vector<Pickup> pickups;
    const auto fileLines = lines(path);
    for (size_t i = 0; i + 5 < fileLines.size(); ++i)
    {
        if (fileLines[i] != "PUPSTART")
        {
            continue;
        }
        float x = 0, y = 0, z = 0;
        std::sscanf(fileLines[i + 2].c_str(), "X %f", &x);
        std::sscanf(fileLines[i + 3].c_str(), "Y %f", &y);
        std::sscanf(fileLines[i + 4].c_str(), "Z %f", &z);
        pickups.push_back({fileLines[i + 1], rh(x, y, z)});
        i += 5;
    }
    return pickups;
}

void readVisibility(const std::string &path, TrackData &td)
{
    std::vector<uint8_t> bytes;
    if (readFile(path, bytes))
    {
        td.visibility.read(bytes);
    }
}

} // namespace

TrackData loadTrackData(const std::string &game, const std::string &trackName)
{
    TrackData td;
    const std::string dir = game + "/data/tracks/" + trackName + "/";
    readBinary<original::LocatorHeader>(dir + "locator.dat", td, readStartSlots);
    readBinary<original::WaterHeader>(dir + "water.dat", td, readWaterBoxes);
    readBinary<original::QuadSpriteHeader>(dir + "sprite.dat", td, readSprites);
    readBinary<original::TrackProperties>(dir + "prop.dat", td, readProperties);
    readVisibility(dir + "visdata.vis", td);
    td.ai = readAiNodes(game + "/config/ai_" + trackName + ".txt");
    td.cameras = readCameras(game + "/config/camera_" + trackName + ".txt");
    td.pickups = readPickups(game + "/config/" + trackName + "pup.txt");
    return td;
}

} // namespace argon
