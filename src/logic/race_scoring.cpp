// Argon: the race's points and score (logic\race_scoring.hpp).
#include "logic/race_scoring.hpp"

#include "files/game_path.hpp"

#include <fstream>

void RaceScoring::load(const std::string &gameFolder, const std::string &trackFolder)
{
    std::ifstream cashFile(argon::findGamePath(gameFolder + "/config/cashmodifiers.txt"));
    cash = CashModifiers::parse(cashFile);
    std::ifstream goals(argon::findGamePath(gameFolder + "/config/trackgoals.txt"));
    lapGoals = parseLapGoals(goals, trackFolder);
}
