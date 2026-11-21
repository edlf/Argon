#include "app/scene_frame.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <set>

#include "app/editor_session.hpp"
#include "logic/race_eggs.hpp"
#include "logic/race_start.hpp"
#include "render/countdown_digits.hpp"
#include "render/editor_markings.hpp"
#include "render/projection.hpp"
#include "render/renderer.hpp"
#include "render/scene_recorder.hpp"
#include "render/scene_toggles.hpp"
#include "track/track_source.hpp"

namespace
{
const float kCdL[4] = {2.f, 3.f, 4.f, 4.4f}, kCdPx[4] = {70.f, -70.f, 70.f, -70.f};   // leave time, x offset
// on the console an arriving digit only shows once it is close (xemu frames: seen about 19 units from the camera,
// not at about 31; the cause is not decoded), so digits farther than this are not drawn
const float kCdMaxDepth = 25.f, kCdEyeZ = 10.f;

argon::gfx::Winding windingOf(bool counterClockwise)
{
    return counterClockwise ? argon::gfx::Winding::CounterClockwise : argon::gfx::Winding::Clockwise;
}
}  // namespace

ModelClock SceneFrame::modelClock(float seconds) const
{
    return ModelClock{seconds, slots_.frame};
}

int SceneFrame::viewSlot() const
{
    return slots_.frame * kMaxViews + slots_.view;
}

argon::gfx::Winding SceneFrame::sceneFrontFace() const
{
    return windingOf(sources_.toggles.frontCounterClockwise != sources_.raceEggs.on(Egg::MirrorTracks));
}

argon::gfx::Winding SceneFrame::modelFrontFace() const
{
    return windingOf(sources_.toggles.frontCounterClockwise);
}

// the GLFW clock (from glfwInit at start-up), held while the window is unfocused (App::waitForFocus)
double SceneFrame::secondsSinceStart() const
{
    return glfwGetTime();
}

void SceneFrame::loadSprayTexture()
{
    const std::string &trackName = sources_.track.name;
    if (!sources_.renderer.hasSprayTexture(trackName))
    {
        sources_.renderer.useSprayTexture(readSprayTexture(sources_.game, trackName), trackName);
    }
}

// pickups_draw 0x8fbc0 -> 0xb83e0: each model's clock (+0x5e4) is the race clock (models without header flag 0x80000),
// so every pickup of a type plays its tracks in step (the spin / bob of the shapes, docs\models\meshes.md "Animation")
void SceneFrame::animatePickupModels(float seconds)
{
    std::set<Track *> posed;
    for (const PickupModels *modelOfType : {&sources_.pickupModels, &sources_.takenPickupModels})
    {
        for (const auto &entry : *modelOfType)
        {
            if (entry.second && posed.insert(&*entry.second).second)
            {
                sources_.models.animate(*entry.second, modelClock(seconds));
            }
        }
    }
}

// the four countdown models over the 3D view (recordFrame, after the scene; the depth is cleared first, so they are
// drawn over the ship and the ground)
void SceneFrame::drawCountdown(SceneRecorder &recorder)
{
    const RaceStart &raceStart = sources_.raceStart;
    if (!raceStart.countdownShown())
    {
        return;
    }
    const auto windowSize = sources_.renderer.presenter().windowSize();
    recorder.commands().clearDepth(argon::gfx::Rect{0, 0, windowSize.width, windowSize.height}, argon::kDepthClear);
    // the game's full-screen hud viewport (0xadab0: fov pi x 0.4; aspect 4:3, 16:9 with the widescreen flag) over the
    // whole window at its aspect
    float aspect = float(windowSize.width) / std::max(float(windowSize.height), 1.f);
    glm::mat4 proj = argon::reversedPerspective(3.14159265f * 0.4f, aspect, 0.5f, 5000.f);
    // the hud camera (eye (0, 0, -10) in the game's frame) is at z +10 here, looking down -z: on the console the
    // digits come in from the distance and leave past the camera (xemu frames), which this countdownView reproduces
    glm::mat4 countdownView = glm::lookAt(glm::vec3(0, 0, kCdEyeZ), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    const float D2R = 3.14159265f / 180.f;
    for (int k = 0; k < 4; ++k)
    {
        if (!sources_.countdownDigits.digit(k).vb)
        {
            continue;
        }
        const float clock = raceStart.countdownClock(), arrival = RaceStart::digitArrival(k);
        float v = 0.f;
        bool leaving = false;
        if (clock < arrival)
        {
            v = arrival - clock;
        }
        else if (clock >= kCdL[k])
        {
            v = (kCdL[k] - clock) * 0.4f;
            leaving = true;
        }
        float ang = 50.f * v * 12.f * D2R, rise = leaving ? std::sin(kCdPx[k] * v * 11.f * D2R) : 0.f;
        glm::vec3 fwd = glm::normalize(glm::vec3(std::sin(ang), rise, std::cos(ang)));
        glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), fwd)), up = glm::cross(fwd, right);
        glm::mat4 m(glm::vec4(right, 0), glm::vec4(up, 0), glm::vec4(fwd, 0), glm::vec4(-kCdPx[k] * v, leaving ? -10.f * v : 0.f, -50.f * v, 1.f));
        if (kCdEyeZ + 50.f * v > kCdMaxDepth)
        {
            continue;
        }
        recorder.useViewProj(proj * countdownView);
        recorder.useViewPoint(countdownView);
        recorder.drawModelBothPasses(sources_.countdownDigits.digit(k), ModelPlacement::forProp(m));
    }
}

// not in a race started from the menus
void SceneFrame::drawEditorMarkings(const SceneRecorder &recorder) const
{
    if (sources_.editor.active())
    {
        recordEditorMarkings(EditorMarkingSources{sources_.track, sources_.toggles, sources_.renderer.linePipe}, recorder);
    }
}
