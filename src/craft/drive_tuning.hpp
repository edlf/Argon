// Argon: the editor's drive settings (the control panel's Drive window, ui_extras\editor_panel.cpp): the time scale,
// the route's top speed and hover height, the AI's engine power and whether the engine sound follows the craft.
#pragma once

struct DriveTuning
{
    float timeScale = 1.f;        // the simulation's speed
    float topSpeed = 110.f;       // the route build's top speed, m/s (~400 km/h)
    float hoverHeight = 0.6f;     // the route's hover height (m), as the route build found it
    float aiPower = 90.f;         // engine power % (aivalues.txt quick race, per speed class)
    bool engineSoundFollows = true;
};
