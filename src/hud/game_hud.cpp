// Argon: the in-race HUD, CGameHud (hud\game_hud.hpp): layout 0x45270, update 0x496f0, render 0x4a100
#include "hud/game_hud.hpp"
#include "ui/lap_time_text.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <imgui.h>

#include "app/weapons.hpp"
#include "files/string_table.hpp"
#include "hud/hud_canvas.hpp"
#include "hud/hud_painter.hpp"
#include "logic/game_logic.hpp"
#include "logic/multiplayer_session.hpp"
#include "logic/race.hpp"
#include "logic/race_eggs.hpp"
#include "logic/race_lap_records.hpp"
#include "logic/race_scoring.hpp"
#include "logic/race_warnings.hpp"
#include "render/gpu_model.hpp"
#include "saves/viewer_settings.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/pause_rules.hpp"
#include "ui/race_end_page.hpp"
#include "hud/minimap_rules.hpp"
#include "hud/shield_bar_rules.hpp"
#include "hud/speedometer_rules.hpp"

namespace
{
constexpr uint32_t kCyan = 0xff80d0ff, kWhite = 0xffffffff, kAmber = 0xffffc864, kOrange = 0xffff8000, kWarningRed = 0xffff4040;
constexpr float kD2R = 3.14159265f / 180.f;
constexpr double kVblanksPerSecond = 60.0;   // 0x439df0, counted by the vertical blank callback 0xb2670

std::u16string number(const char *format, double v)
{
    char text[64];
    std::snprintf(text, sizeof text, format, v);
    return argon::widenLatin1(text);
}
std::u16string integer(int v)
{
    char text[32];
    std::snprintf(text, sizeof text, "%d", v);
    return argon::widenLatin1(text);
}

// the turbo fan's rim point at `degrees` around its centre: the screen point and the texture's uv there
struct FanPoint
{
    ImVec2 at, uv;
};

FanPoint fanPoint(const HudCanvas &canvas, const ImVec2 &centre, float degrees)
{
    const float cosine = std::cos(degrees * kD2R), sine = std::sin(degrees * kD2R);
    return FanPoint{canvas.at(centre.x + 96.f * (cosine + sine), centre.y + 96.f * (cosine - sine)), ImVec2(0.5f + 0.5f * (cosine + sine), 1.f + cosine - sine)};
}

struct Needle
{
    ImVec2 pivot;
    float value, length, width;   // value: the speedometer's 0..270 degree reading
    uint32_t colour;
};

// a needle quad from the pivot along (sin, cos) of 270 - value degrees
void drawNeedle(const HudCanvas &canvas, const HudTexture &texture, const Needle &needle)
{
    const float angle = (90.f - needle.value + 180.f) * kD2R;
    const ImVec2 along(std::sin(angle), std::cos(angle)), side(along.y, -along.x);
    const ImVec2 tip(needle.pivot.x + along.x * needle.length, needle.pivot.y + along.y * needle.length);
    const float half = needle.width * 0.5f;
    canvas.drawList()->AddImageQuad(texture.image,
                               canvas.at(tip.x - side.x * half, tip.y - side.y * half),
                               canvas.at(tip.x + side.x * half, tip.y + side.y * half),
                               canvas.at(needle.pivot.x + side.x * half, needle.pivot.y + side.y * half),
                               canvas.at(needle.pivot.x - side.x * half, needle.pivot.y - side.y * half),
                               ImVec2(0, 0),
                               ImVec2(1, 0),
                               ImVec2(1, 1),
                               ImVec2(0, 1),
                               imColour(needle.colour));
}
}  // namespace

