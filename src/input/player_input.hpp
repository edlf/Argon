// Argon: the players' keyboard and gamepads as the viewer reads them (GLFW, with ImGui's keyboard capture): the
// controller ports by GLFW joystick id (as the Xbox's ports 0..3, each keeping its number while the others come and
// go), the craft's keys, the menus' presses per port, START and the Black button on their press. No key or pad is read
// while the window has no focus (a --shot run's hidden window always has it). What the states mean: pad_mapping.hpp.
#pragma once
#include "craft/craftsim.hpp"
#include "logic/weapon_rules.hpp"
#include "ui/menu_common.hpp"
#include "input/key_bindings.hpp"
#include "input/pad_mapping.hpp"

#include <GLFW/glfw3.h>

#include <array>

class PlayerInput
{
  public:
    explicit PlayerInput(const KeyBindings &keys) : keys_(keys)
    {
    }
    // testSeats: tests' --mp-seats, ports 0..n-1 that count as connected
    void attach(GLFWwindow *window, bool alwaysFocused, int testSeats);
    bool focused() const;

    // the craft's keys as bound (by default W / Up throttle, A D / Left Right steer, Q / E airbrakes, R dive, F climb,
    // S / Down brake, Space turbo, Backspace reset)
    argon::GameCraft::Pad keyboardPad() const;
    argon::weapons::FireButtons fireKeys() const;   // by default 1 / 3 / 2: B / X / Y (red / blue / yellow)
    bool gamepad(int port, GLFWgamepadstate &state) const;        // a connected port's state
    bool nthGamepad(int index, GLFWgamepadstate &state) const;    // the index-th connected gamepad's
    bool viewButtonPressed(const GLFWgamepadstate &pad);   // the Change View key (C) or RB, going down

    // Key Bindings' new key: the keys held at the start count only once pressed again
    void startKeyCapture();
    bool capturedKey(int &key);   // a nameable key that went down since the last call

    // the menus: this frame's presses of the keyboard and every port, each port's kept for portPresses
    menu::MenuInput readMenu();
    menu::MenuInput portPresses(int port) const;   // the last readMenu's
    bool portConnected(int port) const;
    bool startPressed();   // Esc / P, or the first pad's START going down
    bool anyHeld() const;  // the pad poll 0xb4bfc: a button, the d-pad, an analog value; here also a key

  private:
    bool keyboardFree() const;   // focused and ImGui not typing
    float keyLevel(int key) const;
    float actionLevel(KeyAction action) const;   // either of its keys

    static constexpr int kPorts = GLFW_JOYSTICK_LAST + 1;
    const KeyBindings &keys_;
    std::array<bool, GLFW_KEY_LAST + 1> capturedKeysDown_{};
    GLFWwindow *window_ = nullptr;
    bool alwaysFocused_ = false;
    int testSeats_ = 0;
    std::array<argon::input::PressEdges, kPorts> portEdges_{};
    std::array<menu::MenuInput, kPorts> portPresses_{};
    bool startHeld_ = false, viewButtonHeld_ = false;
};
