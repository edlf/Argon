// Argon: the keyboard's craft keys (key_bindings.hpp).
#include "input/key_bindings.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>

namespace
{
struct NamedKey
{
    int key;
    const char *name;
};

// the keys a binding may name, by their US layout's label
constexpr NamedKey kKeyNames[] = {
    {GLFW_KEY_SPACE, "Space"},         {GLFW_KEY_APOSTROPHE, "'"},         {GLFW_KEY_COMMA, ","},
    {GLFW_KEY_MINUS, "Minus"},         {GLFW_KEY_PERIOD, "."},             {GLFW_KEY_SLASH, "/"},
    {GLFW_KEY_0, "0"},                 {GLFW_KEY_1, "1"},                  {GLFW_KEY_2, "2"},
    {GLFW_KEY_3, "3"},                 {GLFW_KEY_4, "4"},                  {GLFW_KEY_5, "5"},
    {GLFW_KEY_6, "6"},                 {GLFW_KEY_7, "7"},                  {GLFW_KEY_8, "8"},
    {GLFW_KEY_9, "9"},                 {GLFW_KEY_SEMICOLON, ";"},          {GLFW_KEY_EQUAL, "="},
    {GLFW_KEY_A, "A"},                 {GLFW_KEY_B, "B"},                  {GLFW_KEY_C, "C"},
    {GLFW_KEY_D, "D"},                 {GLFW_KEY_E, "E"},                  {GLFW_KEY_F, "F"},
    {GLFW_KEY_G, "G"},                 {GLFW_KEY_H, "H"},                  {GLFW_KEY_I, "I"},
    {GLFW_KEY_J, "J"},                 {GLFW_KEY_K, "K"},                  {GLFW_KEY_L, "L"},
    {GLFW_KEY_M, "M"},                 {GLFW_KEY_N, "N"},                  {GLFW_KEY_O, "O"},
    {GLFW_KEY_P, "P"},                 {GLFW_KEY_Q, "Q"},                  {GLFW_KEY_R, "R"},
    {GLFW_KEY_S, "S"},                 {GLFW_KEY_T, "T"},                  {GLFW_KEY_U, "U"},
    {GLFW_KEY_V, "V"},                 {GLFW_KEY_W, "W"},                  {GLFW_KEY_X, "X"},
    {GLFW_KEY_Y, "Y"},                 {GLFW_KEY_Z, "Z"},                  {GLFW_KEY_LEFT_BRACKET, "["},
    {GLFW_KEY_BACKSLASH, "\\"},        {GLFW_KEY_RIGHT_BRACKET, "]"},      {GLFW_KEY_GRAVE_ACCENT, "`"},
    {GLFW_KEY_ESCAPE, "Esc"},          {GLFW_KEY_ENTER, "Enter"},          {GLFW_KEY_TAB, "Tab"},
    {GLFW_KEY_BACKSPACE, "Backspace"}, {GLFW_KEY_INSERT, "Insert"},        {GLFW_KEY_DELETE, "Delete"},
    {GLFW_KEY_RIGHT, "Right"},         {GLFW_KEY_LEFT, "Left"},            {GLFW_KEY_DOWN, "Down"},
    {GLFW_KEY_UP, "Up"},               {GLFW_KEY_PAGE_UP, "Page Up"},      {GLFW_KEY_PAGE_DOWN, "Page Down"},
    {GLFW_KEY_HOME, "Home"},           {GLFW_KEY_END, "End"},              {GLFW_KEY_CAPS_LOCK, "Caps Lock"},
    {GLFW_KEY_SCROLL_LOCK, "Scroll Lock"}, {GLFW_KEY_NUM_LOCK, "Num Lock"}, {GLFW_KEY_PRINT_SCREEN, "Print Screen"},
    {GLFW_KEY_PAUSE, "Pause"},         {GLFW_KEY_F1, "F1"},                {GLFW_KEY_F2, "F2"},
    {GLFW_KEY_F3, "F3"},               {GLFW_KEY_F4, "F4"},                {GLFW_KEY_F5, "F5"},
    {GLFW_KEY_F6, "F6"},               {GLFW_KEY_F7, "F7"},                {GLFW_KEY_F8, "F8"},
    {GLFW_KEY_F9, "F9"},               {GLFW_KEY_F10, "F10"},              {GLFW_KEY_F11, "F11"},
    {GLFW_KEY_F12, "F12"},             {GLFW_KEY_KP_0, "Numpad 0"},        {GLFW_KEY_KP_1, "Numpad 1"},
    {GLFW_KEY_KP_2, "Numpad 2"},       {GLFW_KEY_KP_3, "Numpad 3"},        {GLFW_KEY_KP_4, "Numpad 4"},
    {GLFW_KEY_KP_5, "Numpad 5"},       {GLFW_KEY_KP_6, "Numpad 6"},        {GLFW_KEY_KP_7, "Numpad 7"},
    {GLFW_KEY_KP_8, "Numpad 8"},       {GLFW_KEY_KP_9, "Numpad 9"},        {GLFW_KEY_KP_DECIMAL, "Numpad ."},
    {GLFW_KEY_KP_DIVIDE, "Numpad /"},  {GLFW_KEY_KP_MULTIPLY, "Numpad *"}, {GLFW_KEY_KP_SUBTRACT, "Numpad Minus"},
    {GLFW_KEY_KP_ADD, "Numpad +"},     {GLFW_KEY_KP_ENTER, "Numpad Enter"}, {GLFW_KEY_KP_EQUAL, "Numpad ="},
    {GLFW_KEY_LEFT_SHIFT, "Left Shift"}, {GLFW_KEY_LEFT_CONTROL, "Left Ctrl"}, {GLFW_KEY_LEFT_ALT, "Left Alt"},
    {GLFW_KEY_RIGHT_SHIFT, "Right Shift"}, {GLFW_KEY_RIGHT_CONTROL, "Right Ctrl"}, {GLFW_KEY_RIGHT_ALT, "Right Alt"},
};

constexpr std::array<int, std::size(kKeyNames)> keyCodes()
{
    std::array<int, std::size(kKeyNames)> codes{};
    for (size_t index = 0; index < codes.size(); ++index)
    {
        codes[index] = kKeyNames[index].key;
    }
    return codes;
}
constexpr std::array<int, std::size(kKeyNames)> kKeyCodes = keyCodes();

constexpr int kReservedKeys[] = {GLFW_KEY_ESCAPE, GLFW_KEY_P, GLFW_KEY_F2, GLFW_KEY_F11, GLFW_KEY_F12};

constexpr const char *kActionNames[kKeyActionCount] = {"throttle", "brake",    "steerLeft", "steerRight", "airbrakeLeft", "airbrakeRight", "dive",
                                                       "climb",    "turbo",    "reset",     "fireRed",    "fireBlue",     "fireYellow",    "changeView"};
}  // namespace

