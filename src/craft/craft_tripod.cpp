// Argon: the chase cameras' tripod (0x5c700, 0x5be00) (GameCraft, craft\craftsim.hpp; docs\physics\README.md).
#include "craft/craftsim.hpp"
#include "files/game_path.hpp"
#include "logic/blast_push.hpp"
#include "logic/rumble_rules.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

#include <glm/gtc/type_ptr.hpp>
#include "craft/row_major.hpp"

namespace argon
{

// 0x5c700: sideways camera offset from the averaged lateral velocity and the spin about up
void GameCraft::tripodUpdate(float dt)
{
    const State &state = currentState();
    for (int i = 11; i > 0; --i)
    {
        latHist_[i] = latHist_[i - 1];
    }
    latHist_[0] = glm::dot(state.velocity, col(state.rotation, 0));
    float avg = 0;
    for (float v : latHist_)
    {
        avg += v;
    }
    avg *= 0.0833333358f;
    static const char *side[3] = {"camera_external_close_sideshift", "camera_external_mid_sideshift", "camera_external_far_sideshift"};
    static const float spin[3] = {0.03f, 0.05f, 0.05f};
    for (int k = 0; k < 3; ++k)
    {
        float x = st(side[k]) * avg * -0.01f;   // global per-view factor; the sideshift values are the likely source
        x = std::clamp(x, -2.5f, 2.5f);
        x = x > 0 ? x * x : -x * x;
        x += glm::dot(up_, state.angularMomentum) * spin[k];
        float step = (x - sway_[k]) * dt;
        bool fast = x <= 0.f ? sway_[k] <= x : x <= sway_[k];
        sway_[k] += fast ? step * 3.f : step * 2.f;
    }
    // internal camera (0x5aa10): head tilt from the steering inputs, averaged over turn_headtilt_buffer frames
    {
        upSmooth_ += (col(state.rotation, 1) - upSmooth_) * 0.5f;
        if (glm::length(upSmooth_) > 1e-4f)
        {
            upSmooth_ = glm::normalize(upSmooth_);
        }
        float headTilt = (onGround_ || inWater_) ? (ctrl_[6] - ctrl_[5]) * 0.01f * st("turn_headtilt_amount") : 0.f;
        headTilt = headTilt > 0 ? headTilt * headTilt : -headTilt * headTilt;
        int tiltFrames = std::clamp(int(st("turn_headtilt_buffer")), 1, 64);
        tiltHist_[tiltIdx_] = headTilt;
        tiltIdx_ = (tiltIdx_ + 1) % tiltFrames;
        float sum = 0;
        for (int i = 0; i < tiltFrames; ++i)
        {
            sum += tiltHist_[i];
            tiltHist_[i] *= 0.9f;
        }
        const float meanTilt = sum / float(tiltFrames);
        float rate = (ctrl_[6] == 0.f && ctrl_[5] == 0.f) ? st("turn_headtilt_falloff") : st("turn_headtilt_gain");
        tilt_ += (meanTilt - tilt_) * rate * dt * 0.06f;
    }
    // camera up (0x5be00): follows the craft only when it is far from upright, else back to world up
    if (up_.y <= 0.2f)
    {
        camUp_ += (up_ - camUp_) * 0.05f;
    }
    else
    {
        camUp_ += (glm::vec3(0, 1, 0) - camUp_) * 0.1f;
    }
}

GameCraft::Tripod GameCraft::tripod(int view) const
{   // 0x5be00 (forward view); view 3 = internal (0x5aa10)
    if (view == 3)
    {
        const State &state = currentState();
        glm::vec3 right = col(state.rotation, 0), f = col(state.rotation, 2);
        Tripod tripod;
        tripod.pos = state.position + f * st("camera_forward") + up_ * st("camera_up");
        tripod.look = tripod.pos + f;
        // head tilt rolls the camera against the steering, on top of the smoothed craft up
        float hs = st("turn_headtilt_stiffening");
        float vlim = sp("absolute_max_speed") * 0.138888896f;
        float slowShare = std::clamp((vlim - speed_) / vlim, 0.f, 1.f);
        float tiltStiffness = (1.f - hs) + slowShare * slowShare * hs;   // +0x9b8
        const shake::ShakenView jolted = shake::jolted({-(right * tiltStiffness * tilt_) + upSmooth_, tripod.look}, jolt_.vector(), {right, col(state.rotation, 1), f});
        const shake::ShakenView shaken = shake::shaken(jolted, shake_.offset());
        tripod.look = shaken.look;
        tripod.up = glm::length(shaken.up) > 1e-4f ? glm::normalize(shaken.up) : up_;
        return tripod;
    }
    static const char *dist[3] = {"camera_external_close_distance", "camera_external_mid_distance", "camera_external_far_distance"};
    static const char *above[3] = {"camera_external_close_above", "camera_external_mid_above", "camera_external_far_above"};
    static const char *lookh[3] = {"camera_external_close_lookatheight", "camera_external_mid_lookatheight", "camera_external_far_lookatheight"};
    view = std::clamp(view, 0, 2);
    const State &state = currentState();
    glm::vec3 right = col(state.rotation, 0), up = col(state.rotation, 1), forward = col(state.rotation, 2);
    float distance = st(dist[view]);
    Tripod tripod;
    tripod.pos = state.position + up * st(above[view]) - forward * distance + right * sway_[view] * 5.f;
    tripod.look = state.position + up * ((depthSum_ - 2.2f) * 0.05f);
    tripod.look.y += st(lookh[view]) * up.y;
    glm::vec3 dir = tripod.pos - state.position;
    if (glm::length(dir) > 1e-4f)
    {
        tripod.pos = state.position + glm::normalize(dir) * distance;
    }
    tripod.up = glm::length(camUp_) > 1e-4f ? glm::normalize(camUp_) : glm::vec3(0, 1, 0);
    return tripod;
}

} // namespace argon
