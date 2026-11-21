// Argon: a craft's controller rumble as the game computes it, without the race around it (docs\physics\rumble.md "Rumble"):
// the motor words of craft_rumble 0x5b610, the knocked-out fade of player_pad_input 0x70ac0 and the pad's motor
// thresholds 0xb46d0. GameCraft::rumble and App::updateVibration apply them; tests\rumble_rules_tests.cpp.
#pragma once
#include <cstdint>

namespace argon::rumble
{

// the pad's two motors as XInputSetState gets them (0xb4530: pad +0x194 -> XINPUT_FEEDBACK Rumble.wLeftMotorSpeed,
// +0x196 -> wRightMotorSpeed): first = the left, low-frequency motor, second = the right, high-frequency one
struct Motors
{
    uint16_t first = 0;
    uint16_t second = 0;
};

// fistp to a 64-bit integer, then its low 16 bits (the game's motor words wrap, they are not clamped)
uint16_t motorWord(float value);

// x (1 - rate x dt): +0x9dc (rate 4), +0x8b0 (2), +0x1138 (10), +0x1134 (5)
float decayed(float value, float rate, float dt);

// 0x5b927: the knock +0x8b0 x 1000, 0..65000, added to the second motor
float knockMotor(float knock);

// 0x5b9a3: past 2.2 of hover compression (+0x890, the four rays' depths summed), the second motor word gains
// (compression - 1.5) x speed x 25 (16-bit arithmetic)
uint16_t withCompression(uint16_t secondWord, float compression, float speed);

// player_pad_input 0x70ac0: a knocked-out craft (+0x1145, out since +0x1148) runs both motors at
// round((3 - t) x 33) x 300 for 3 s, then 0
Motors knockedOutFade(float secondsOut);

// 0xb46d0: the first motor runs only above 0x4000, the second above 0x2000 (else 0)
Motors padMotors(Motors requested);

// what player_pad_input sends for one player's pad: nothing while the port's vibration flag is set (Controller n
// Vibration Off, or the pause: 0x52c8c0 + port), the fade once knocked out, the craft's motors while flying it
struct PadState
{
    bool vibrationOn = true;
    bool flying = false;        // the player's pad drives the craft
    bool knockedOut = false;
    float secondsOut = 0.f;
};
Motors playerMotors(const PadState &pad, Motors craft);

}  // namespace argon::rumble
