// Argon: the speedometer's second (throttle) needle, gamehud_render 0x4ad71..0x4ae5e. Notes: docs\physics\README.md (HUD,
// "Needles").
#pragma once

namespace argon::hud
{

// two draws of the CRT rand() (0xf96ea) masked to 0..255, taken each time the HUD is drawn
struct NeedleJitterDraws
{
    int raise = 0;   // 0x4ad84: adds draw / 255 x gauge x 0.1
    int lower = 0;   // 0x4adb3: takes draw / 255 x gauge x 0.1 off
};

// the needle's target angle in degrees for huddata +0x18 (the throttle gauge): gauge x 1.441 plus the jitter
// (raise - lower) / 255 x gauge x 0.1, clamped to 0..144.1 (the dial's end); the HUD then moves its shown angle
// (+0x8940) 0.2 of the way there per frame
float secondNeedleTarget(float throttleGauge, const NeedleJitterDraws &draws);

}  // namespace argon::hud
