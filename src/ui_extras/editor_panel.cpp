#include "ui_extras/editor_panel.hpp"

#include <algorithm>
#include <cstdio>

#include <imgui.h>

#include "app/weapons.hpp"
#include "audio/audio.hpp"
#include "craft/craftsim.hpp"
#include "craft/drive.hpp"
#include "gfx/device.hpp"
#include "gfx/presenter.hpp"
#include "logic/drive_camera.hpp"
#include "logic/free_camera.hpp"
#include "logic/race.hpp"
#include "logic/start_grid.hpp"
#include "render/gpu_model.hpp"
#include "render/scene_toggles.hpp"
#include "track/route_report.hpp"

namespace
{

const float kPanelWidth = 330.f, kGap = 10.f, kStagger = 28.f;

void tooltip(const char *text)
{
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", text);
    }
}

template <class... Args> void countedCheckbox(bool *value, const char *format, Args... args)
{
    char label[96];
    std::snprintf(label, sizeof label, format, args...);
    ImGui::Checkbox(label, value);
}

void volumeSlider(const char *id, float *volume)
{
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    ImGui::SliderFloat(id, volume, 0.f, 1.f, "%.2f");
}

bool isProblem(const argon::RouteIssue &issue)
{
    return issue.problem();
}

} // namespace

const EditorPanel::ToolWindow EditorPanel::kToolWindows[kEditorToolWindows] = {
    {"Layers", &EditorPanel::drawLayers},
    {"Markers", &EditorPanel::drawMarkers},
    {"Render", &EditorPanel::drawRender},
    {"Ships & pickups", &EditorPanel::drawShipsAndPickups},
    {"Audio", &EditorPanel::drawAudio},
    {"AI drive", &EditorPanel::drawAiDrive},
    {"Camera", &EditorPanel::drawCamera},
    {"Warnings", &EditorPanel::drawWarnings},
};

void EditorPanel::draw()
{
    drawMainWindow();
    for (size_t i = 0; i < kEditorToolWindows; ++i)
    {
        if (sources_.windows[i])
        {
            drawToolWindow(i);
        }
    }
}