// CGameHud_ctor 0x448b0: each element's font, flags and colour (layout 0x45270 places them)
GameHud::GameHud(GameHudHost &host, const GameHudSources &sources, int player)
    : host(host), sources(sources), player(player), lapLabel(0, 1, kCyan), lapCount(1, 0x1001, kWhite), timeLabel(0, 1, kCyan), prevLabel(0, 1, kCyan),
      bestLabel(0, 1, kCyan), timeValue(0, 1, kWhite), prevValue(0, 1, kAmber), bestValue(0, 1, kAmber), scoreLabel(0, 0, kCyan), scoreValue(1, 0, kWhite),
      shieldLabel(0, 0, kCyan), shieldValue(0, 0, kWhite), speedoOverlay("speedo_overlay", 192.f, 96.f), turboText(0, 0, kWhite), turboLabel(0, 1, kWhite),
      speedValue(1, 1, kWhite), speedUnit(0, 1, kCyan), resetMessage(1, 2, kOrange), recordMessage(1, 2, kWhite), wrongWayMessage(1, 2, kOrange),
      attackWarning(0, 2, kWarningRed)
{
}

// the game lays its HUD out on a 640 x 480 virtual screen; the viewer keeps that height and widens it for other aspects.
// The viewer's HUD Size draws its units smaller and the screen as more of them: the elements and the safe area's margins
// shrink alike, so the HUD moves towards the view's edges (docs\argon\differences.md)
void GameHud::render()
{
    const int players = sources.multiplayer.players();
    if (player >= players || sources.weapons.craft(slot()).knockedOut())
    {
        return;   // no view, or the craft is out (0x49440: the HUD mode 2)
    }
    ImDrawList *drawList = ImGui::GetBackgroundDrawList();
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    const float pixelsPerUnit = ds.y / 480.f, vw = ds.x / pixelsPerUnit;
    const argon::split::ScreenRect view = argon::split::viewOf(players, player);
    const float sx = vw / 640.f;   // the views stretch over the window's width, the HUD keeps the game's height
    const float size = hudScale();
    const argon::split::ScreenRect sizedView = inHudUnits({view.x * sx, view.y, view.w * sx, view.h}, size);
    layout(safeArea(view, vw, size), sizedView);
    if (split())
    {
        layoutSplit();
    }
    update();
    showSinglePlayerElements(!split());
    const HudCanvas c(sources.painter, drawList, pixelsPerUnit * size);
    draw(c);
    const MarkerFrame frame = markerFrame(sizedView);
    drawNemesisMarker(c, frame);
    drawCrosshairs(c, frame);
    drawReconnectMessage(c, sizedView.y);
}

// the game's margins (hudFrame) in the HUD's units, measured in the view stretched over the window (widescreen) or,
// with the widescreen flag off, in the view's part of the centred 640 frame: the 4:3 layout (docs\argon\differences.md)
GameHud::SafeArea GameHud::safeArea(const argon::split::ScreenRect &view, float screenWidth, float size) const
{
    const bool wide = sources.viewerSettings.widescreen();
    const float sx = screenWidth / 640.f, originX = (screenWidth - 640.f) * 0.5f;
    const argon::split::ScreenRect viewArea = wide ? argon::split::ScreenRect{view.x * sx, view.y, view.w * sx, view.h}
                                               : argon::split::ScreenRect{view.x + originX, view.y, view.w, view.h};
    argon::split::ScreenRect frame = wide ? argon::split::hudFrame(viewArea, screenWidth) : argon::split::hudFrame(view);
    frame.x += wide ? 0.f : originX;
    const argon::split::ScreenRect sized = inHudUnits(viewArea, size);
    return {sized.x + (frame.x - viewArea.x), sized.x + sized.w - (viewArea.x + viewArea.w - frame.x - frame.w), sized.y + (frame.y - viewArea.y),
            sized.y + sized.h - (viewArea.y + viewArea.h - frame.y - frame.h)};
}

float GameHud::hudScale() const
{
    return sources.viewerSettings.hudScale();
}

// a rectangle in the game's units in the HUD's units, which are `size` of them
argon::split::ScreenRect GameHud::inHudUnits(const argon::split::ScreenRect &rect, float size)
{
    return {rect.x / size, rect.y / size, rect.w / size, rect.h / size};
}

// the player's view: its camera (App::raceViews) over its part of the screen; one player: the frame's own camera
GameHud::MarkerFrame GameHud::markerFrame(const argon::split::ScreenRect &view) const
{
    MarkerFrame frame;
    frame.viewProj = sources.hudCamera.viewProj;
    frame.eye = sources.hudCamera.eye;
    if (split())
    {
        const HudCamera camera = host.playerHudCamera(player);
        frame.viewProj = camera.viewProj;
        frame.eye = camera.eye;
    }
    frame.x = view.x;
    frame.y = view.y;
    frame.w = view.w;
    frame.h = view.h;
    return frame;
}

