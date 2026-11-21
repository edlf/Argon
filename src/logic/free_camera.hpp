// Argon: the viewer's free camera: where it is, which way it looks (yaw about world up, pitch above the horizon) and its
// fov. In the editor the right mouse button turns it and the keys move it; while a drive camera, the loading flyby or a
// story scene owns the view it stands at that view's eye (the sky's centre, the audio listener and the read-outs).
#pragma once
#include <glm/glm.hpp>

class FreeCamera
{
  public:
    glm::vec3 position{0, 20, 0};
    float yaw = 0, pitch = -0.2f;   // radians
    float speed = 40.f;             // the editor's keys, m/s (x5 with Shift)
    float fov = 70.f;               // degrees

    glm::vec3 forward() const;
    glm::mat4 view() const;
    void face(const glm::vec3 &direction);   // normalised
    void lookAtFrom(const glm::vec3 &target, const glm::vec3 &offset);
    // this frame's cursor: while the button stays held its movement turns the camera
    void followCursor(const glm::dvec2 &cursor, bool held);
    bool dragging() const;
    // the editor's keys: x along the view, y across it, z up, each -1..1
    void move(const glm::vec3 &keys, float dt, bool fast);

  private:
    bool dragging_ = false;
    glm::dvec2 lastCursor_{0.0};
};
