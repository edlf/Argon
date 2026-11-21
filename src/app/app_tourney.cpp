// Argon: Tournament mode (docs\frontend\tournament.md "Tournament"). FE TOURNEYTYPE (vtable 0x19dfc0: build 0x35110,
// enter 0x36320, input 0x35940, update 0x35e50, ticker 0x365b0, start 0x35b20), the track order (0x79130 with the
// table 0x1b9438 and 0x791f0), the race counts (0xa1320), the progress per character and level (0x1bb0f4 +
// (character x 44 + level) x 4; only a won race moves it on, 0x95dc7) and the tournament win (0x84160).

#include "app/app.hpp"
#include "ui/fe_shell.hpp"
#include "movie/story_scene.hpp"
#include "movie/story_scenes.hpp"
#include "logic/tourney_rules.hpp"
#include "logic/tourney_win.hpp"

void App::startTourneyRace(int level)
{
    modes.select(modes.tournament());
    progress.tourneyLevel = level;
    picks.speedClass = progress.tourneyLevel;   // 0x35b20: the game speed = the level
    tourneyRun.character = picks.character;
    startQuickRace(picks.character, progressKeeper.nextTourneyTrack(progress.tourneyLevel, picks.character));
}

// the race page's leave (0x7fa50, 0x7fac4): after winning a tournament's last race (progress -1) above Novice, the
// beat-the-nemesis or winning scene (movie\story_scenes.hpp) plays over the race's track before the front end; A or
// START on any pad ends it (0x7fdc0)
bool App::playRaceEndScene(int page)
{
    const bool tournamentWon = &logic() == &modes.tournament() && progress.tourney[tourneyRun.character][progress.tourneyLevel] < 0;
    if (story->playing() || !tournamentWon || trackIndex < 0)
    {
        return false;
    }
    const std::optional<std::string> scene = argon::story::raceEndScene(progress.tourneyLevel, picks.loadTrackId, courseFolder(picks.loadTrackId));
    if (!scene || !story->start(courseFolder(picks.loadTrackId), *scene))
    {
        return false;
    }
    session.raceEndScenePage = page;
    session.driving = false;
    session.playerDrive = false;
    audio.holdRaceSounds();   // 0x7fb75: the race's sounds stop under the scene
    startTransition();
    return true;
}

void App::updateRaceEndScene()
{
    story->update();
    if (input.readMenu().a)
    {
        story->skip();
    }
    if (!story->finished())
    {
        return;
    }
    story->stop();
    audio.resumeRaceSounds();
    leaveRaceFor(session.raceEndScenePage);
}