// the time block, the speed read-out and the minimap: a split screen has none (0x45270 hides them)
void GameHud::showSinglePlayerElements(bool on)
{
    for (HudText *label : {&timeLabel, &prevLabel, &bestLabel, &timeValue, &prevValue, &bestValue, &speedValue, &speedUnit})
    {
        label->show(on);
    }
    minimap.show(on);
}

bool GameHud::split() const
{
    return sources.multiplayer.players() > 1;
}

const argon::GameCraft &GameHud::craft() const
{
    return sources.race.racer(player);
}

int GameHud::slot() const
{
    return sources.race.slotOf(player);
}

// 0x45270 / 0x4a100 with more than one player: the power-up panel half size 32 right, the shield bar at the bottom
// (B - 32), the turbo read-out in a 72 x 20 box (drawTurboBox); showSinglePlayerElements hides the rest
void GameHud::layoutSplit()
{
    const float centre = (area.left + area.right) * 0.5f, barY = area.bottom - 32.f;
    const argon::hud::HudLanguageLayout language = languageLayout();
    powerUps.place(centre - 64.f + 32.f, area.top);
    attackWarning.place(centre - 64.f + 32.f + 64.f, area.top + 45.f);   // 0x4d80d: the panel + (64, 45)
    powerUps.setScale(0.5f);
    shieldLabel.place(area.left, barY - 19.f);
    shieldValue.place(area.left + 5.f, barY + language.shieldValueDrop);
    turboText.place(area.right - 68.f, barY + language.splitTurboTextDrop);
    turboLabel.place(area.right - 4.f, barY - 19.f);
}

argon::hud::HudLanguageLayout GameHud::languageLayout() const
{
    return argon::hud::hudLanguageLayout(sources.viewerSettings.language());
}

// 0x45270, single player; the shield and turbo texts are drawn by render 0x4a100 itself, from the bar's / box's y
void GameHud::layout(const SafeArea &safeArea, const argon::split::ScreenRect &view)
{
    area = safeArea;
    const float left = safeArea.left, R = safeArea.right, T = safeArea.top, B = safeArea.bottom, centre = (left + R) * 0.5f;
    const argon::hud::HudLanguageLayout language = languageLayout();
    const float timeRow = T + 52.f, step = language.timeRowStep, shieldLine = T + language.shieldBarTop;
    minimap.place(left + 4.f, B - 232.f);
    powerUps.place(centre - 64.f, T);
    attackWarning.place(centre - 64.f + 64.f, T + 90.f);   // 0x4d7f0: the panel + (64, 90)
    lapLabel.place(R - 4.f, T);
    lapCount.place(R - 4.f, T + 10.f);
    timeLabel.place(R - 114.f, timeRow);
    prevLabel.place(R - 114.f, timeRow + step);
    bestLabel.place(R - 114.f, timeRow + 2.f * step);
    timeValue.place(R - 4.f, timeRow);
    prevValue.place(R - 4.f, timeRow + step);
    bestValue.place(R - 4.f, timeRow + 2.f * step);
    scoreLabel.place(left, T);   // 0x456ba
    scoreValue.place(left, T + language.scoreValueTop);
    shieldLabel.place(left, shieldLine - 19.f);
    shieldValue.place(left + 5.f, shieldLine + language.shieldValueDrop);
    speedoOverlay.place(R - 167.f, B - 166.f);
    turboText.place(R - 146.f, B - 57.f + language.turboTextDrop);
    speedValue.place(R - language.speedValueLeft, B - language.speedValueAbove);
    speedUnit.place(R - 3.f, B - language.speedUnitAbove);
    staticOverlay.place(view.x, view.y);   // the view (+0x19394..; the game's L - 52, T - 36)
    staticOverlay.setSize(view.w, view.h);
    const float middle = (T + B) * 0.5f;   // (B - T) / 2 + T
    resetMessage.place(centre, middle - 40.f);
    recordMessage.place(centre, middle - 10.f);   // 0x46239
    wrongWayMessage.place(centre, middle + 40.f);
    powerUps.setScale(1.f);
    pointsPopup.place(centre, T + 120.f);
}

