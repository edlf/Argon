// Argon: a race's score (race_score.hpp).
#include "logic/race_score.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace
{
std::string trimmed(const std::string &line)
{
    const size_t first = line.find_first_not_of(" \t\r"), last = line.find_last_not_of(" \t\r");
    return first == std::string::npos ? std::string() : line.substr(first, last - first + 1);
}

bool startsNumber(const std::string &text)
{
    return std::isdigit(static_cast<unsigned char>(text[0])) || text[0] == '-';
}

// trackgoals.txt names Brackenstowe in full, the second routes with their "alt_"
std::string goalsKey(const std::string &trackFolder)
{
    const bool secondRoute = trackFolder.rfind("alt_", 0) == 0;
    std::string name = secondRoute ? trackFolder.substr(4) : trackFolder;
    if (name == "bracken")
    {
        name = "brackenstowe";
    }
    return (secondRoute ? "alt_" : "") + name + "_lap_times";
}
}  // namespace

CashModifiers CashModifiers::parse(std::istream &file)
{
    CashModifiers cash;
    std::string name, line;
    while (std::getline(file, line))
    {
        const std::string text = trimmed(line);
        if (text.empty())
        {
            continue;
        }
        if (!name.empty() && startsNumber(text))
        {
            cash.values_[name] = std::atoi(text.c_str());
            name.clear();
        }
        else
        {
            name = text;
        }
    }
    return cash;
}

int CashModifiers::value(const std::string &name) const
{
    const auto found = values_.find(name);
    return found != values_.end() ? found->second : 0;
}

LapGoals parseLapGoals(std::istream &file, const std::string &trackFolder)
{
    LapGoals goals{};
    const std::string key = goalsKey(trackFolder);
    std::string line;
    while (std::getline(file, line))
    {
        if (line.rfind(key, 0) != 0)
        {
            continue;
        }
        if (std::getline(file, line))
        {
            std::stringstream values(line);
            for (float &goal : goals)
            {
                values >> goal;
            }
        }
        break;
    }
    return goals;
}

RaceScore scoreRace(int place, int points, int speedClass, int previousHigh, const CashModifiers &cash)
{
    static const char *const kPlaceBonus[6] = {"1st_bonus", "2nd_bonus", "3rd_bonus", "4th_bonus", "5th_bonus", "6th_bonus"};
    static const char *const kSpeedModifier[kSpeedClasses] = {
        "novice_speed_modifer", "amateur_speed_modifer", "expert_speed_modifer", "pro_speed_modifer", "qr_speed_modifer"};
    RaceScore score;
    score.place = place;
    score.points = points;
    score.bonus = place >= 1 && place <= 6 ? cash.value(kPlaceBonus[place - 1]) : 0;
    score.speedModifier = cash.value(kSpeedModifier[std::clamp(speedClass, 0, kSpeedClasses - 1)]);
    score.score = (score.bonus + score.points) * score.speedModifier;
    score.previousHigh = previousHigh;
    score.gain = std::max(0, score.score - previousHigh);
    return score;
}
