// Argon: the race start. Grid intro camera + countdown (race_intro_start 0x43d60, race_intro_update
// 0x43de0, init 0x43be0 "Countdown"): docs\frontend\README.md "Race start".
//  - Grid intro (0x52af9d): only the first race after a load (0x1b9788, set by the init) and not in modes 3 / 4; the
//    camera shows craft (crafts - 1 - floor(t / 2)) for 2 s each, from the end of the craft list to the player;
//    e = 1 - (1 - fmod(t, 2) / 2)^3; eye = craft + its matrix x offset, looking at the craft, fov pi x 0.4: the first
//    shot (the last craft) from (0, 10, 100) to (2, 2, -4), the others from (-8 / 8, 1, 10) to (-2 / 2, 1, -3) (odd
//    index: +). A pad's A skips it. Then 0x3f1d0 resets the clock.
//  - Countdown on that clock: the models hudmodels\countdown3 / 2 / 1 / 0 ("GO", per language) in their own full
//    screen viewport (camera (0, 0, -10) at the origin, fov pi x 0.4; drawn here from z +10, see SceneFrame::drawCountdown); model k has a target time
//    T = 1 / 2 / 3 / 4 and a leave time L = 2 / 3 / 4 / 4.4: before T, v = T - t, after L, v = (L - t) x 0.4, else 0;
//    turned about y by 50 x v x 12 deg (leaving also pitched by sin(px x v x 11 deg)), placed at (-px v, -10 v when
//    leaving, -50 v), px = 70 / -70 / 70 / -70. At T the sounds SF_<track>_three / two / one / go (0xc38b0 /
//    0xc3920 / 0xc3990 / 0xc3a00) and at 1 the race music (music_set_mode(1)); past 4 every pad gets control
//    (0x78270) and the lap timers start (0x63a30); past 10 the group goes off.
#include "app/app.hpp"

namespace
{
const char *kCdSound[4] = {"three", "two", "one", "go"};
} // namespace

// the first race after a load: the grid intro, then the countdown
void App::startGridIntro()
{
    loadCountdownModels();
    raceStart.startIntro();
}

// a restart, or a mode without the grid intro: the countdown at once
void App::startRaceCountdown()
{
    loadCountdownModels();
    startCountdown();
}

void App::loadCountdownModels()
{
    countdownDigits.load(models, game, renderer.propPool);
}

// the countdown from 3; the music moves on to on_start.wav (0x44259)
void App::startCountdown()
{
    raceStart.startCountdown();
    audio.startRaceMusic();
}

bool App::raceHeld() const
{
    return raceStart.held();
}

// one 60 Hz step (stepDrive), before the crafts move
void App::stepRaceIntro(float dt)
{
    const bool skip = raceStart.introRunning() && session.playerDrive && input.readMenu().a;   // a pad's A skips the intro
    const RaceStart::Step happened = raceStart.step(dt, race.racerCount(), skip);
    if (happened.countdownStarted)
    {
        audio.startRaceMusic();
    }
    for (int digit = happened.firstDigit; digit < happened.firstDigit + happened.digits; ++digit)
    {
        audio.playOneShotNamed(kCdSound[digit]);
    }
}

// the craft the grid intro shows: crafts - 1 - floor(t / 2), from the end of the craft list to the player
int App::introCraft() const
{
    return raceStart.introCraft(race.racerCount());
}

int App::introCraftSlot() const
{
    const int idx = introCraft();
    return race.slotOf(idx);
}

// the grid intro camera (after driveCamera, so it wins)
void App::introCamera()
{
    if (!gridIntroRunning())
    {
        return;
    }
    const int racers = race.racerCount(), idx = introCraft();
    float shotClock = std::fmod(raceStart.introClock(), RaceStart::kIntroShotSeconds), ease = 1.f - std::pow(1.f - shotClock * 0.5f, 3.f);
    glm::vec3 offsetFrom, offsetTo;
    if (idx == racers - 1)
    {
        offsetFrom = {0.f, 10.f, 100.f};
        offsetTo = {2.f, 2.f, -4.f};
    }
    else if (idx & 1)
    {
        offsetFrom = {8.f, 1.f, 10.f};
        offsetTo = {2.f, 1.f, -3.f};
    }
    else
    {
        offsetFrom = {-8.f, 1.f, 10.f};
        offsetTo = {-2.f, 1.f, -3.f};
    }
    glm::vec3 offset = offsetFrom + (offsetTo - offsetFrom) * ease;
    glm::mat4 craftPose = craftPoses.slotMatrix(introCraftSlot());
    glm::vec3 p(craftPose[3]);
    viewOverride = true;
    const glm::vec3 eye = p + glm::vec3(craftPose[0]) * offset.x + glm::vec3(craftPose[1]) * offset.y + glm::vec3(craftPose[2]) * offset.z;
    viewShot = {eye, p, glm::vec3(0, 1, 0), 72.f};   // fov pi x 0.4
    freeCamera.position = eye;
}