void GameHud::update()
{
    updateMinimap();
    updateLapBlock();
    updateShieldAndScore();
    updateSpeedometer();
    updateMessages();
    updatePointsPopup();
}

// "Lap", "%d[F:0] / %d" (Time Attack "%d[F:0] / --": no lap limit, 0x496f0), time / prev / best
void GameHud::updateLapBlock()
{
    const argon::GameCraft &sim = craft();
    const bool limited = sources.modes.current().hasLapLimit();
    const int lap = limited ? std::min(sim.laps() + 1, sources.hudLaps) : sim.laps() + 1;
    char text[64];
    if (limited)
    {
        std::snprintf(text, sizeof text, "%d[F:0] / %d", lap, sources.hudLaps);
    }
    else
    {
        std::snprintf(text, sizeof text, "%d[F:0] / --", lap);
    }
    lapLabel.setText(sources.strings.race(605, "Lap"));
    lapCount.setText(argon::widenLatin1(text));
    timeLabel.setText(sources.strings.race(613, "time:"));
    prevLabel.setText(sources.strings.race(614, "prev:"));
    bestLabel.setText(sources.strings.race(615, "best:"));
    timeValue.setText(lapTimeText(sim.lapTime(), LapShown::Current));
    prevValue.setText(sim.laps() > 0 ? lapTimeText(sim.lastLap(), LapShown::Record) : lapTimeText(-1, LapShown::Record));
    bestValue.setText(sim.laps() > 0 ? lapTimeText(sim.bestLap(), LapShown::Record) : lapTimeText(-1, LapShown::Record));
}

// the shield (huddata +0x44) and the score, the power-up panel: none in Time Attack (0x4a100 skips the shield, 0x49440
// hides +0x3950 and the score's +0x66a4 / +0x68b0); Weapons Off (0x52abe3 clear) hides only the power-up panel
// (gamehud_setup 0x47816, the HUD modes 0x4964e)
void GameHud::updateShieldAndScore()
{
    const bool weapons = sources.modes.current().hasWeapons();
    powerUps.show(weapons && sources.weaponsOn);
    scoreLabel.show(weapons && !split());   // single player only (0x45270)
    scoreValue.show(weapons && !split());
    shieldLabel.show(weapons);
    shieldValue.show(weapons);
    shieldPercent = sources.weapons.shieldPercent(slot());
    shieldFlash = sources.weapons.craft(slot()).shieldFlashLevel();
    const Weapons::Loadout held = sources.weapons.loadoutOf(slot());
    HudPowerUpPanel::Weapons panelWeapons;
    panelWeapons.held = {held.red, held.blue, held.yellow};
    panelWeapons.caps = {sources.pickupCaps.red, sources.pickupCaps.blue, sources.pickupCaps.yellow};
    if (player != 0)
    {   // players 2..4: their craft's caps from the race setup's table (0x3f3d0)
        const int cap = sources.weapons.aiWeaponSetupOf(slot()).levelCap;
        panelWeapons.caps = {cap, cap, cap};
    }
    panelWeapons.specialReady = player == 0 ? sources.hudSpecial : held.special > 0;
    powerUps.setWeapons(panelWeapons);
    powerUps.setClock(sources.simClock);
    scoreLabel.setText(sources.strings.race(606, "Score"));
    scoreValue.setText(integer(sources.scoring.points.shown()));
    shieldLabel.setText(sources.strings.race(601, "Shield"));
    shieldValue.setText(integer(int(sources.weapons.craft(slot()).energy() + 0.5f)));   // huddata +0x48, the value
}

