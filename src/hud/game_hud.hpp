// Argon: the in-race HUD, the game's CGameHud (gamehud.cpp; docs\physics\hud.md "HUD"): ctor 0x448b0 builds its
// elements, gamehud_layout 0x45270 places them in the single-player safe area, gamehud_update 0x496f0 sets their
// strings and visibility from the player's craft (huddata, craft +0xa30), gamehud_render 0x4a100 draws them with the
// speedometer and the shield bar it draws itself; then the markers over the race (0x4d9d0, hud\markers.cpp).
#pragma once
#include "hud/speed_readout.hpp"
#include <random>

#include <glm/glm.hpp>

#include "hud/game_hud_host.hpp"
#include "hud/hud_elements.hpp"
#include "hud/hud_language_layout.hpp"
#include "hud/hud_minimap.hpp"
#include "logic/split_screen.hpp"

namespace argon
{
class GameCraft;
class RaceWarnings;
namespace weapons
{
struct PowerLevels;
}
}  // namespace argon
namespace menu
{
class PadWatch;
}

struct ImDrawList;
class GameModes;
class GameStrings;
class HudCanvas;
class HudPainter;
class MenuKit;
class MultiplayerSession;
class Race;
class RaceEggs;
class RaceLapRecords;
class RaceStart;
class ViewerSettings;
class Weapons;
class CraftPoses;
class PlayerPads;
struct RaceScoring;
struct Track;

// what the HUD draws with and reads
struct GameHudSources
{
    HudPainter &painter;
    const MenuKit &menus;                    // the font's lines, wrapped text
    const GameStrings &strings;
    const ViewerSettings &viewerSettings;    // widescreen, the HUD size, the language
    const MultiplayerSession &multiplayer;   // the players
    const Race &race;
    const argon::GameCraft &playerCraft;     // racer 0 (App::sim)
    const Weapons &weapons;                  // shields, power-ups, locks, warnings
    const GameModes &modes;
    const RaceScoring &scoring;              // the points and their popup
    const argon::RaceWarnings &raceWarnings;
    const RaceLapRecords &lapRecords;
    const RaceStart &raceStart;              // the grid intro's clock
    const RaceEggs &raceEggs;
    const menu::PadWatch &padWatch;          // the reconnect messages
    const HudCamera &hudCamera;              // the frame's first view
    const Track &track;
    const argon::weapons::PowerLevels &pickupCaps;
    const float &simClock, &turboGrantT;
    const SpeedReadout &speedReadout;        // the last four forward speeds
    const int &hudLaps, &speedClass, &raceEnd;
    const bool &hudSpecial, &weaponsOn, &speedMph, &showMap;
    const CraftPoses &poses;                 // where the crafts are drawn
    const PlayerPads &pads;                  // the player's pad (its rumble row)
};

class GameHud
{
  public:
    GameHud(GameHudHost &host, const GameHudSources &sources, int player);   // player: 0..3, the split screen's view it draws in
    GameHud(const GameHud &) = delete;
    GameHud &operator=(const GameHud &) = delete;

    void render();                    // one frame: layout, update, draw (ImGui's background list); none without a view

  private:
    struct SafeArea
    {
        float left, right, top, bottom;
    };   // 52 / 36 in from the 640 x 480 edges (0x45270)

    float hudScale() const;   // the viewer's HUD Size: 0.25 .. 1
    static argon::split::ScreenRect inHudUnits(const argon::split::ScreenRect &rect, float size);
    argon::hud::HudLanguageLayout languageLayout() const;   // the viewer's language picks the game's 0x5293b0 offsets
    SafeArea safeArea(const argon::split::ScreenRect &view, float screenWidth, float size) const;
    void layout(const SafeArea &a, const argon::split::ScreenRect &view);
    void layoutSplit();
    void showSinglePlayerElements(bool on);
    const argon::GameCraft &craft() const;   // the player's craft
    int slot() const;                     // and its grid slot
    bool split() const;
    void update();
    void updateLapBlock();
    void updateShieldAndScore();
    void updateSpeedometer();
    void updateMinimap();
    void updateMessages();
    void updateAttackWarning();
    void updateResetOverlay();
    bool lapRecordShown() const;
    void updatePointsPopup();
    std::u16string popupLabel(int kind, int pilot, const std::u16string &label) const;
    void draw(const HudCanvas &canvas) const;
    void drawShieldBar(const HudCanvas &c) const;     // hud_draw_bar 0xc24d0
    void drawTurboFan(const HudCanvas &c) const;      // render 0x4a2bc
    void drawNeedles(const HudCanvas &c) const;       // hud_draw_rotated_quad 0xcbe50
    // a view's camera and its part of the HUD's screen (the markers project into it)
    struct MarkerFrame
    {
        glm::mat4 viewProj{1.f};
        glm::vec3 eye{0.f};
        float x = 0, y = 0, w = 640, h = 480;
        glm::vec2 toHud(const glm::vec4 &clip) const
        {
            return {x + (clip.x / clip.w * 0.5f + 0.5f) * w, y + (clip.y / clip.w * 0.5f + 0.5f) * h};
        }
    };
    MarkerFrame markerFrame(const argon::split::ScreenRect &view) const;
    void drawNemesisMarker(const HudCanvas &canvas, const MarkerFrame &frame) const;
    void drawIntroLabelsOver(const HudCanvas &canvas, const MarkerFrame &frame, int slot, int nemesis) const;
    void drawIntroLabels(const HudCanvas &canvas, float x, float y, int pilot, bool nemesis) const;
    int pilotInSlot(int slot) const;          // -1: no craft there
    int nemesisSlot(int nemesis) const;       // -1: the nemesis doesn't race
    void drawCrosshairs(const HudCanvas &canvas, const MarkerFrame &frame) const;
    void drawTurboBox(const HudCanvas &canvas) const;
    void drawReconnectMessage(const HudCanvas &canvas, float viewTop) const;

    GameHudHost &host;
    GameHudSources sources;
    int player;
    SafeArea area{};
    HudMinimap minimap;
    HudPowerUpPanel powerUps;
    HudText lapLabel, lapCount, timeLabel, prevLabel, bestLabel, timeValue, prevValue, bestValue;
    HudText scoreLabel, scoreValue, shieldLabel, shieldValue;
    HudSprite speedoOverlay;
    HudText turboText, turboLabel, speedValue, speedUnit;   // turboLabel: split screen only
    HudStaticOverlay staticOverlay;
    HudText resetMessage, recordMessage, wrongWayMessage;
    HudText attackWarning;   // the power-up panel's +0x1860 (0x4d630)
    HudPointsPopup pointsPopup;

    float speedKmh = 0, turboSeconds = 0, shieldPercent = 100.f;
    float shieldFlash = 0;            // huddata +0x70 (craft weapon +0x94): the fill blinks white while above 0
    float secondNeedle = 0;           // smoothed (degrees), huddata +0x18
    std::minstd_rand noise;           // the game's rand(): the static overlay's uv offset (0x4c060), the needle's jitter
};
