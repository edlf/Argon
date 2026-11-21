// Argon: where a view goes on the window: the game's 640 x 480 layout (logic\split_screen.hpp) stretched over it.
#pragma once
#include "gfx/gfx_types.hpp"
#include "logic/split_screen.hpp"

#include <glm/glm.hpp>

// the window's width over its height (a zero height counts as one pixel)
float windowAspect(const argon::gfx::WindowSize &window);

// a view's projection aspect: its area's own shape on that window
float viewAspect(const argon::gfx::WindowSize &window, const argon::split::ScreenRect &area);

// the area in window pixels, its edges rounded so neighbouring views meet
argon::gfx::Rect screenArea(const argon::gfx::WindowSize &window, const argon::split::ScreenRect &area);

// the camera of the frame's first view, kept for what draws after it: the HUD's projected markers, the speed blur, the
// track sprites and the sky
struct HudCamera
{
    glm::mat4 viewProj{1.f};
    glm::vec3 eye{0.f};
    glm::vec3 forward{0, 0, 1};
};
