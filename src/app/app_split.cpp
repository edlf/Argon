// Argon: the split screen (docs\frontend\multiplayer.md "Multiplayer"): a view per human player (logic\split_screen.hpp),
// each on its own craft's chase camera.
#include "app/app.hpp"
#include "hud/game_hud.hpp"
#include "ui/race_menus.hpp"

// players 2..4 (racers 1..3): their craft's tripod between its last two steps, as camera0's view 0..3 sets it (fov 60,
// +0xcc4)
ViewShot App::chaseShotOf(int player) const
{
    const Race::Drone &drone = race.grid()[size_t(player - 1)];
    const float blend = craftPoses.simAlpha();
    const glm::vec3 up = glm::mix(drone.camPrev.up, drone.camCur.up, blend);
    return ViewShot{glm::mix(drone.camPrev.pos, drone.camCur.pos, blend), glm::mix(drone.camPrev.look, drone.camCur.look, blend),
                    glm::length(up) > 1e-4f ? glm::normalize(up) : glm::vec3(0, 1, 0), ViewCameras::kChaseFovDeg};
}

// each player's HUD in their view (hud0..hud3, 0x3b0b0)
void App::renderHuds()
{
    for (const std::unique_ptr<GameHud> &hud : huds)
    {
        hud->render();
    }
}

// FE MAIN's Multiplayer (0x28e90) as far as the race: game mode 4 for that many players, no picks yet; the laps are set
// when the race loads (App::setRaceLaps)
void App::startMultiplayer(int players)
{
    modes.select(modes.multiplayer());
    multiplayer.start(players);
}

// one player again (FE MAIN's other modes): one view, no picks
void App::leaveMultiplayer()
{
    multiplayer.leave();
}

namespace
{
// the players' knock-outs (racers 0..players - 1 that the race has)
class PlayersOut
{
  public:
    PlayersOut(const Race &race, int players) : count_(std::min({players, race.racerCount(), argon::split::kMaxPlayers}))
    {
        for (int p = 0; p < count_; ++p)
        {
            out_[size_t(p)] = race.knockedOut(p);
        }
    }
    std::span<const bool> flags() const
    {
        return {out_.data(), size_t(count_)};
    }

  private:
    int count_;
    std::array<bool, argon::split::kMaxPlayers> out_{};
};
}  // namespace

argon::multiplayer::Standing App::multiplayerStanding() const
{
    return argon::multiplayer::standingOf(multiplayer.players(), race.finishOrder(), PlayersOut(race, multiplayer.players()).flags());
}

// 0x41de0 / 0x620e0 (logic\multiplayer_rules.hpp)
void App::stepMultiplayerEnd()
{
    const argon::multiplayer::Standing standing = multiplayerStanding();
    if (argon::multiplayer::raceOver(standing))
    {
        endMultiplayerRace(standing);
    }
}

// MULTIPLAYERENDMENU (0x52afa8 = 7): the last one racing joins the finish order ("By Default"), the winner's Wins
// count goes up, the crafts fly on under the AI behind the menu
void App::endMultiplayerRace(const argon::multiplayer::Standing &standing)
{
    if (argon::multiplayer::winsByDefault(standing))
    {
        multiplayer.setDefaultWinner(
            argon::multiplayer::lastRacing(multiplayer.players(), race.finishOrder(), PlayersOut(race, multiplayer.players()).flags()));
        race.finishByPlace();
    }
    const std::vector<int> &order = race.finishOrder();
    if (!order.empty() && order.front() < argon::split::kMaxPlayers && !race.knockedOut(order.front()))
    {
        multiplayer.countWin(order.front());
    }
    std::printf("race end: multiplayer, %d players%s (finish order:", standing.players, multiplayer.defaultWinner() >= 0 ? ", by default" : "");
    for (int racer : order)
    {
        std::printf(" %d", racer);
    }
    std::printf(")\n");
    session.raceEnd = kMultiplayerEnd;
    session.playerDrive = false;
    raceMenus->multiplayerEnd().open();
}