const char *nameOf(KeyAction action)
{
    return kActionNames[size_t(action)];
}

std::optional<KeyAction> keyActionNamed(const std::string &name)
{
    for (int action = 0; action < kKeyActionCount; ++action)
    {
        if (name == kActionNames[action])
        {
            return KeyAction(action);
        }
    }
    return std::nullopt;
}

std::string keyName(int key)
{
    for (const NamedKey &named : kKeyNames)
    {
        if (named.key == key)
        {
            return named.name;
        }
    }
    return "-";
}

int keyNamed(const std::string &name)
{
    for (const NamedKey &named : kKeyNames)
    {
        if (name == named.name)
        {
            return named.key;
        }
    }
    return kUnboundKey;
}

std::span<const int> nameableKeys()
{
    return kKeyCodes;
}

bool isReservedKey(int key)
{
    return std::find(std::begin(kReservedKeys), std::end(kReservedKeys), key) != std::end(kReservedKeys);
}

KeyBindings::KeyBindings()
    : keys_{{{GLFW_KEY_W, GLFW_KEY_UP},
             {GLFW_KEY_S, GLFW_KEY_DOWN},
             {GLFW_KEY_A, GLFW_KEY_LEFT},
             {GLFW_KEY_D, GLFW_KEY_RIGHT},
             {GLFW_KEY_Q, kUnboundKey},
             {GLFW_KEY_E, kUnboundKey},
             {GLFW_KEY_R, kUnboundKey},
             {GLFW_KEY_F, kUnboundKey},
             {GLFW_KEY_SPACE, kUnboundKey},
             {GLFW_KEY_BACKSPACE, kUnboundKey},
             {GLFW_KEY_1, kUnboundKey},
             {GLFW_KEY_3, kUnboundKey},
             {GLFW_KEY_2, kUnboundKey},
             {GLFW_KEY_C, kUnboundKey}}}
{
}

int KeyBindings::key(KeyAction action, int slot) const
{
    return keys_[size_t(action)][size_t(slot)];
}

void KeyBindings::bind(KeyAction action, int slot, int key)
{
    for (std::array<int, kKeySlots> &slots : keys_)
    {
        std::replace(slots.begin(), slots.end(), key, kUnboundKey);
    }
    keys_[size_t(action)][size_t(slot)] = key;
}

void KeyBindings::clear(KeyAction action, int slot)
{
    keys_[size_t(action)][size_t(slot)] = kUnboundKey;
}
