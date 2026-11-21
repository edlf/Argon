#include "saves/lap_records.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

#include "files/utf.hpp"

namespace fs = std::filesystem;
using nlohmann::json;

namespace
{

bool validEntry(int track, int speed, float time)
{
    return track >= 0 && track < LapRecordBook::kTracks && speed >= 0 && speed < LapRecordBook::kSpeeds && time > 0.f;
}

json readJson(const std::string &file)
{
    std::ifstream stream{fs::path(file)};
    if (!stream)
    {
        return json::object();
    }
    try
    {
        return json::parse(stream);
    }
    catch (const json::exception &e)
    {
        std::fprintf(stderr, "lap records: %s: %s\n", file.c_str(), e.what());
        return json::object();
    }
}

// speed + 5 x track, both kept to the table
size_t indexOf(int track, int speed)
{
    return size_t(std::clamp(speed, 0, kLapTableSpeeds - 1) + kLapTableSpeeds * std::clamp(track, 0, kLapTableTracks - 1));
}

} // namespace

LapRecord &LapRecordBook::at(int track, int speed)
{
    return records_[indexOf(track, speed)];
}

const LapRecord &LapRecordBook::at(int track, int speed) const
{
    return records_[indexOf(track, speed)];
}

// {"lapRecords": [{"track": 0, "speed": 1, "time": 55.816, "name": "PLAYER1"}, ...]}; entries that do not fit are
// skipped
void LapRecordBook::load(const std::string &file)
{
    for (LapRecord &record : records_)
    {
        record = LapRecord{};
    }
    const json root = readJson(file);
    if (!root.contains("lapRecords") || !root["lapRecords"].is_array())
    {
        return;
    }
    for (const json &entry : root["lapRecords"])
    {
        const int track = entry.value("track", -1), speed = entry.value("speed", -1);
        const float time = entry.value("time", 0.f);
        if (!validEntry(track, speed, time))
        {
            continue;
        }
        LapRecord &record = at(track, speed);
        record.time = time;
        record.name = utf16(entry.value("name", std::string()));
    }
}

void LapRecordBook::save(const std::string &file) const
{
    json entries = json::array();
    for (int track = 0; track < kTracks; ++track)
    {
        for (int speed = 0; speed < kSpeeds; ++speed)
        {
            const LapRecord &record = at(track, speed);
            if (record.set())
            {
                entries.push_back({{"track", track}, {"speed", speed}, {"time", record.time}, {"name", utf8(record.name)}});
            }
        }
    }
    std::error_code ec;
    fs::create_directories(fs::path(file).parent_path(), ec);
    std::ofstream(fs::path(file), std::ios::trunc) << json{{"lapRecords", entries}}.dump(2) << '\n';
}

ProfileBestLaps::ProfileBestLaps()
{
    times_.fill(kUnsetLap);
}

ProfileBestLaps ProfileBestLaps::read(const std::string &text)
{
    ProfileBestLaps laps;
    size_t at = 0;
    for (size_t index = 0; index < laps.times_.size() && at < text.size(); ++index)
    {
        size_t end = text.find(',', at);
        if (end == std::string::npos)
        {
            end = text.size();
        }
        laps.times_[index] = float(std::atof(text.substr(at, end - at).c_str()));
        at = end + 1;
    }
    return laps;
}

float ProfileBestLaps::at(int track, int speed) const
{
    return times_[indexOf(track, speed)];
}

void ProfileBestLaps::set(int track, int speed, float time)
{
    times_[indexOf(track, speed)] = time;
}

// "%.3f," per time, as the profiles have always been written
std::string ProfileBestLaps::text() const
{
    std::string text;
    for (float time : times_)
    {
        char value[24];
        std::snprintf(value, sizeof value, "%.3f,", time);
        text += value;
    }
    return text;
}
