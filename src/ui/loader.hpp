// Argon: the race loading screen LOADER (drawn by the loader thread 0x808e0), between the front end and the
// race (ui\loader.cpp; the load itself: app_loader.cpp)
#pragma once
#include "logic/player_pads.hpp"
#include "ui/fe_canvas.hpp"
#include <array>
#include <string>
#include "input/player_input.hpp"
#include "logic/game_progress.hpp"
#include "logic/multiplayer_session.hpp"
#include "logic/tourney_win.hpp"
#include "ui/fe_page.hpp"
#include "ui/page_hosts.hpp"

#include <memory>

class LoadingFlyby;
class GameModes;
class ReplayRace;
class StoryScene;
#include "ui/nemesis_intro.hpp"

// what the loading screen drives and reads: the race load and flyby, the story scene before a tournament race, the
// race being loaded, the players' pads
struct LoadingSources
{
    RaceLoading &loading;
    std::unique_ptr<StoryScene> &story;   // made after the shell
    LoadingFlyby &flyby;                   // the <t>_glory.nms flyby behind the screen
    const int &pilot, &track;              // the race being loaded (game ids)
    const PlayerInput &players;
    const MultiplayerSession &multiplayer;
    GameModes &modes;
    const argon::GameProgress &progress;
    const TourneyRun &tourney;
    const ReplayRace &replay;              // the attract demo shows no hints
    const UnlockRules &rules;
    const std::string &gameFolder;
    const PlayerPads &pads;                // the players' ports in a multiplayer race
};

class LoaderPage : public FePage
{
  public:
    LoaderPage(const PageKit &kit, const LoadingSources &sources) : FePage(kit), sources(sources)
    {
    }
    void open();   // App::startQuickRace: the track loads once the screen has been shown
    void showClockAt(float seconds)
    {
        clockShift = seconds;
    }   // tests (--loader-at): the loader's clock starts there

    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    bool bare() const override
    {
        return true;
    }

  private:
    LoadingSources sources;
    enum class Phase
    {
        Showing,
        Loading,
        Loaded,
        Story
    };   // Loaded: "Press [BUTA] to continue."; Story: the tournament's scene before the race (loader_run 0x80354)
    float seconds() const;   // since it opened (+0x1a80)
    bool showingNemesisIntro() const
    {
        return nemesisRace && menu::NemesisIntro::showing(seconds());
    }   // +0x1a84
    void drawNemesisIntro(const FeCanvas &canvas) const;
    void drawIntroText(const FeCanvas &canvas, const menu::IntroText &text) const;
    void drawTitleBox(const FeCanvas &canvas) const;
    void drawHint(const FeCanvas &canvas) const;
    void drawStatus(const FeCanvas &canvas) const;
    uint32_t statusColour() const;
    bool loadsTheAttractDemo() const;
    void playersInput();
    void drawPlayerList(const FeCanvas &canvas) const;
    std::array<bool, 4> playerReady{};   // multiplayer: pressed A (+0x1a8e + controller)
    void findStory();
    void startStory();
    void playStory(const menu::MenuInput &in);
    void drawStoryLoading(const FeCanvas &canvas) const;
    std::string storyFolder, storyFile;   // data\movies\<folder>\<file>
    bool storyPending = false;            // +0x1a70: the scene plays once the track has loaded
    int storyEndedStep = -1;              // the shell step the scene ended at (-1: playing)

    Phase phase = Phase::Showing;
    int framesShown = 0;   // drawn since it opened: the load waits until the screen is up
    int openedAtStep = 0;  // the shell's 60 Hz clock when it opened
    float clockShift = 0.f;
    bool nemesisRace = false;   // 0x80450: a tournament's race against the nemesis opens with the intro
};
