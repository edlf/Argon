// Argon: the drive views. 0 cockpit, 1 close, 2 mid, 3 far are the game's tripods (the craft's, +0xe24); 4 TV is the
// viewer's director over config\camera_<t>.txt's static / moving / heli cameras (each covering a stretch of the lap);
// 5 the viewer's level chase; 6.. the track's ship-mounted cameras. App::driveCamera picks the shot: the knock-out's
// death camera and the replay director (race end, replays) go first, the tripods are the craft's; this holds the rest.
#pragma once
#include "track/trackdata.hpp"

#include <glm/glm.hpp>

#include <vector>

struct ViewShot
{
    glm::vec3 eye{0.f}, look{0.f, 0.f, 1.f}, up{0.f, 1.f, 0.f};
    float fov = 60.f;   // every tripod is created with fov 60 (+0xcc4)
};

class DriveCamera
{
  public:
    static constexpr int kCockpit = 0, kMidTripod = 2, kTv = 4, kViewerChase = 5, kFirstShipMounted = 6;

    void choose(int view);   // the TV director starts again
    int view() const;
    bool showsTripod() const;      // views 0..3
    int tripodIndex() const;       // the craft's tripod for the view (0 close .. 3 internal); the mid one otherwise
    bool internalTripod() const;   // the game's "tripod internal" (+0xe24 0, 0x5aa10)
    int tvCamera() const;          // the TV camera showing, -1 none

    // TV: the camera showing while it covers the ship's lap percentage (static ones only within their range), else the
    // first other one that does; false when nothing covers the stretch (the mid tripod shows)
    bool tvShot(const std::vector<argon::TrackCamera> &cameras, float lapPercent, const glm::mat4 &ship, ViewShot &shot);
    // the viewer's chase: 15 behind, 5.5 up, eased from `from` over 0.12 s, level
    static ViewShot chaseShot(const glm::mat4 &ship, const glm::vec3 &from, float dt, float fov);
    // camera_<t>.txt ship space: x forward, y left, z up ("Top behind" = -4 0 1)
    ViewShot shipMountedShot(const std::vector<argon::TrackCamera> &cameras, const glm::mat4 &ship) const;

    static std::vector<int> shipCameras(const std::vector<argon::TrackCamera> &cameras);   // indices of the "ship" ones

  private:
    static bool covers(const argon::TrackCamera &camera, float lapPercent, const glm::vec3 &ship);
    static ViewShot tvCameraShot(const argon::TrackCamera &camera, float lapPercent, const glm::mat4 &ship);

    int view_ = kMidTripod;
    int tvCamera_ = -1;
};
