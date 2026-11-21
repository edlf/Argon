// The viewer's own ImGui windows (not the game's UI): a small main window (the main menu, the track, which tool
// windows show) and one optional window each for layers, markers, render toggles, ships, audio, AI drive, camera.
#pragma once

#include <array>
#include <string>
#include <vector>

#include "logic/upgrade_rules.hpp"

class DriveCamera;
class FreeCamera;
class Race;
class StartGrid;
struct SceneToggles;
struct Track;
namespace argon
{
class Audio;
class CraftStats;
class GameCraft;
class Route;
class RouteReport;
namespace gfx
{
class Device;
class Presenter;
}  // namespace gfx
namespace weapons
{
struct PowerLevels;
}
}  // namespace argon

constexpr size_t kEditorToolWindows = 8;
using EditorWindowsOpen = std::array<bool, kEditorToolWindows>;   // kept by App across frames

// what the panel shows and changes
struct EditorPanelSources
{
    SceneToggles &toggles;
    EditorWindowsOpen &windows;
    const std::vector<std::string> &trackList;
    const int &trackIndex;
    int &pendingTrack;                      // the track picked, loaded by App
    const Track &track, &sky;               // the groups' counts, the markers, the cameras, the warnings
    const argon::gfx::Presenter &presenter;   // the samples
    const argon::gfx::Device &device;         // wireframe support
    StartGrid &startGrid;                   // the pilots
    argon::Audio &audio;
    const bool &audioEnabled;
    const std::string &enginePilot;
    const argon::RouteReport &routeReport;   // the summary line and the issues list
    const argon::Route &route;
    bool &driving, &chaseCam, &playerDrive, &showHud, &hudSpecial, &pickupLogic, &upgradeCapsAuto, &linkEngine;
    const bool &simStarted;
    argon::GameCraft &player;               // racer 0 (App::sim)
    DriveCamera &driveView;
    int (&powerLevels)[3];                  // the player's red / blue / yellow levels (App::hudWeapon)
    int &weaponSpecial, &levelCap, &speedClass;
    argon::weapons::PowerLevels &pickupCaps;
    float &driveScale, &aiPower;
    Race &race;                             // the opponents, contacts, catch-up and routes switches
    const argon::CraftStats &craftStats;
    FreeCamera &freeCamera;
};

// what the panel asks App for
class EditorHost
{
  public:
    virtual void openMenu() = 0;
    virtual void choosePlayerPilot(int pilot) = 0;
    virtual void resetDriver() = 0;
    virtual void applyUpgradeCaps() = 0;
    virtual int upgradeLevel(argon::upgrades::Upgrade upgrade) const = 0;
    virtual void resetPickups() = 0;
    virtual void reshuffleGrid() = 0;
    virtual std::string viabilityText() const = 0;
    virtual void cameraToStart() = 0;
    virtual void cameraToTop() = 0;
    virtual void chooseDriveView(int chosenView) = 0;

  protected:
    ~EditorHost() = default;
};

class EditorPanel
{
  public:
    EditorPanel(EditorHost &host, const EditorPanelSources &sources) : host_(host), sources_(sources)
    {
    }
    void draw();

  private:
    struct ToolWindow
    {
        const char *title;
        void (EditorPanel::*body)();
    };
    static const ToolWindow kToolWindows[kEditorToolWindows];

    void drawMainWindow();
    void drawWindowToggles();
    void drawToolWindow(size_t index);
    void drawTrackPicker();
    void drawLayers();
    void drawMarkers();
    void drawRender();
    void drawShipsAndPickups();
    void drawEffectToggles();
    void drawTrailLook();
    void drawPlayerShipPicker();
    void drawAudio();
    void drawAudioVolumes();
    void drawEngineAndOneShots();
    void drawAiDrive();
    void drawDriveButtons();
    void drawTurbo();
    void drawViewPicker();
    void drawHudPreview();
    void drawPickupRules();
    void drawRaceSetup();
    void drawOpponents();
    void drawPlaces();
    void drawDriveStats();
    void drawViability();
    void drawIssueList();
    void drawCamera();
    void drawWarnings();

    EditorHost &host_;
    EditorPanelSources sources_;
};
