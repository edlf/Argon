// Argon: what a gamepad's state means (pad_mapping.hpp).
#include "input/pad_mapping.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace argon::input
{
namespace
{
// a stick axis past the game's dead zone (12288 / 32768), rescaled to 0..1 from there
float stickBeyondDeadZone(float axis)
{
    constexpr float kDeadZone = 12288.f / 32768.f;
    return std::abs(axis) <= kDeadZone ? 0.f : std::copysign((std::abs(axis) - kDeadZone) / (1.f - kDeadZone), axis);
}

float triggerPull(float axis)   // -1 released .. 1 pulled
{
    return std::clamp((axis + 1.f) * 0.5f, 0.f, 1.f);
}

bool down(const GLFWgamepadstate &state, int button)
{
    return state.buttons[button] == GLFW_PRESS;
}
}  // namespace

void addGamepad(GameCraft::Pad &pad, const GLFWgamepadstate &state)
{
    float sx = stickBeyondDeadZone(state.axes[GLFW_GAMEPAD_AXIS_LEFT_X]);
    float sy = stickBeyondDeadZone(state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]);   // y: -1 = forward
    if (down(state, GLFW_GAMEPAD_BUTTON_DPAD_LEFT))
    {
        sx = -1.f;
    }
    if (down(state, GLFW_GAMEPAD_BUTTON_DPAD_RIGHT))
    {
        sx = 1.f;
    }
    if (down(state, GLFW_GAMEPAD_BUTTON_DPAD_UP))
    {
        sy = -1.f;
    }
    if (down(state, GLFW_GAMEPAD_BUTTON_DPAD_DOWN))
    {
        sy = 1.f;
    }
    if (sx != 0.f)
    {
        pad.steer = sx;
    }
    if (sy < 0.f)
    {
        pad.dive = std::max(pad.dive, -sy);
    }
    if (sy > 0.f)
    {
        pad.climb = std::max(pad.climb, sy);
    }
    pad.throttle = std::max(pad.throttle, triggerPull(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER]));
    const float airbrake = triggerPull(state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER]);
    if (pad.steer < 0.f)
    {
        pad.airLeft = std::max(pad.airLeft, airbrake);
    }
    if (pad.steer > 0.f)
    {
        pad.airRight = std::max(pad.airRight, airbrake);
    }
    // A is the turbo (pad bit 0x10, 0xb4800); B / X / Y fire (0x20 / 0x40 / 0x80); RB is the Black button, the view.
    // The game's pad has no brake (0x70ac0 never writes slot [9]): the keyboard's S only
    if (down(state, GLFW_GAMEPAD_BUTTON_A))
    {
        pad.turbo = true;
    }
    if (down(state, GLFW_GAMEPAD_BUTTON_BACK))
    {
        pad.reset = true;   // pad bit 0x200: put back on the track (0x71023)
    }
}

void mirrorSteering(GameCraft::Pad &pad)
{
    pad.steer = -pad.steer;
    std::swap(pad.airLeft, pad.airRight);
}

menu::MenuInput menuButtonsHeld(const GLFWgamepadstate &state)
{
    menu::MenuInput held;
    held.up = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] || state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] < -0.5f;
    held.down = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] || state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] > 0.5f;
    held.left = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] || state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] < -0.5f;
    held.right = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] || state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] > 0.5f;
    held.a = state.buttons[GLFW_GAMEPAD_BUTTON_A] || state.buttons[GLFW_GAMEPAD_BUTTON_START] != 0;
    held.b = state.buttons[GLFW_GAMEPAD_BUTTON_B] != 0;
    held.x = state.buttons[GLFW_GAMEPAD_BUTTON_X] != 0;
    held.y = state.buttons[GLFW_GAMEPAD_BUTTON_Y] != 0;
    return held;
}

void mergePresses(menu::MenuInput &into, const menu::MenuInput &more)
{
    into.up = into.up || more.up;
    into.down = into.down || more.down;
    into.left = into.left || more.left;
    into.right = into.right || more.right;
    into.a = into.a || more.a;
    into.b = into.b || more.b;
    into.x = into.x || more.x;
    into.y = into.y || more.y;
}

bool anyButtonHeld(const GLFWgamepadstate &state)
{
    for (unsigned char button : state.buttons)
    {
        if (button)
        {
            return true;
        }
    }
    const bool stick = std::abs(state.axes[GLFW_GAMEPAD_AXIS_LEFT_X]) > 0.25f || std::abs(state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]) > 0.25f ||
                       std::abs(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X]) > 0.25f || std::abs(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]) > 0.25f;
    const bool trigger = state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > -0.5f || state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > -0.5f;
    return stick || trigger;
}

menu::MenuInput PressEdges::step(const menu::MenuInput &held)
{
    menu::MenuInput pressed;
    pressed.up = held.up && !previous_.up;
    pressed.down = held.down && !previous_.down;
    pressed.left = held.left && !previous_.left;
    pressed.right = held.right && !previous_.right;
    pressed.a = held.a && !previous_.a;
    pressed.b = held.b && !previous_.b;
    pressed.x = held.x && !previous_.x;
    pressed.y = held.y && !previous_.y;
    previous_ = held;
    return pressed;
}
}  // namespace argon::input
