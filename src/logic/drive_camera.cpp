// Argon: the drive views (drive_camera.hpp).
#include "logic/drive_camera.hpp"

#include <algorithm>
#include <cmath>

namespace
{
constexpr int kInternalTripod = 3;

glm::vec3 vec(const std::array<float, 3> &v)
{
    return {v[0], v[1], v[2]};
}

// the ship's heading on the ground, `otherwise` when it points straight up or down
glm::vec3 levelHeading(const glm::vec3 &forward, const glm::vec3 &otherwise)
{
    return glm::length(glm::vec2(forward.x, forward.z)) > 1e-3f ? glm::normalize(glm::vec3(forward.x, 0, forward.z)) : otherwise;
}

// a moving camera's share of its pan: the lap percentage's way from panFrom to panTo, both wrapping at 100
float panShare(const argon::TrackCamera &camera, float lapPercent)
{
    float span = camera.panTo - camera.panFrom;
    if (span < 0)
    {
        span += 100.f;
    }
    float into = lapPercent - camera.panFrom;
    if (into < 0)
    {
        into += 100.f;
    }
    return span > 0 ? std::clamp(into / span, 0.f, 1.f) : 0.f;
}
}  // namespace

void DriveCamera::choose(int view)
{
    view_ = view;
    tvCamera_ = -1;
}

int DriveCamera::view() const
{
    return view_;
}

bool DriveCamera::showsTripod() const
{
    return view_ <= kInternalTripod;
}

int DriveCamera::tripodIndex() const
{
    return view_ == kCockpit ? kInternalTripod : view_ <= kInternalTripod ? view_ - 1 : kMidTripod;
}

bool DriveCamera::internalTripod() const
{
    return tripodIndex() == kInternalTripod;
}

int DriveCamera::tvCamera() const
{
    return tvCamera_;
}

bool DriveCamera::covers(const argon::TrackCamera &camera, float lapPercent, const glm::vec3 &ship)
{
    if (camera.type != "static" && camera.type != "moving" && camera.type != "heli")
    {
        return false;
    }
    const bool inWindow = camera.panFrom <= camera.panTo ? (lapPercent >= camera.panFrom && lapPercent <= camera.panTo)
                                                         : (lapPercent >= camera.panFrom || lapPercent <= camera.panTo);
    if (!inWindow)
    {
        return false;
    }
    return camera.type != "static" || glm::length(ship - vec(camera.pos)) <= camera.range;
}

bool DriveCamera::tvShot(const std::vector<argon::TrackCamera> &cameras, float lapPercent, const glm::mat4 &ship, ViewShot &shot)
{
    const glm::vec3 position(ship[3]);
    const bool keep = tvCamera_ >= 0 && tvCamera_ < int(cameras.size()) && covers(cameras[size_t(tvCamera_)], lapPercent, position);
    if (!keep)
    {
        int pick = -1;
        for (int i = 0; i < int(cameras.size()) && pick < 0; ++i)
        {
            if (i != tvCamera_ && covers(cameras[size_t(i)], lapPercent, position))
            {
                pick = i;
            }
        }
        tvCamera_ = pick;
    }
    if (tvCamera_ < 0)
    {
        return false;
    }
    shot = tvCameraShot(cameras[size_t(tvCamera_)], lapPercent, ship);
    return true;
}

// moving: from pos to pos2 (and fov to fov2) over its pan; heli: half its height behind and its height up; static: at
// its place, following the ship (its target is only the first aim), the dynamic fov keeping the ship a similar size
ViewShot DriveCamera::tvCameraShot(const argon::TrackCamera &camera, float lapPercent, const glm::mat4 &ship)
{
    const glm::vec3 position(ship[3]), forward(ship[2]);
    ViewShot shot;
    shot.look = position;
    shot.fov = camera.fov;
    if (camera.type == "moving")
    {
        const float share = panShare(camera, lapPercent);
        shot.eye = glm::mix(vec(camera.pos), vec(camera.pos2), share);
        shot.fov = camera.fov + (camera.fov2 - camera.fov) * share;
    }
    else if (camera.type == "heli")
    {
        shot.eye = position - levelHeading(forward, glm::vec3(0, 0, 1)) * camera.height * 0.5f + glm::vec3(0, camera.height, 0);
    }
    else
    {
        shot.eye = vec(camera.pos);
        if (camera.dynamicFov)
        {
            const float distance = glm::length(position - shot.eye);
            shot.fov = std::clamp(glm::degrees(2.f * std::atan(14.f / std::max(1.f, distance))), 4.f, camera.fov);
        }
    }
    return shot;
}

ViewShot DriveCamera::chaseShot(const glm::mat4 &ship, const glm::vec3 &from, float dt, float fov)
{
    const glm::vec3 position(ship[3]), up(ship[1]), forward(ship[2]);
    const glm::vec3 heading = levelHeading(forward, forward);
    const glm::vec3 wanted = position - heading * 15.f + glm::vec3(0, 5.5f, 0) + up * 0.5f;
    ViewShot shot;
    shot.eye = glm::mix(from, wanted, 1.f - std::exp(-dt / 0.12f));
    shot.look = position + heading * 10.f;
    shot.fov = fov;
    return shot;
}

ViewShot DriveCamera::shipMountedShot(const std::vector<argon::TrackCamera> &cameras, const glm::mat4 &ship) const
{
    const glm::vec3 position(ship[3]), right(ship[0]), up(ship[1]), forward(ship[2]);
    const std::vector<int> mounted = shipCameras(cameras);
    ViewShot shot;
    if (mounted.empty())
    {
        shot.eye = position - forward * 8.f + up * 2.f;
        shot.look = position;
        return shot;
    }
    const int which = std::clamp(view_ - kFirstShipMounted, 0, int(mounted.size()) - 1);
    const argon::TrackCamera &camera = cameras[size_t(mounted[size_t(which)])];
    shot.eye = position + forward * camera.pos[0] - right * camera.pos[1] + up * camera.pos[2];
    shot.look = position + forward * camera.target[0] - right * camera.target[1] + up * camera.target[2];
    shot.up = up;
    shot.fov = camera.fov;
    return shot;
}

std::vector<int> DriveCamera::shipCameras(const std::vector<argon::TrackCamera> &cameras)
{
    std::vector<int> mounted;
    for (size_t i = 0; i < cameras.size(); ++i)
    {
        if (cameras[i].type == "ship")
        {
            mounted.push_back(int(i));
        }
    }
    return mounted;
}