// km/h = the last 4 forward speeds x 0.9 (huddata +0x14); the second needle follows +0x18 = (class + 1) x throttle / 6
// x 0.5 with a fresh jitter each frame (two rand() draws, 0x4ad84 / 0x4adb3), x 0.2 per frame
void GameHud::updateSpeedometer()
{
    speedKmh = sources.speedReadout.kmh();
    turboSeconds = craft().turboSeconds();   // huddata +0xc (craft +0x8e4)
    // "%s: %.1fs" in one box; split screen "Turbo" over a "%.1fs" box (0x4a986)
    turboText.setText(split() ? number("%.1fs", turboSeconds) : sources.strings.race(600, "Turbo") + number(": %.1fs", turboSeconds));
    turboLabel.setText(sources.strings.race(600, "Turbo"));
    turboLabel.show(split());
    const float throttleGauge = (sources.speedClass + 1) * craft().throttle() * (1.f / 6.f) * 0.5f;
    const argon::hud::NeedleJitterDraws draws{int(noise() & 0xff), int(noise() & 0xff)};
    const float want = argon::hud::secondNeedleTarget(throttleGauge, draws);
    secondNeedle += (want - secondNeedle) * (1.f - std::pow(0.8f, ImGui::GetIO().DeltaTime * 60.f));
    speedValue.setText(integer(int(std::lround(sources.speedMph ? speedKmh / 1.609344f : speedKmh))));   // Speed Units
    speedUnit.setText(sources.speedMph ? sources.strings.race(624, "mph") : sources.strings.race(623, "km/h"));
}

void GameHud::updateMinimap()
{
    static const char *const kPlace[6] = {"1st", "2nd", "3rd", "4th", "5th", "6th"};
    const int total = sources.race.racerCount();
    const std::vector<int> place = sources.race.places();
    std::vector<HudRacer> racers(static_cast<size_t>(total));
    for (int i = 0; i < total; ++i)
    {
        HudRacer &racer = racers[size_t(i)];
        racer.pose = i == 0 ? sources.poses.driverMatrix() : sources.race.droneMatrix(sources.race.grid()[size_t(i - 1)]);
        racer.pilot = sources.race.racer(i).pilot();
        racer.place = place[size_t(i)];
        const int slot = sources.race.slotOf(i);
        racer.energy = sources.weapons.shieldPercent(slot);
        racer.hitFlash = sources.weapons.craft(slot).hitFlashColour();
        racer.knockedOut = sources.race.knockedOut(i);
    }
    std::array<std::u16string, 6> words;
    for (int j = 0; j < 6; ++j)
    {
        words[size_t(j)] = sources.strings.race(0x25f + j, kPlace[j]);
    }
    const int trackId = host.courseId(sources.track.name);
    minimap.setMap("minimap/" + argon::hud::minimapName(trackId), sources.showMap);
    minimap.setWorldPerTexel(argon::hud::minimapWorldPerTexel(trackId));
    minimap.setMirrored(sources.raceEggs.on(Egg::MirrorTracks));
    minimap.setRace(std::move(racers), sources.modes.current().hasOpponents(), words);
}

// RESET (616) while the respawn's timer runs (huddata +0x60 > 0, 0x4c060) over the static overlay; WRONG WAY (617,
// steady) while huddata +0x43 is set and the race runs (0x49f54); "New lap record" (800) only on Time Attack's HUD
void GameHud::updateMessages()
{
    resetMessage.setText(sources.strings.race(616, "RESET"));
    recordMessage.setText(sources.strings.race(800, "New lap record"));
    wrongWayMessage.setText(sources.strings.race(617, "WRONG WAY"));
    const bool firstPlayer = player == 0;   // the warnings follow player 1's craft
    resetMessage.show(firstPlayer && sources.raceWarnings.resetTime() > 0.f);
    recordMessage.show(lapRecordShown());
    if (lapRecordShown())
    {
        host.playLapRecordSound();   // 0x4c90c
    }
    wrongWayMessage.show(firstPlayer && sources.raceWarnings.wrongWay() && sources.raceEnd == race_end::kRacing);
    updateResetOverlay();
    updateAttackWarning();
}

