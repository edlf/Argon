// Argon: a craft knocked out by weapons (craftdeath.cpp in the game, docs\weapons\weapons.explosions().md "Knock-out"):
//   cage        0x61f20 (race setup): the track model's NET_CAGE bone holds the knocked-out crafts; 0x58d20 puts the
//               craft there, upright and at rest (+0x1145 out)
//   camera      0x620e0 (the human player): orbits in from the cage towards the craft over 6 s
//   messages    0x624f0: "DAMAGE CRITICAL" / "Emergency Teleport" for 6 s, "You were destroyed by" + the killer
//   race end    0x620e0, single player: 10 s after the knock-out or on A, LOSEMENU (0x52afa8 = 2) without POSTABLE,
//               SCOREMENU or the tournament's progress
//   glow        0x61f20 hides the track's NET_GLOW bone (0xb97f0(bone, 0): bone flags +0x102 bit 0), craftdeath
//               (0x61fc0) shows it at a knock-out; it stays on for the rest of the race
// Not ported: the split screen's MULTIPLAYERENDMENU, the levels and cash kept for Race Again (0x1bc87c).
#include "app/app.hpp"
#include "ui/race_menus.hpp"

namespace
{
const float kMessageTime = 6.f;      // 0x1a36b0: the warnings, and the camera's fly-in
const float kLoseMenuDelay = 10.f;   // 0x1a35a4
const uint32_t kRed = 0xffff0000u, kYellow = 0xffffff00u, kWhite = 0xffffffffu;
const int kCentred = 2;
}  // namespace

std::optional<glm::mat4> App::netCage() const
{
    const int bone = track.findBone("NET_CAGE");
    if (bone < 0)
    {
        return std::nullopt;
    }
    return track.boneRest[size_t(bone)];
}

int App::hiddenNetGlowBone() const
{
    if (!session.driving || weapons.anyKnockedOut())
    {
        return -1;
    }
    return track.findBone("NET_GLOW");
}

void App::retireCraft(int slot)
{
    const auto cage = netCage();
    const Found<argon::GameCraft> craft = race.craftInSlot(slot);
    if (cage && craft)
    {
        craft->retire(glm::vec3((*cage)[3]));
    }
}

// 0x620e0 (single player): LOSEMENU 10 s after the knock-out, or at once on A
void App::stepPlayerKnockedOut()
{
    const bool skip = input.readMenu().a;
    if (playerKnockOut.secondsSince(simClock) <= kLoseMenuDelay && !skip)
    {
        return;
    }
    session.raceEnd = kLoseMenu;
    raceMenus->raceEnd().open();
    std::printf("race end: the player was knocked out by slot %d, LOSEMENU after %.2f s\n", playerKnockOut.by(), playerKnockOut.secondsSince(simClock));
    session.playerDrive = false;
    if (session.paused)
    {
        resumeRace();
    }
    audio.loadUi(game);
}

// 0x620e0: eye = the cage + its rotation x (20 - 30 f, 100 - 150 f, 120 - 110 f), f = 1 - (1 - t / 6)^3 up to 1,
// looking at the craft (up y, fov pi x 0.4)
argon::GameCraft::Tripod App::deathCamera() const
{
    const float u = 1.f - (playerKnockOut.secondsSince(simClock)) / kMessageTime;
    const float fall = std::min(1.f - u * u * u, 1.f);
    const glm::vec3 offset(20.f - 30.f * fall, 100.f - 150.f * fall, 120.f - 110.f * fall);
    const glm::mat4 cage = netCage().value_or(glm::mat4(1.f));
    return {glm::vec3(cage[3]) + glm::mat3(cage) * offset, sim.position(), glm::vec3(0.f, 1.f, 0.f)};
}

// 0x624f0 (the player's viewport, font 1 in single player, centred): while racing, for 6 s "DAMAGE CRITICAL" red at
// a quarter of its height and "Emergency Teleport" yellow at three quarters; "You were destroyed by\n<killer>" white
// at half
void App::drawKnockOutMessages()
{
    if (!weapons.craft(0).knockedOut() || session.raceEnd != kRacing || playerKnockOut.by() == PlayerKnockOut::kNobody)
    {
        return;
    }
    ImDrawList *drawList = ImGui::GetBackgroundDrawList();
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    const float pixelsPerUnit = ds.y / 480.f, x = ds.x / pixelsPerUnit * 0.5f;
    const FeCanvas canvas(painter, drawList, pixelsPerUnit, 0.f);
    if (playerKnockOut.secondsSince(simClock) < kMessageTime)
    {
        canvas.text({x, 120.f}, strings.race(0x51d, "DAMAGE CRITICAL"), {1, kRed, kCentred});
        canvas.text({x, 360.f}, strings.race(0x51e, "Emergency Teleport"), {1, kYellow, kCentred});
    }
    const Found<const argon::GameCraft> killer = race.craftInSlot(playerKnockOut.by());
    canvas.text({x, 240.f}, strings.race(0x514, "You were destroyed by"), {1, kWhite, kCentred});
    if (killer)
    {
        canvas.text({x, 240.f + painter.font(1).line}, racerName(killer->pilot()), {1, kWhite, kCentred});
    }
}
