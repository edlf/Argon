// Argon: the race's results as the game fixes them at the player's finish (logic\results_rules.hpp).
#include "logic/results_rules.hpp"

#include <algorithm>

namespace argon::results
{

namespace
{
void appendIfAbsent(std::vector<int> &order, int racer)
{
    if (std::find(order.begin(), order.end(), racer) == order.end())
    {
        order.push_back(racer);
    }
}
}  // namespace

RowTimes rowTimes(const RacerProgress &racer, int raceLaps)
{
    if (racer.lapsDone >= raceLaps)
    {
        return {racer.finishedLapsTime, racer.bestLap};
    }
    if (racer.lapFraction <= 0.f || racer.lapTimeSoFar <= 0.f)
    {
        return {racer.finishedLapsTime + racer.lapTimeSoFar, racer.bestLap};
    }
    const float lapPace = racer.lapTimeSoFar / racer.lapFraction;
    if (racer.lapsDone <= 0)
    {
        return {lapPace * float(raceLaps), lapPace};
    }
    return {racer.finishedLapsTime + lapPace * float(raceLaps - racer.lapsDone), racer.bestLap};
}

void keepPlacesInOrder(std::vector<float> &raceTimesInOrder)
{
    for (size_t place = 1; place < raceTimesInOrder.size(); ++place)
    {
        const float ahead = raceTimesInOrder[place - 1];
        if (raceTimesInOrder[place] < ahead)
        {
            raceTimesInOrder[place] = ahead + kPlaceGap;
        }
    }
}

std::vector<int> finishOrder(const std::vector<int> &finished, const std::vector<int> &racingByPlace, const std::vector<int> &knockedOutFirstToLast)
{
    std::vector<int> order;
    for (int racer : finished)
    {
        appendIfAbsent(order, racer);
    }
    for (int racer : racingByPlace)
    {
        appendIfAbsent(order, racer);
    }
    for (auto it = knockedOutFirstToLast.rbegin(); it != knockedOutFirstToLast.rend(); ++it)
    {
        appendIfAbsent(order, *it);
    }
    return order;
}

}  // namespace argon::results
