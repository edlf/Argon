// Argon: the keyboard's craft keys as the player binds them (Argon Options' Key Bindings, settings.json's
// "keyBindings"): each action two keys, GLFW key codes, kUnboundKey where none. A key belongs to one action's slot at a
// time. The menus keep their own keys (the arrows, Enter, Space, Esc, Backspace); Esc, P and F2 / F11 / F12 cannot be
// bound (pause, the FPS meter, fullscreen, screenshots).
#pragma once
#include <array>
#include <optional>
#include <span>
#include <string>

enum class KeyAction
{
    Throttle,
    Brake,
    SteerLeft,
    SteerRight,
    AirbrakeLeft,
    AirbrakeRight,
    Dive,
    Climb,
    Turbo,
    Reset,
    FireRed,      // the pad's B
    FireBlue,     // X
    FireYellow,   // Y
    ChangeView,   // RB
};
inline constexpr int kKeyActionCount = 14;
static_assert(int(KeyAction::ChangeView) + 1 == kKeyActionCount);
inline constexpr int kKeySlots = 2;
inline constexpr int kUnboundKey = -1;   // GLFW_KEY_UNKNOWN

const char *nameOf(KeyAction action);                         // settings.json's: "throttle" .. "changeView"
std::optional<KeyAction> keyActionNamed(const std::string &name);

std::string keyName(int key);              // "W", "Space", "Up", "Numpad 5"; "-" for kUnboundKey
int keyNamed(const std::string &name);     // kUnboundKey for a name no key has
std::span<const int> nameableKeys();       // every key keyName names
bool isReservedKey(int key);               // Esc, P, F2, F11, F12

class KeyBindings
{
  public:
    KeyBindings();   // W / Up throttle, S / Down brake, A / Left, D / Right, Q / E airbrakes, R dive, F climb, Space
                     // turbo, Backspace reset, 1 / 3 / 2 fire, C view
    bool operator==(const KeyBindings &other) const = default;

    int key(KeyAction action, int slot) const;
    void bind(KeyAction action, int slot, int key);   // the key taken from any other slot first
    void clear(KeyAction action, int slot);

  private:
    std::array<std::array<int, kKeySlots>, kKeyActionCount> keys_;
};
