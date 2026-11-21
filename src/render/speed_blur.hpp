// Argon: what the race scene draws back over its 3D view from the echo (race scene 0x93490, single player): the speed
// blur (the last frame zoomed out and faded once the camera moves fast; Hyper Blur 0x52c8ba swaps it for the menus'
// swirl), the echo itself after its copy, and the underwater overlay (0x92340). The echo is FrameCopies' feedback copy.
#pragma once
#include "render/camera_speed.hpp"

#include <imgui.h>

class FrameCopies;

// how the blur looks this frame: Argon Options and the eggs
struct BlurLook
{
    bool marsFix;     // Mars Flicker Fix on a Mars track: the steady speed, no zoom
    bool hyperBlur;   // the Hyper Blur egg
};

class SpeedBlur
{
  public:
    explicit SpeedBlur(FrameCopies &copies);

    float cameraSpeed(const HudCamera &camera, const BlurLook &look);   // read every frame, the blur drawn or not
    void drawOver(ImDrawList *drawList, const BlurLook &look, float speed);   // the zoom or the swirl
    void drawEcho(ImDrawList *drawList);                                      // after the copy (0x93b39)
    void drawUnderwater(ImDrawList *drawList, float clock);                   // over the rain (0x93b64)

  private:
    void drawZoom(ImDrawList *drawList, const BlurLook &look, float speed);
    void drawHyperBlur(ImDrawList *drawList, float speed);

    FrameCopies &copies_;
    CameraSpeed speed_;
};
