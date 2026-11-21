// Argon: the race recorded in the game's replay format (race_recording.hpp).
#include "logic/race_recording.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace
{
void writeFile(const std::string &path, const void *bytes, size_t size)
{
    std::ofstream out(path, std::ios::binary);
    out.write(static_cast<const char *>(bytes), std::streamsize(size));
    if (!out)
    {
        throw std::runtime_error("cannot write " + path);
    }
}
}  // namespace

void RaceRecording::start(const argon::replay::Info &lineup, float simClock, const std::array<float, kCrafts> &shields)
{
    clock_ = 0;
    shots_ = {};
    shields_ = shields;
    timing_ = Timing{simClock, 0, -1};
    writer_.start(lineup, clock_);
}

void RaceRecording::tick(bool introRunning, bool held)
{
    if (introRunning)
    {
        return;
    }
    clock_ += kStep;
    ++timing_.steps;
    if (timing_.goStep < 0 && !held)
    {
        timing_.goStep = timing_.steps;   // from it on the race's steps move the effects' clock
    }
}

void RaceRecording::recordFrame(const std::vector<argon::replay::CraftEvent> &crafts)
{
    writer_.recordFrame(clock_, crafts);
}

void RaceRecording::recordWeapon(const argon::replay::WeaponEvent &event)
{
    if (!writer_.recording())
    {
        return;
    }
    shots_.count(event);
    writer_.recordWeapon(event);
}

void RaceRecording::recordShield(int slot, float energy, int knockedOutBy)
{
    float &last = shields_[size_t(std::clamp(slot, 0, kCrafts - 1))];
    if (energy == last)
    {
        return;
    }
    last = energy;
    recordWeapon(argon::replay::shieldEvent(slot, energy, knockedOutBy));
}

void RaceRecording::stop()
{
    writer_.stop();
    lastRace_ = writer_.files();
    lastRaceTiming_ = timing_;
}

void RaceRecording::stopWithoutKeeping()
{
    writer_.stop();
}

bool RaceRecording::recording() const
{
    return writer_.recording();
}

bool RaceRecording::pastLongest() const
{
    return clock_ > kLongest;
}

float RaceRecording::clock() const
{
    return clock_;
}

size_t RaceRecording::frames() const
{
    return writer_.frames();
}

std::string RaceRecording::weaponsSummary() const
{
    return shots_.summary();
}

const argon::replay::RecordingFiles &RaceRecording::lastRace() const
{
    return lastRace_;
}

const RaceRecording::Timing &RaceRecording::lastRaceTiming() const
{
    return lastRaceTiming_;
}

bool RaceRecording::hasLastRace() const
{
    return !lastRace_.index.empty();
}

void RaceRecording::saveLastRace(const std::string &stem) const
{
    writeFile(stem + ".info", lastRace_.lineupText.data(), lastRace_.lineupText.size());
    writeFile(stem + ".index", lastRace_.index.data(), lastRace_.index.size());
    writeFile(stem + ".events", lastRace_.events.data(), lastRace_.events.size());
}