// 0x4d97b: race string 0x51f while huddata +0x108 has bit 0x20 (the craft in danger, 0x6bf3e), the race runs and its
// start is over, on the power-up panel (none without weapons). The game also hides it while a pad is out (0x52abfc)
// and on Restart / Track Select (0x52abe8 / 0x52abe9), which leave the race in the viewer
void GameHud::updateAttackWarning()
{
    attackWarning.setText(sources.strings.race(0x51f, "ATTACK WARNING"));
    attackWarning.show(sources.modes.current().hasWeapons() && sources.weaponsOn && sources.weapons.warnings().shown(slot()) &&
                       sources.raceEnd == race_end::kRacing && !host.raceHeld());
}

// 0x4c060: alpha = clamp(+0x60 x 255), a random uv offset (rand & 0xfff) / 4095 each frame
void GameHud::updateResetOverlay()
{
    const float resetTime = player == 0 ? sources.raceWarnings.resetTime() : 0.f;
    staticOverlay.show(resetTime > 0.f);
    const uint32_t alpha = uint32_t(std::clamp(std::lround(resetTime * 255.f), 0L, 255L));
    const float u = float(noise() & 0xfff) / 4095.f, v = float(noise() & 0xfff) / 4095.f;
    staticOverlay.setFrame(alpha, u, v);
}

// CGameHudTimeAttack 0x4c850: the lap that set the race's best (huddata +0x42) shows it for the next lap's first 2.5 s
// (lap time x 4 < 10), on while frac(lap time x 4) > 0.4 (a 4 Hz blink); not after the race (0x52af9f, post race)
bool GameHud::lapRecordShown() const
{
    if (player != 0 || !sources.modes.current().showsLapRecord() || sources.lapRecords.recordOnLap() != sources.playerCraft.laps() ||
        sources.raceEnd != race_end::kRacing)
    {
        return false;
    }
    const float quarters = sources.playerCraft.lapTime() * 4.f;
    return quarters < 10.f && quarters - std::floor(quarters) > 0.4f;
}

// 0x4cc30: the first queued popup, its age a = time / 0.8; labels from race strings (table 0x1b9748)
void GameHud::updatePointsPopup()
{
    pointsPopup.show(player == 0 && sources.scoring.points.hasPopup());
    if (!sources.scoring.points.hasPopup())
    {
        return;
    }
    static const int kLabel[16] = {1230, 1231, 1232, 1233, 1234, 1235, 1236, 1237, 1238, 1239, 1240, 1241, 1317, 1318, 1332, 1242};
    static const char *const kFallback[16] = {"Attack",
                                              "AWESOME ATTACK",
                                              "Killed",
                                              "AWESOME KILL",
                                              "Great Lap",
                                              "AWESOME LAP",
                                              "Airtime",
                                              "AWESOME AIRTIME",
                                              "Points Bonus",
                                              "Power-up Bonus",
                                              "MEGA-UP BONUS",
                                              "Nemesis Kill",
                                              "Turbo Up",
                                              "Shield Low",
                                              "Opponent Shielded",
                                              "Hit by"};
    const PointsPopup &popup = sources.scoring.points.popup();
    const int kind = std::clamp(popup.kind, 0, 15);
    const float shown = std::clamp(sources.scoring.points.popupShownFor(sources.simClock) / RacePoints::kPopupSeconds, 0.f, 1.f);
    pointsPopup.setEntry(popup.points ? integer(popup.points) : std::u16string(), popupLabel(kind, popup.pilot, sources.strings.race(kLabel[kind], kFallback[kind])), shown);
}

// 0x4cd62: "%s %s": "Killed" (kind 2) after its pilot's name, "Hit by" (kind 15) before it (infotext 400 + pilot)
std::u16string GameHud::popupLabel(int kind, int pilot, const std::u16string &label) const
{
    const int killed = 2, hitBy = 15;
    if (kind == killed)
    {
        return host.racerName(pilot) + u" " + label;
    }
    if (kind == hitBy)
    {
        return label + u" " + host.racerName(pilot);
    }
    return label;
}

