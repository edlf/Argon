// Argon: the crafts' effects, the ships the shadows are cast from, the speed blur.
#include "app/app.hpp"
#include "hud/rain_overlay.hpp"
#include "gfx/vulkan/vulkan_handles.hpp"
#include "render/sprite_pool.hpp"
#include "ui/feedback_swirl.hpp"
#include "ui/race_menus.hpp"

using argon::gfx::SamplerMode;

namespace
{
// the water the spray stands on: the simulation's water boxes (0xce6b0)
class SimWorldWater final : public argon::WaterQuery
{
  public:
    explicit SimWorldWater(const argon::SimWorld &world) : world_(world)
    {
    }
    std::optional<float> depthAt(const glm::vec3 &point) const override
    {
        float depth = 0.f;
        if (!world_.waterDepth(point, depth))
        {
            return std::nullopt;
        }
        return depth;
    }

  private:
    const argon::SimWorld &world_;
};
}  // namespace

void App::stepEffectsClock()
{
    simClock += 1.f / 60.f;
    wakeSpray.age(simClock);
}

void App::followCraftEffects(const argon::GameCraft &craft, const glm::mat4 &matrix, int slot)
{
    const float step = 1.f / 60.f;
    engineTrails.follow(slot, matrix, argon::TrailDrive{craft.forwardSpeed(), craft.turboOn()});
    exhausts.updateJet(slot, argon::JetDrive{craft.throttle(), craft.turboOn()}, step);
    exhausts.updateFlaps(slot, craft, step);
    const SimWorldWater water(simWorld);
    const argon::WakeCraft wake{slot, matrix, craft.wakeCorners() >= argon::kWakeCornersNeeded};
    wakeSpray.follow(wake, argon::WakeStep{simClock, viewerRand, water});
}

std::vector<std::pair<Track *, glm::mat4>> App::visibleShips()
{
    std::vector<std::pair<Track *, glm::mat4>> out;
    if (!toggles.ships)
    {
        return out;
    }
    for (size_t i = 0; i < track.data.starts.size(); ++i)
    {
        const Found<Track> ship = props.loadedCraft(startGrid.pilotOf(int(i)));
        if (!ship || !slotShown(int(i)))
        {
            continue;
        }
        glm::mat4 pose = craftPoses.slotMatrix(int(i));
        if (i == 0 && session.driving && session.chaseCam && driveView.view() == DriveCamera::kCockpit)
        {
            continue;   // cockpit view: own ship hidden
        }
        out.push_back({&*ship, pose});
    }
    // the race scene's shadow loop (0x93871) skips only hidden (+0x1129) and dead (+0x1145) crafts: ghosts cast one
    for (int g = 0; g < 2; ++g)
    {
        if (const Found<Track> model = shownGhostModel(g))
        {
            out.push_back({&*model, modes.timeAttack().ghostMatrix(g, craftPoses.simAlpha())});
        }
    }
    return out;
}

// a Time Attack ghost's craft model while it is shown: the loaded ghost (0) in its character's craft, the session
// ghost (1) in the player's; nullptr otherwise
Found<Track> App::shownGhostModel(int ghost)
{
    if (!logic().hasGhosts())
    {
        return {};
    }
    const TimeAttackLogic &ta = modes.timeAttack();
    if (!ta.ghostShown(ghost))
    {
        return {};
    }
    return props.loadedCraft(ghost == 0 ? argon::pilotFolder(std::clamp(ta.loadedGhost().pilot, 0, 15)) : startGrid.pilotOf(0));
}

void App::recordShadows(argon::gfx::CommandList &commands)
{
    renderer.clearShadows();
    if (!toggles.shadows || simGrid.empty())
    {
        return;
    }
    std::vector<ShadowedShip> ships;
    for (auto &[ship, placement] : visibleShips())
    {
        if (ships.size() < size_t(ShipShadows::kMaxShadows))
        {
            models.refreshCubes(*ship, renderer.trackPool, trackSerial);
            ships.push_back({ship, placement});
        }
    }
    renderer.recordShadows(commands, ships, Renderer::ShadowView{sceneFrame.slots().frame, sceneFrame.viewSlot(), sunDirection(track.data.props), simGrid, toggles.solidShadows});
}

// the race scene's echo (render\speed_blur.hpp): the blur over the 3D view (the camera's speed read every frame), then the
// echo copied for the next frame and drawn back, the rain over it, and the underwater overlay when the eye is under the
// water (+0x34e) or with Atlantis Mode (0x52c8bb); not while the pause or upgrade background holds the race frame
void App::drawSpeedBlur()
{
    const BlurLook look{fixesMarsSpeedBlur(), raceEggs.on(Egg::HyperBlur)};
    const float speed = speedBlur.cameraSpeed(hudCamera, look);
    if (raceMenus->holdsRaceFrame() || !session.driving || !frameCopies.fitMenuFeedback() || !frameCopies.menuFeedbackImage())
    {
        return;
    }
    ImDrawList *drawList = ImGui::GetBackgroundDrawList();
    if (viewerSettings.speedBlur())
    {
        speedBlur.drawOver(drawList, look, speed);
    }
    frameCopies.cutToMenuFeedback(drawList);   // the copy restarts the pass cleared
    speedBlur.drawEcho(drawList);
    RainOverlay(RainOverlaySources{weather.rain(), hudCamera, frameCopies, painter}).draw(drawList, ImGui::GetIO().DisplaySize);
    if (raceEggs.on(Egg::AtlantisMode) || cameraUnderWater())
    {
        speedBlur.drawUnderwater(drawList, simClock);
    }
}

bool App::fixesMarsSpeedBlur() const
{
    return viewerSettings.marsSpeedBlurFix() && onMarsTrack();
}

