// Argon: reading the game's replays (logic\replay.hpp, docs\replays\README.md).
#include "logic/replay.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>

#include <glm/gtc/type_ptr.hpp>

#include "original/event_buffer_records.hpp"

namespace argon::replay
{
namespace
{

// the names fe_shell_start_attract 0x31c10 knows, in id order
constexpr std::string_view kTrackNames[] = {"silver",
                                            "kalau",
                                            "fuusoku",
                                            "flames",
                                            "solar",
                                            "bracken",
                                            "mount",
                                            "draco",
                                            "alt_silver",
                                            "alt_kalau",
                                            "alt_fuusoku",
                                            "alt_flames",
                                            "alt_solar",
                                            "alt_bracken",
                                            "alt_mount",
                                            "alt_draco",
                                            "mars",
                                            "alt_mars"};
constexpr std::string_view kPilotNames[] = {
    "tara", "yuri", "kumiko", "darius", "jordan", "marcus", "leah", "kukai", "alicia", "vladimir", "abi", "zafir", "kaitlyn", "jaya", "lukas", "kutoo"};

using original::EventChunkHeader;
using original::EventCraftEntry;
using original::EventEntryHeader;
using original::EventIndexEntry;
using original::EventWeaponSignal;

template <class Names> int indexOfName(const Names &names, std::string_view name)
{
    auto it = std::find(std::begin(names), std::end(names), name);
    return it == std::end(names) ? -1 : int(it - std::begin(names));
}

class ByteReader
{
  public:
    explicit ByteReader(const std::vector<uint8_t> &bytes) : bytes_(bytes)
    {
    }
    template <class T> T at(size_t offset) const
    {
        if (offset + sizeof(T) > bytes_.size())
        {
            throw std::runtime_error("replay: read past the end of the file at offset " + std::to_string(offset));
        }
        T value;
        std::memcpy(&value, bytes_.data() + offset, sizeof(T));
        return value;
    }
    size_t size() const
    {
        return bytes_.size();
    }

  private:
    const std::vector<uint8_t> &bytes_;
};

// 0x423b0: rows 0 and 1 from six shorts (one per dword), row 2 = row 0 x row 1; columns right, up, forward
glm::mat3 unpackRotation(const int32_t (&packedRows)[2][3])
{
    glm::vec3 rows[3];
    for (int r = 0; r < 2; ++r)
    {
        for (int c = 0; c < 3; ++c)
        {
            rows[r][c] = float(int16_t(packedRows[r][c])) * original::kEventOrientationScale;
        }
    }
    rows[2] = glm::cross(rows[0], rows[1]);
    return glm::transpose(glm::mat3(rows[0], rows[1], rows[2]));
}

CraftEvent readCraftEvent(const ByteReader &in, size_t at, float time)
{
    CraftEvent event;
    event.time = time;
    const auto entry = in.at<EventCraftEntry>(at);
    event.pos = glm::make_vec3(entry.position);
    event.vel = glm::make_vec3(entry.velocity);
    event.angularMomentum = glm::make_vec3(entry.angularMomentum);
    event.rotation = unpackRotation(entry.rotationRows);
    event.controls = entry.controls;
    return event;
}

WeaponEvent readWeaponEvent(const ByteReader &in, size_t at, float time)
{
    WeaponEvent event;
    event.time = time;
    const auto entry = in.at<EventWeaponSignal>(at);
    event.kind = entry.weapon.kind;
    event.projectile = entry.weapon.projectile;
    event.weapon = entry.weapon.slot;
    event.craft = entry.weapon.craft;
    event.extra = entry.weapon.extra;
    event.sequence = entry.weapon.sequence;
    event.signal = entry.signal;
    static_assert(sizeof entry == std::tuple_size_v<decltype(event.raw)>);
    std::memcpy(event.raw.data(), &entry, sizeof entry);
    return event;
}

// the craft a continuous entry is addressed to ("craft<i>"), -1 for another name
int craftOfTarget(uint32_t target)
{
    for (int i = 0; i < kMaxCrafts; ++i)
    {
        if (nameHash("craft" + std::to_string(i)) == target)
        {
            return i;
        }
    }
    return -1;
}

class ChunkReader
{
  public:
    ChunkReader(const ByteReader &in, Recording &out) : in_(in), out_(out), weaponTarget_(nameHash("WeaponEvents"))
    {
    }

    // FUN_000db090: a chunk's header {size, time, continuous count, critical count} and its entries
    void read(const IndexEntry &frame)
    {
        const auto header = in_.at<EventChunkHeader>(frame.offset);
        const uint32_t count = header.continuousCount + header.criticalCount;
        size_t at = frame.offset + sizeof(EventChunkHeader);
        for (uint32_t i = 0; i < count; ++i)
        {
            at += readEntry(at, frame.time);
        }
        if (at != size_t(frame.offset) + header.size)
        {
            throw std::runtime_error("replay: the chunk at offset " + std::to_string(frame.offset) + " does not end at its size");
        }
    }

  private:
    size_t readEntry(size_t at, float time)
    {
        const auto head = in_.at<EventEntryHeader>(at);
        const uint32_t type = head.type, size = head.size;
        if (type == original::kEventContinuous && size == sizeof(EventCraftEntry))
        {
            addCraftEvent(craftOfTarget(head.targetHash), readCraftEvent(in_, at, time));
        }
        else if (type == original::kEventCritical && size == sizeof(EventWeaponSignal) && head.targetHash == weaponTarget_)
        {
            out_.weapons.push_back(readWeaponEvent(in_, at, time));
        }
        else
        {
            throw std::runtime_error("replay: an unknown entry (type " + std::to_string(type) + ", size " + std::to_string(size) + ") at offset " +
                                     std::to_string(at));
        }
        return size;
    }