// render 0x4a100: the elements in the game's order, with the shield bar, the turbo fan and box and the needles
void GameHud::draw(const HudCanvas &canvas) const
{
    minimap.drawIfShown(canvas);
    powerUps.drawIfShown(canvas);
    attackWarning.drawIfShown(canvas);
    for (const HudText *label : {&lapLabel, &lapCount, &timeLabel, &prevLabel, &bestLabel, &timeValue, &prevValue, &bestValue})
    {
        label->drawIfShown(canvas);
    }
    shieldLabel.drawIfShown(canvas);
    scoreLabel.drawIfShown(canvas);
    scoreValue.drawIfShown(canvas);
    if (sources.modes.current().hasWeapons())
    {
        drawShieldBar(canvas);
    }
    shieldValue.drawIfShown(canvas);
    if (!split())
    {   // 0x4a100 draws the overlay with the fan, single player only (the layout keeps it hidden)
        speedoOverlay.drawIfShown(canvas);
        drawTurboFan(canvas);
    }
    drawTurboBox(canvas);
    turboLabel.drawIfShown(canvas);
    turboText.drawIfShown(canvas);
    if (!split())
    {
        drawNeedles(canvas);
    }
    speedValue.drawIfShown(canvas);
    speedUnit.drawIfShown(canvas);
    staticOverlay.drawIfShown(canvas);
    resetMessage.drawIfShown(canvas);
    recordMessage.drawIfShown(canvas);
    wrongWayMessage.drawIfShown(canvas);
    pointsPopup.drawIfShown(canvas);
}

// render 0x4a99f: a 162 x 20 box at (R - 162, B - 57) for "Turbo: %.1fs"; split screen "%.1fs" in a 72 x 20 box at
// (R - 76, B - 32)
void GameHud::drawTurboBox(const HudCanvas &canvas) const
{
    if (split())
    {
        canvas.panel({area.right - 76.f, area.bottom - 32.f, 72.f, 20.f}, {false, false, 0xffffff00u, false, 0, 0});
        return;
    }
    canvas.panel({area.right - 162.f, area.bottom - 57.f, 162.f, 20.f}, {false, false, 0xffffff00u, false, 0, 0});
}

// hud_draw_bar 0xc24d0: back panel (highlight + glow while a shield blocks hits, 0x6aba0; fill EAX = 0), then the
// fill panel of shield% x len, all six corners EBX (a solid fill), never highlighted; each panel also gets the plasma
// overlay and its outline. The highlight is the front end's selected-row style (hud_draw_panel 0xc1b40: overlay alpha
// 0xb0, yellow outline, glow ring at half the line colour). The fill blinks red under 0.33 % and white while the shield flash is up
// (hud\shield_bar_rules.hpp, on the 60 Hz vertical blanks). Length
// (0x4a100, single player): 90 + (shield size - 100) x 100 / 180, the size being App::shieldSize's. The bar sits on
// the shield line (T + 61, split screen B - 32), Japanese 3 lower and the single-player line at T + 79
void GameHud::drawShieldBar(const HudCanvas &canvas) const
{
    const float extra = sources.weapons.craft(slot()).shieldMax() - 100.f;
    const float len = split() ? 70.f + extra * (70.f / 180.f) : 90.f + extra * (100.f / 180.f);
    const argon::hud::HudLanguageLayout language = languageLayout();
    const float line = split() ? area.bottom - 32.f : area.top + language.shieldBarTop;
    const float x = area.left, y = line + language.shieldBarDrop;
    const bool up = sources.weapons.shieldGuard(slot()).blocked;   // 0x6aba0: a live class 2 weapon with shield-damage resistance
    const auto vblanks = static_cast<unsigned>(ImGui::GetTime() * kVblanksPerSecond);
    const uint32_t fill = argon::hud::shieldFillColour(shieldPercent, shieldFlash, vblanks);
    canvas.panel({x, y, len, 20.f}, {up, up, 0xffffff00u, true, 0x00000000u, 0x00000000u});
    canvas.panel({x, y, shieldPercent * 0.01f * len, 20.f}, {false, false, 0xffffff00u, true, fill, fill});   // 0xc2556: EAX = EBX
}

