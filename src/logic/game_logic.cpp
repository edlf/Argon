// Argon: the game modes' logic (logic\game_logic.hpp)
#include "logic/game_logic.hpp"

#include <algorithm>

#include "effects/craft_exhausts.hpp"
#include "craft/craftsim.hpp"
#include "logic/game_progress.hpp"
#include "logic/progress_keeper.hpp"
#include "logic/tourney_win.hpp"
#include "logic/tourney_rules.hpp"
#include "ui/fe_shell.hpp"

void GameLogic::characterPicked()
{
    pages.openCourseSelect();
}

void GameLogic::speedPicked(int pilot, int track)
{
    host.startQuickRace(pilot, track);
}

int GameLogic::loaderHint() const
{
    return kRaceHintBase + sources.raceHint;
}

void GameLogic::nextLoaderHint()
{
    sources.raceHint = (sources.raceHint + 1) % kRaceHints;
}

int GameLogic::pilot() const
{
    return std::clamp(sources.loadPilot, 0, 15);
}

// ---- Tournament
void TournamentLogic::characterPicked()
{
    pages.openTourneyType();
}

int TournamentLogic::pilot() const
{
    return std::clamp(sources.tourney.character, 0, 15);
}

HighScoreTable &TournamentLogic::highScores() const
{
    return sources.progress.hiTourney;
}

void TournamentLogic::earnCash(int character, int amount)
{
    sources.progress.cash[std::clamp(character, 0, 15)] += amount;
}

int TournamentLogic::droneSeats(int chosen) const
{
    return argon::tourney::droneSeats(
        sources.progress.tourneyLevel, sources.rules.tourneyDone(sources.progress.tourneyLevel, sources.tourney.character), chosen);
}

// a race of the tournament ends: won, it counts; the last race won wins the tournament (0x84160) and its awards
void TournamentLogic::raceFinished(bool won)
{
    TourneyRun &run = sources.tourney;
    argon::GameProgress &progress = sources.progress;
    run.finished = false;
    if (!won)
    {
        return;
    }
    int &racesWon = progress.tourney[run.character][progress.tourneyLevel];
    if (racesWon < 0)
    {
        racesWon = 0;
    }
    if (racesWon + 1 >= argon::tourney::racesIn(progress.tourneyLevel))
    {
        run.awards = argon::tourney::winTournament(progress, progress.tourneyLevel, run.character);
        racesWon = -1;
        run.finished = true;
        run.awardsPending = true;   // WINMENU 0xa1bf9: the way back through FE MAIN shows FE AWARDS
    }
    else
    {
        ++racesWon;
    }
    sources.rules.storeProgress();
}

int TournamentLogic::loaderHint() const
{
    return kHintBase + hint;
}

void TournamentLogic::nextLoaderHint()
{
    hint = (hint + 1) % kHints;
}

// ---- Quick Race
// the home track's row: the character's id & 7 (a nemesis 8..15 shares its original's track)
void QuickRaceLogic::characterPicked()
{
    sources.track = sources.character & 7;
    GameLogic::characterPicked();
}

// the home track's route: the character's id >> 3 (a nemesis races its original's track's second route)
int QuickRaceLogic::routeChoiceDefault(int row) const
{
    return row == (sources.character & 7) ? sources.character >> 3 : 0;
}

HighScoreTable &QuickRaceLogic::highScores() const
{
    return sources.progress.hiQuick;
}

// ---- Time Attack
void TimeAttackLogic::speedPicked(int pilot, int track)
{
    pages.openGhostSelect(pilot, track);
}

int TimeAttackLogic::loaderHint() const
{
    return kHintBase + hint;
}

void TimeAttackLogic::nextLoaderHint()
{
    hint = (hint + 1) % kHints;
}

HighScoreTable &MultiplayerLogic::highScores() const
{
    return sources.progress.hiQuick;
}

HighScoreTable &TimeAttackLogic::highScores() const
{
    return sources.progress.hiQuick;
}