    void addCraftEvent(int craft, const CraftEvent &event)
    {
        if (craft < 0)
        {
            throw std::runtime_error("replay: a craft entry for an unknown object");
        }
        out_.crafts[size_t(craft)].push_back(event);
    }

    const ByteReader &in_;
    Recording &out_;
    const uint32_t weaponTarget_;
};

std::vector<uint8_t> readFile(const std::filesystem::path &path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
    {
        throw std::runtime_error("replay: cannot open " + path.string());
    }
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

bool isBefore(float time, const CraftEvent &event)
{
    return time < event.time;
}

glm::quat rotationOf(const CraftEvent &event)
{
    return glm::normalize(glm::quat_cast(event.rotation));
}

}  // namespace

uint32_t nameHash(std::string_view name)
{
    const size_t whole = name.size() & ~size_t(3);
    uint32_t h = 0;
    for (size_t i = 0; i < whole; i += 4)
    {
        uint32_t word;
        std::memcpy(&word, name.data() + i, 4);
        h ^= word + uint32_t(i);
    }
    for (size_t k = 0; k < (name.size() & 3); ++k)
    {
        h ^= uint32_t(int32_t(int8_t(name[name.size() - 1 - k])) + int32_t(k));
    }
    return h;
}

int Info::craftCount() const
{
    int count = 0;
    for (int pilot : pilots)
    {
        count += pilot >= 0 ? 1 : 0;
    }
    return count;
}

int trackIdNamed(std::string_view name)
{
    return indexOfName(kTrackNames, name);
}

int pilotIdNamed(std::string_view name)
{
    return indexOfName(kPilotNames, name);
}

std::string_view trackName(int id)
{
    return id >= 0 && id < int(std::size(kTrackNames)) ? kTrackNames[id] : std::string_view();
}

std::string_view pilotName(int id)
{
    return id >= 0 && id < int(std::size(kPilotNames)) ? kPilotNames[id] : std::string_view();
}

Info parseInfo(std::string_view text)
{
    Info lineup;
    std::istringstream lines{std::string(text)};
    std::string key, value;
    while (lines >> key >> value)
    {
        if (key == "track")
        {
            lineup.track = trackIdNamed(value);
        }
        else if (key.rfind("craft", 0) == 0)
        {
            const int craft = std::atoi(key.c_str() + 5);
            if (craft >= 0 && craft < kMaxCrafts)
            {
                lineup.pilots[size_t(craft)] = pilotIdNamed(value);
            }
        }
    }
    return lineup;
}

std::vector<IndexEntry> parseIndex(const std::vector<uint8_t> &bytes)
{
    const ByteReader in(bytes);
    std::vector<IndexEntry> index;
    for (size_t at = 0; at + sizeof(EventIndexEntry) <= in.size(); at += sizeof(EventIndexEntry))
    {
        const auto entry = in.at<EventIndexEntry>(at);
        if (entry.time == original::kEventIndexEnd)
        {
            break;
        }
        index.push_back({entry.time, entry.offset});
    }
    return index;
}

Recording parseRecording(std::string_view lineupText, const std::vector<uint8_t> &index, const std::vector<uint8_t> &events)
{
    Recording recording;
    recording.lineup = parseInfo(lineupText);
    recording.index = parseIndex(index);
    const ByteReader in(events);
    ChunkReader chunks(in, recording);
    for (const IndexEntry &frame : recording.index)
    {
        chunks.read(frame);
    }
    return recording;
}

Recording loadRecording(const std::filesystem::path &folder, const std::string &stem)
{
    const std::vector<uint8_t> lineupFile = readFile(folder / (stem + ".info"));
    return parseRecording(std::string_view(reinterpret_cast<const char *>(lineupFile.data()), lineupFile.size()),
                          readFile(folder / (stem + ".index")),
                          readFile(folder / (stem + ".events")));
}

glm::mat4 Pose::matrix() const
{
    glm::mat4 pose = glm::mat4_cast(rot);
    pose[3] = glm::vec4(pos, 1.f);
    return pose;
}

glm::vec3 easedStartPosition(const glm::vec3 &current, const glm::vec3 &recorded, float clock)
{
    return clock < kEaseSeconds ? current + (recorded - current) * kEaseShare : recorded;
}

bool poseAt(const std::vector<CraftEvent> &events, float seconds, Pose &out)
{
    if (events.empty() || seconds < events.front().time)
    {
        return false;
    }
    auto next = std::upper_bound(events.begin(), events.end(), seconds, isBefore);
    const CraftEvent &before = *(next - 1);
    out.controls = before.controls;
    out.vel = before.vel;
    if (next == events.end())
    {   // past the last entry: dead reckoning on the position, as the game (0xd06f0: x + v dt)
        out.pos = before.pos + before.vel * (seconds - before.time);
        out.rot = rotationOf(before);
        return true;
    }
    const CraftEvent &after = *next;
    const float share = (seconds - before.time) / (after.time - before.time);
    out.pos = glm::mix(before.pos, after.pos, share);
    out.rot = glm::slerp(rotationOf(before), rotationOf(after), share);
    return true;
}

}  // namespace argon::replay