// speedo_bars, centre uv (0.5, 1) + 6 points at 225 - k f deg; f from the turbo seconds. gamehud_render 0x4a2bc: red
// / blue = 0x40, 0xff while a turbo pushes (every other 4 frames of the 60 Hz counter 0x439df0), and 0x40 + 191 x
// (1 - t) for a second after a grant (t = time since, +0x114c)
void GameHud::drawTurboFan(const HudCanvas &canvas) const
{
    const float T5 = turboSeconds;
    const float fanTurn = T5 < 5    ? T5 * 0.2f * 70.f
                    : T5 < 10 ? 70.f + (T5 - 5) * 0.2f * 35.f
                    : T5 < 15 ? 105.f + (T5 - 10) * 0.2f * 17.5f
                    : T5 < 20 ? 122.5f + (T5 - 15) * 0.2f * 12.5f
                    : T5 < 25 ? 135.f + (T5 - 20) * 0.2f * 11.1f
                              : 144.1f;
    const Found<const HudTexture> bt = canvas.texture("speedo_bars");
    if (!bt || !(fanTurn > 0.f))
    {
        return;
    }
    int cc = 0x40;
    if (sources.playerCraft.turboOn() && (int(sources.simClock * 60.f) & 7) < 4)
    {
        cc = 0xff;
    }
    const float tg = std::clamp(sources.simClock - sources.turboGrantT, 0.f, 1.f);
    cc = std::min(std::max(cc, 0x40 + int(191.f * (1.f - tg))), 0xff);
    const ImU32 colour = imColour(0xe000ff00u | (uint32_t(cc) << 16) | uint32_t(cc));
    const ImVec2 ctr(area.right - 71.f, area.bottom - 70.f);
    ImDrawList *drawList = canvas.drawList();
    drawList->PushTextureID(bt->image);
    drawList->PrimReserve(15, 15);
    for (int i = 0; i < 5; ++i)
    {
        const FanPoint from = fanPoint(canvas, ctr, 225.f - float(i) * 0.2f * fanTurn), to = fanPoint(canvas, ctr, 225.f - float(i + 1) * 0.2f * fanTurn);
        drawList->PrimVtx(canvas.at(ctr.x, ctr.y), ImVec2(0.5f, 1.f), colour);
        drawList->PrimVtx(from.at, from.uv, colour);
        drawList->PrimVtx(to.at, to.uv, colour);
    }
    drawList->PopTextureID();
}

// pivot (R-69.5, B-70), angle = 270 - a degrees, quad from the pivot along (sin, cos); colour white / 0xffff0000
void GameHud::drawNeedles(const HudCanvas &canvas) const
{
    const Found<const HudTexture> texture = canvas.texture("speedo_needle");
    if (!texture)
    {
        return;
    }
    const ImVec2 pivot(area.right - 69.5f, area.bottom - 70.f);
    drawNeedle(canvas, *texture, Needle{pivot, std::clamp(speedKmh * 0.12008f, 0.f, 144.1f), 96.f, 24.f, 0xffffffffu});
    drawNeedle(canvas, *texture, Needle{pivot, secondNeedle, 64.f, 16.f, 0xffff0000u});
}

// the HUD's text +0x6ea0 (0x45ef0: race 0x548 + the player's port, "Please reconnect controller into port n, and press
// START."; font 0, flags 0x102, white): "insert on" / "insert off" from PAUSEMENU (0x4999d / 0x499c3, ui\pause_rules.hpp)
// show it; the layout (0x45403) puts it centred across the view's safe width at (B - T) / 2 below the view's top (not
// below T as the other messages), wrapping to that width (+0x44, +0x70a8)
void GameHud::drawReconnectMessage(const HudCanvas &canvas, float viewTop) const
{
    if (!sources.padWatch.messageShown(player))
    {
        return;
    }
    const int pad = sources.pads.padOfPlayer(player);
    const std::u16string text = sources.strings.race(0x548 + pad, "Please reconnect controller, and press START.");
    const float centre = (area.left + area.right) * 0.5f, lineHeight = sources.menus.fontLine(0);
    float y = (area.bottom - area.top) * 0.5f + viewTop;
    for (const std::u16string &line : sources.menus.wrapText(text, 0, 1.f, area.right - area.left))
    {
        canvas.text({centre, y}, line, {0, kWhite, 2});
        y += lineHeight;
    }
}