void EditorPanel::drawMainWindow()
{
    ImGui::SetNextWindowPos(ImVec2(kGap, kGap), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(kPanelWidth, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Argon");
    if (ImGui::Button("Main menu"))
    {
        host_.openMenu();
    }
    drawTrackPicker();
    ImGui::SeparatorText("Windows");
    drawWindowToggles();
    ImGui::End();
}

// two columns of checkboxes, one per tool window
void EditorPanel::drawWindowToggles()
{
    for (size_t i = 0; i < kEditorToolWindows; ++i)
    {
        if (i % 2 == 1)
        {
            ImGui::SameLine(kPanelWidth * 0.5f);
        }
        ImGui::Checkbox(kToolWindows[i].title, &sources_.windows[i]);
    }
}

// a closable window right of the main one, each first placed a step lower than the one before it
void EditorPanel::drawToolWindow(size_t index)
{
    const float maxHeight = ImGui::GetIO().DisplaySize.y - 2.f * kGap;
    ImGui::SetNextWindowPos(ImVec2(kPanelWidth + 2.f * kGap, kGap + kStagger * float(index)), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(kPanelWidth, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(200.f, 60.f), ImVec2(FLT_MAX, maxHeight));
    if (ImGui::Begin(kToolWindows[index].title, &sources_.windows[index]))
    {
        (this->*kToolWindows[index].body)();
    }
    ImGui::End();
}

void EditorPanel::drawTrackPicker()
{
    const char *current = sources_.trackIndex >= 0 ? sources_.trackList[sources_.trackIndex].c_str() : "(none)";
    if (ImGui::BeginCombo("Track", current))
    {
        for (int i = 0; i < int(sources_.trackList.size()); ++i)
        {
            if (ImGui::Selectable(sources_.trackList[i].c_str(), i == sources_.trackIndex))
            {
                sources_.pendingTrack = i;
            }
        }
        ImGui::EndCombo();
    }
    if (sources_.trackIndex < 0)
    {
        return;
    }
    const Track &track = sources_.track;
    ImGui::Text("%s", track.data.props.name.c_str());
    ImGui::TextDisabled("%zu draws, %zu tris, %zu textures, loaded in %.2fs", track.draws.size(), track.triangles, track.textures.size(), track.loadSeconds);
}

void EditorPanel::drawLayers()
{
    for (int g = 0; g < G_COUNT; ++g)
    {
        const int count = g == G_SKY ? sources_.sky.groupCount[G_SKY] : sources_.track.groupCount[g];
        countedCheckbox(&sources_.toggles.groups[g], "%s (%d)", kGroupNames[g], count);
    }
    countedCheckbox(&sources_.toggles.water, "Water (%zu areas)", sources_.track.data.water.size());
}

void EditorPanel::drawMarkers()
{
    const argon::TrackData &data = sources_.track.data;
    countedCheckbox(&sources_.toggles.starts, "Start grid (%zu)", data.starts.size());
    countedCheckbox(&sources_.toggles.aiRoute, "AI racing line (%zu nodes)", data.ai.size());
    countedCheckbox(&sources_.toggles.pickups, "Pickups (%zu)", data.pickups.size());
    ImGui::Checkbox("Draw markers on top", &sources_.toggles.markersOnTop);
}

void EditorPanel::drawRender()
{
    const Track &track = sources_.track;
    ImGui::Checkbox("Textures", &sources_.toggles.textures);
    ImGui::SameLine();
    ImGui::Checkbox("Lighting", &sources_.toggles.lighting);
    ImGui::SameLine();
    ImGui::Checkbox("Fog", &sources_.toggles.fog);
    ImGui::Checkbox("Baked lighting", &sources_.toggles.bakedLighting);
    tooltip("The vertices' prelit colours (the lighting baked into the track); off draws them white");
    countedCheckbox(&sources_.toggles.gameMaterials, "Game materials (%d blended)", track.blendedCount);
    tooltip("Per-mesh blend mode, alpha test, depth write and culling decoded from\n"
            "each mesh's surface block (see docs\\models\\README.md). Off = everything opaque.");
    if (sources_.toggles.gameMaterials)
    {
        ImGui::TextDisabled("%d multi-layer, %d animated/scrolling, %d env-mapped", track.multiStage, track.animated, track.envMapped);
        ImGui::Checkbox("Back-face culling", &sources_.toggles.gameCull);
        ImGui::SameLine();
        ImGui::TextDisabled("AA %dx MSAA", int(sources_.presenter.samples()));
        tooltip("Anti-aliasing samples, set at start by Argon Options' MSAA or --msaa 1/2/4/8");
    }
    else
    {
        ImGui::SetNextItemWidth(110);
        ImGui::Combo("Alpha cutout", &sources_.toggles.alphaMode, "Off\0Auto\0All\0");
        ImGui::Checkbox("Cull back faces", &sources_.toggles.cull);
    }
    if (sources_.device.wireframeSupported())
    {
        ImGui::SameLine();
        ImGui::Checkbox("Wire", &sources_.toggles.wireframe);
    }
}

void EditorPanel::drawShipsAndPickups()
{
    ImGui::Checkbox("Ships on the start grid", &sources_.toggles.ships);
    ImGui::Checkbox("Pickup models", &sources_.toggles.pickupModels);
    drawEffectToggles();
    if (sources_.toggles.trails)
    {
        drawTrailLook();
    }
    if (!sources_.startGrid.pilots.empty())
    {
        drawPlayerShipPicker();
    }
}

void EditorPanel::drawEffectToggles()
{
    ImGui::SameLine();
    ImGui::Checkbox("Ship shadows", &sources_.toggles.shadows);
    if (sources_.toggles.shadows)
    {
        ImGui::SameLine();
        ImGui::Checkbox("Solid", &sources_.toggles.solidShadows);
    }
    ImGui::SameLine();
    ImGui::Checkbox("Trail", &sources_.toggles.trails);
    ImGui::SameLine();
    ImGui::Checkbox("Track sprites", &sources_.toggles.trackSprites);
    tooltip("sprite.dat's glows, clouds and trees (0xbdf40)");
    ImGui::SameLine();
    ImGui::Checkbox("Spray", &sources_.toggles.spray);
    if (!sources_.toggles.spray)
    {
        return;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Vertex alpha", &sources_.toggles.sprayVertexAlpha);
    tooltip("On: puff alpha from the vertex only; off: texture x vertex alpha");
    ImGui::SameLine();
    ImGui::Checkbox("Droplets", &sources_.toggles.drops);
    tooltip("Sideways water sheets (0x9d2c0)");
}

void EditorPanel::drawTrailLook()
{
    static const char *kModes[] = {"Stage 0 only", "Modulate", "Modulate 2x", "Modulate 4x", "Add"};
    ImGui::SetNextItemWidth(130);
    ImGui::Combo("Trail look", &sources_.toggles.trailMode, kModes, 5);
    tooltip("The game leaves the trail's texture combine to whatever was drawn before it;\npick the one that matches.");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90);
    ImGui::SliderFloat("Gain", &sources_.toggles.trailGain, 0.5f, 8.f, "%.1fx", ImGuiSliderFlags_Logarithmic);
}

void EditorPanel::drawPlayerShipPicker()
{
    ImGui::SetNextItemWidth(150);
    if (!ImGui::BeginCombo("Your ship (slot 0)", sources_.startGrid.pilots[sources_.startGrid.playerPilot].c_str()))
    {
        return;
    }
    for (int i = 0; i < int(sources_.startGrid.pilots.size()); ++i)
    {
        if (ImGui::Selectable(sources_.startGrid.pilots[i].c_str(), i == sources_.startGrid.playerPilot))
        {
            host_.choosePlayerPilot(i);
        }
    }
    ImGui::EndCombo();
}

void EditorPanel::drawAudio()
{
    if (!sources_.audio.ok())
    {
        ImGui::TextDisabled(sources_.audioEnabled ? "no audio device" : "muted (--mute / --shot)");
    }
    drawAudioVolumes();
    ImGui::Checkbox("Show emitter markers", &sources_.toggles.emitters);
    tooltip("Cross = sound position, inner ring MINDIST (full volume),\nouter ring MAXDIST (silent)");
    drawEngineAndOneShots();
}

void EditorPanel::drawAudioVolumes()
{
    argon::Audio &audio = sources_.audio;
    ImGui::Checkbox("Mute", &audio.muted);
    tooltip("The editor opens muted");
    ImGui::SliderFloat("Master", &audio.master, 0.f, 1.f, "%.2f");
    ImGui::Checkbox("Music", &audio.music);
    volumeSlider("##music", &audio.musicVol);
    if (!audio.nowPlaying().empty())
    {
        ImGui::TextDisabled("  %s", audio.nowPlaying().c_str());
    }
    ImGui::Checkbox("Ambience", &audio.ambient);
    volumeSlider("##amb", &audio.ambientVol);
    countedCheckbox(&audio.emitterSounds, "Emitters (%zu)", audio.emitters().size());
    volumeSlider("##emit", &audio.emitterVol);
}

void EditorPanel::drawEngineAndOneShots()
{
    argon::Audio &audio = sources_.audio;
    countedCheckbox(&audio.engine, "Engine: %s (%d layers)", sources_.enginePilot.c_str(), audio.engineLayers());
    ImGui::SetNextItemWidth(200);
    ImGui::SliderFloat("Engine (% top speed)", &audio.throttle, 0.f, 120.f, "%.0f");
    if (ImGui::Button("Countdown 3-2-1-GO"))
    {
        audio.playCountdown();
    }
    const auto &shots = audio.oneShots();
    for (int i = 0; i < int(shots.size()); ++i)
    {
        if (i % 4)
        {
            ImGui::SameLine();
        }
        if (ImGui::SmallButton(shots[i].c_str()))
        {
            audio.playOneShot(i);
        }
    }
}

// ------------------------------------------------------------------ AI drive
void EditorPanel::drawAiDrive()
{
    ImGui::TextWrapped("%s", sources_.routeReport.summary().c_str());
    if (!sources_.route.valid())
    {
        return;
    }
    drawDriveButtons();
    drawTurbo();
    drawViewPicker();
    drawPickupRules();
    drawRaceSetup();
    drawDriveStats();
    ImGui::Separator();
    drawViability();
}

void EditorPanel::drawDriveButtons()
{
    if (ImGui::Button(sources_.driving ? "Stop" : "Start"))
    {
        sources_.driving = !sources_.driving;
        if (sources_.driving)
        {
            sources_.chaseCam = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset"))
    {
        host_.resetDriver();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Chase camera", &sources_.chaseCam);
    ImGui::SameLine();
    if (ImGui::Checkbox("Drive yourself", &sources_.playerDrive) && sources_.playerDrive)
    {
        sources_.chaseCam = true;
    }
    tooltip("Keys: W/Up throttle, A D / arrows steer, Q E airbrakes, R dive, F climb, S/Down brake\n"
            "Pad (the game's mapping): RT throttle, left stick / d-pad steer (forward = dive,\n"
            "back = climb), LT = airbrake on the side you steer, A / Space turbo,\n"
            "B / Y / X / LB or 1-4 fire; RB brake (not on the game's pad).\n"
            "Press Start to begin; the free-camera keys are off while you drive.");
}

// viewer only: the game fills the turbo to (1.5 + 0.5 x level) x laps at each lap line (docs\physics\turbo-flaps-cameras.md "Turbo")
void EditorPanel::drawTurbo()
{
    if (ImGui::Button("+5 s turbo"))
    {
        sources_.player.addTurbo(5.f);
    }
    tooltip("Viewer only: the game fills the turbo to (1.5 + 0.5 x Turbo level) x laps\n"
            "at the first lap-line pass after GO and at every lap");
    ImGui::SameLine();
    ImGui::Text("turbo %.1f s", sources_.player.turboSeconds());
}

void EditorPanel::drawViewPicker()
{
    std::vector<std::string> names = {"Cockpit", "Chase close", "Chase mid", "Chase far", "TV (track cameras)", "Viewer chase"};
    for (int camera : DriveCamera::shipCameras(sources_.track.data.cameras))
    {
        names.push_back("Ship: " + sources_.track.data.cameras[camera].name);
    }
    if (sources_.driveView.view() >= int(names.size()))
    {
        sources_.driveView.choose(DriveCamera::kMidTripod);
    }
    ImGui::SetNextItemWidth(170);
    if (ImGui::BeginCombo("View", names[size_t(sources_.driveView.view())].c_str()))
    {
        for (int i = 0; i < int(names.size()); ++i)
        {
            if (ImGui::Selectable(names[i].c_str(), i == sources_.driveView.view()))
            {
                host_.chooseDriveView(i);
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::Checkbox("HUD", &sources_.showHud);
    if (sources_.showHud)
    {
        drawHudPreview();
    }
}

void EditorPanel::drawHudPreview()
{
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90);
    ImGui::SliderInt3("Power-ups", sources_.powerLevels, 0, 3);
    tooltip("Preview the power-up panel: red / blue / yellow level");
    ImGui::SameLine();
    if (ImGui::Checkbox("Special", &sources_.hudSpecial))
    {
        sources_.weaponSpecial = sources_.hudSpecial ? 1 : 0;
    }
}

void EditorPanel::drawPickupRules()
{
    ImGui::Checkbox("Collect pickups", &sources_.pickupLogic);
    tooltip("The game's pickup logic: drive within 14 m to take one (power-up level +1, mega-up all three,\n"
            "a special once all are at 3), starburst + sound; it comes back 0.4 s later and shows the\n"
            "_ns model until your next lap. Power chips never come back.");
    ImGui::SameLine();
    if (ImGui::Checkbox("Caps from upgrade", &sources_.upgradeCapsAuto))
    {
        host_.applyUpgradeCaps();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("The game's pickup caps from the character's Weapon Strength level (0x3f650):\n"
                          "level %d: red %d, yellow %d, blue %d, special %d",
                          host_.upgradeLevel(argon::upgrades::kWeaponStrength),
                          sources_.pickupCaps.red,
                          sources_.pickupCaps.yellow,
                          sources_.pickupCaps.blue,
                          sources_.pickupCaps.special);
    }
    if (!sources_.upgradeCapsAuto)
    {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        if (ImGui::SliderInt("Level cap", &sources_.levelCap, 1, 3))
        {
            host_.applyUpgradeCaps();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        ImGui::SliderInt("Special cap", &sources_.pickupCaps.special, 0, 1);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Reset pickups"))
    {
        host_.resetPickups();
    }
    if (sources_.driveView.view() == DriveCamera::kTv && sources_.driveView.tvCamera() >= 0)
    {
        ImGui::TextDisabled("  TV: %s", sources_.track.data.cameras[size_t(sources_.driveView.tvCamera())].name.c_str());
    }
}

void EditorPanel::drawRaceSetup()
{
    static const char *kClasses[5] = {"Speed 1", "Speed 2", "Speed 3", "Speed 4", "Speed 5"};
    ImGui::SetNextItemWidth(200);
    ImGui::SliderFloat("Time scale", &sources_.driveScale, 0.25f, 2.f, "%.2fx");
    ImGui::SetNextItemWidth(120);
    if (ImGui::Combo("Speed class", &sources_.speedClass, kClasses, 5))
    {
        host_.resetDriver();
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90);
    ImGui::SliderFloat("AI power", &sources_.aiPower, 60.f, 120.f, "%.0f%%");
    drawOpponents();
    if (!sources_.race.grid().empty() && sources_.simStarted)
    {
        drawPlaces();
    }
}

void EditorPanel::drawOpponents()
{
    Race &race = sources_.race;
    const int maxOpponents = std::max(0, std::min(int(sources_.track.data.starts.size()) - 1, Weapons::kShips - 1));
    ImGui::SetNextItemWidth(120);
    if (ImGui::SliderInt("Opponents", &race.droneCount, 0, maxOpponents))
    {
        host_.resetDriver();
    }
    tooltip("Grid slots 1..n race with the game's AI + physics (quick race: 5)");
    ImGui::SameLine();
    if (ImGui::SmallButton("New grid"))
    {
        host_.reshuffleGrid();
    }
    tooltip("The game's quick-race grid: you, your nemesis in slot 1, the rest random");
    ImGui::SameLine();
    ImGui::Checkbox("Catch-up", &race.catchUpOn);
    tooltip(race.catchUpLoaded() ? "The game's rubber band (0x6d650): config\\ai_values_<track>.txt zones vs your race distance"
                                 : "No config\\ai_values_<track>.txt for this track");
    ImGui::SameLine();
    ImGui::Checkbox("AI routes", &race.routesOn);
    tooltip("The AI's route at the forks (0xe1370: a list per pilot, lap and slot); off: each fork's first link");
    ImGui::SameLine();
    ImGui::Checkbox("Ship contacts", &race.contactsOn);
}

void EditorPanel::drawPlaces()
{
    const Race &race = sources_.race;
    const auto place = race.places();
    std::string line = "Places:";
    for (int p = 1; p <= race.racerCount(); ++p)
    {
        for (int i = 0; i < race.racerCount(); ++i)
        {
            if (place[size_t(i)] == p)
            {
                line += "  " + std::to_string(p) + ". " + argon::pilotName(race.racer(i).pilot()) + (i == 0 ? " (you)" : "");
            }
        }
    }
    ImGui::TextWrapped("%s", line.c_str());
    ImGui::TextDisabled("ship contacts %d", race.contacts());
}

void EditorPanel::drawDriveStats()
{
    const argon::GameCraft &sim = sources_.player;
    ImGui::Checkbox("Engine sound follows the drive (game mapping)", &sources_.linkEngine);
    if (!sources_.craftStats.loaded())
    {
        ImGui::TextColored(ImVec4(1, .4f, .3f, 1), "config\\character_stats.txt not loaded");
    }
    ImGui::Text("%s  %.0f km/h  gear %d  revs %.0f  throttle %.0f",
                argon::pilotName(argon::pilotIndex(sources_.startGrid.pilotOf(0))),
                sim.speedKmh(),
                sim.gear() + 1,
                sim.revs(),
                sim.throttle());
    ImGui::Text("lap %d  %.1f s   %s%s  node %d",
                sim.laps() + 1,
                sim.lapTime(),
                sim.onGround() ? sim.surfaceName() : "airborne",
                sim.inWater() ? " (water)" : "",
                sim.node());
    if (sim.laps() > 0)
    {
        ImGui::Text("last %.2f s   best %.2f s", sim.lastLap(), sim.bestLap());
    }
    ImGui::Text(
        "steer L %.0f R %.0f  err %+.3f  wall hits %d  respawns %d", sim.steerLeft(), sim.steerRight(), sim.steerError(), sim.contacts(), sim.respawns());
}

void EditorPanel::drawViability()
{
    const auto &issues = sources_.routeReport.issues();
    const bool bad = std::any_of(issues.begin(), issues.end(), isProblem);
    ImGui::TextColored(bad ? ImVec4(1, .7f, .3f, 1) : ImVec4(.4f, 1, .5f, 1), "Viability: %s", host_.viabilityText().c_str());
    ImGui::Checkbox("Show issue markers", &sources_.toggles.issues);
    tooltip("Pylons on the route: red = no road, orange = wall,\nyellow = ledge, blue = jump/drop (info)");
    drawIssueList();
}

void EditorPanel::drawIssueList()
{
    const auto &issues = sources_.routeReport.issues();
    for (int i = 0; i < int(issues.size()) && i < 60; ++i)
    {
        ImGui::PushID(i);
        if (ImGui::SmallButton("Go"))
        {
            sources_.chaseCam = false;
            sources_.freeCamera.lookAtFrom(issues[i].pos, glm::vec3(18, 12, 18));
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, issues[i].problem() ? ImVec4(1, .8f, .5f, 1) : ImVec4(.6f, .85f, 1, 1));
        ImGui::TextWrapped("%s", issues[i].text().c_str());
        ImGui::PopStyleColor();
        ImGui::PopID();
    }
}

void EditorPanel::drawCamera()
{
    ImGui::SliderFloat("Speed", &sources_.freeCamera.speed, 2.f, 1000.f, "%.0f", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("FOV", &sources_.freeCamera.fov, 30.f, 110.f, "%.0f");
    if (ImGui::Button("Go to start"))
    {
        host_.cameraToStart();
    }
    ImGui::SameLine();
    if (ImGui::Button("Top view"))
    {
        host_.cameraToTop();
        sources_.toggles.fog = false;
    }   // track fog whites out at altitude
    ImGui::TextDisabled("game pos  %.1f  %.1f  %.1f", sources_.freeCamera.position.x, sources_.freeCamera.position.y, sources_.freeCamera.position.z);
    ImGui::TextDisabled("RMB look, WASD move, Q/E down/up, Shift fast");
}

void EditorPanel::drawWarnings()
{
    const auto &warnings = sources_.track.warnings;
    if (warnings.empty())
    {
        ImGui::TextDisabled("No warnings for this track");
        return;
    }
    for (auto &warning : warnings)
    {
        ImGui::TextWrapped("%s", warning.c_str());
    }
}
