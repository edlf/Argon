// Argon: the players' keyboard and gamepads (player_input.hpp).
#include "input/player_input.hpp"

#include <imgui.h>

#include <algorithm>

void PlayerInput::attach(GLFWwindow *window, bool alwaysFocused, int testSeats)
{
    window_ = window;
    alwaysFocused_ = alwaysFocused;
    testSeats_ = testSeats;
}

bool PlayerInput::focused() const
{
    return alwaysFocused_ || (window_ && glfwGetWindowAttrib(window_, GLFW_FOCUSED) == GLFW_TRUE);
}

bool PlayerInput::keyboardFree() const
{
    return focused() && !ImGui::GetIO().WantCaptureKeyboard;
}

float PlayerInput::keyLevel(int key) const
{
    return key != kUnboundKey && glfwGetKey(window_, key) == GLFW_PRESS ? 1.f : 0.f;
}

float PlayerInput::actionLevel(KeyAction action) const
{
    float level = 0.f;
    for (int slot = 0; slot < kKeySlots; ++slot)
    {
        level = std::max(level, keyLevel(keys_.key(action, slot)));
    }
    return level;
}

argon::GameCraft::Pad PlayerInput::keyboardPad() const
{
    argon::GameCraft::Pad pad;
    if (!keyboardFree())
    {
        return pad;
    }
    pad.throttle = actionLevel(KeyAction::Throttle);
    pad.brake = actionLevel(KeyAction::Brake);
    pad.steer = actionLevel(KeyAction::SteerRight) - actionLevel(KeyAction::SteerLeft);
    pad.airLeft = actionLevel(KeyAction::AirbrakeLeft);
    pad.airRight = actionLevel(KeyAction::AirbrakeRight);
    pad.dive = actionLevel(KeyAction::Dive);
    pad.climb = actionLevel(KeyAction::Climb);
    pad.turbo = actionLevel(KeyAction::Turbo) > 0.f;   // the pad's A
    pad.reset = actionLevel(KeyAction::Reset) > 0.f;   // the pad's BACK
    return pad;
}

argon::weapons::FireButtons PlayerInput::fireKeys() const
{
    argon::weapons::FireButtons held;
    if (keyboardFree())
    {
        held.b = actionLevel(KeyAction::FireRed) > 0.f;
        held.x = actionLevel(KeyAction::FireBlue) > 0.f;
        held.y = actionLevel(KeyAction::FireYellow) > 0.f;
    }
    return held;
}

bool PlayerInput::gamepad(int port, GLFWgamepadstate &state) const
{
    return portConnected(port) && glfwGetGamepadState(port, &state) == GLFW_TRUE;
}

bool PlayerInput::nthGamepad(int index, GLFWgamepadstate &state) const
{
    int seen = 0;
    for (int port = GLFW_JOYSTICK_1; port <= GLFW_JOYSTICK_LAST; ++port)
    {
        if (glfwJoystickIsGamepad(port) && seen++ == index)
        {
            return glfwGetGamepadState(port, &state) == GLFW_TRUE;
        }
    }
    return false;
}

bool PlayerInput::viewButtonPressed(const GLFWgamepadstate &pad)
{
    const bool held = (keyboardFree() && actionLevel(KeyAction::ChangeView) > 0.f) || pad.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER] == GLFW_PRESS;
    const bool pressed = held && !viewButtonHeld_;
    viewButtonHeld_ = held;
    return pressed;
}

void PlayerInput::startKeyCapture()
{
    for (const int key : nameableKeys())
    {
        capturedKeysDown_[size_t(key)] = keyLevel(key) > 0.f;
    }
}

bool PlayerInput::capturedKey(int &key)
{
    bool found = false;
    for (const int candidate : nameableKeys())
    {
        const bool down = keyboardFree() && keyLevel(candidate) > 0.f;
        if (down && !capturedKeysDown_[size_t(candidate)] && !found)
        {
            key = candidate;
            found = true;
        }
        capturedKeysDown_[size_t(candidate)] = down;
    }
    return found;
}

menu::MenuInput PlayerInput::readMenu()
{
    menu::MenuInput in;
    portPresses_.fill(menu::MenuInput{});
    if (!focused())
    {
        return in;
    }
    if (!ImGui::GetIO().WantCaptureKeyboard)
    {
        in.up = ImGui::IsKeyPressed(ImGuiKey_UpArrow);
        in.down = ImGui::IsKeyPressed(ImGuiKey_DownArrow);
        in.left = ImGui::IsKeyPressed(ImGuiKey_LeftArrow);
        in.right = ImGui::IsKeyPressed(ImGuiKey_RightArrow);
        in.a = ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false) || ImGui::IsKeyPressed(ImGuiKey_Space, false);
        in.b = ImGui::IsKeyPressed(ImGuiKey_Escape, false) || ImGui::IsKeyPressed(ImGuiKey_Backspace, false);
        in.x = ImGui::IsKeyPressed(ImGuiKey_X, false);
        in.y = ImGui::IsKeyPressed(ImGuiKey_Y, false);
    }
    for (int port = GLFW_JOYSTICK_1; port <= GLFW_JOYSTICK_LAST; ++port)
    {
        GLFWgamepadstate state;
        if (!glfwJoystickIsGamepad(port) || !glfwGetGamepadState(port, &state))
        {
            continue;
        }
        portPresses_[size_t(port)] = portEdges_[size_t(port)].step(argon::input::menuButtonsHeld(state));
        argon::input::mergePresses(in, portPresses_[size_t(port)]);   // any pad's press counts (not only the first GLFW lists)
    }
    return in;
}

menu::MenuInput PlayerInput::portPresses(int port) const
{
    return port >= 0 && port <= GLFW_JOYSTICK_LAST && glfwJoystickIsGamepad(port) ? portPresses_[size_t(port)] : menu::MenuInput{};
}

bool PlayerInput::portConnected(int port) const
{
    return (port >= 0 && port < testSeats_) || (port >= 0 && port <= GLFW_JOYSTICK_LAST && glfwJoystickIsGamepad(port));
}

bool PlayerInput::startPressed()
{
    if (!focused())
    {
        return false;
    }
    bool start = !ImGui::GetIO().WantCaptureKeyboard && (ImGui::IsKeyPressed(ImGuiKey_Escape, false) || ImGui::IsKeyPressed(ImGuiKey_P, false));
    for (int port = GLFW_JOYSTICK_1; port <= GLFW_JOYSTICK_LAST; ++port)
    {
        GLFWgamepadstate state;
        if (!glfwJoystickIsGamepad(port) || !glfwGetGamepadState(port, &state))
        {
            continue;
        }
        const bool now = state.buttons[GLFW_GAMEPAD_BUTTON_START] != 0;
        start = start || (now && !startHeld_);
        startHeld_ = now;
        break;   // the first pad's only
    }
    return start;
}

bool PlayerInput::anyHeld() const
{
    if (!focused())
    {
        return false;
    }
    for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; ++key)
    {
        if (ImGui::IsKeyDown(ImGuiKey(key)))
        {
            return true;
        }
    }
    for (int port = GLFW_JOYSTICK_1; port <= GLFW_JOYSTICK_LAST; ++port)
    {
        GLFWgamepadstate state;
        if (glfwJoystickIsGamepad(port) && glfwGetGamepadState(port, &state) && argon::input::anyButtonHeld(state))
        {
            return true;
        }
    }
    return false;
}
