// Argon: what the command line's test options ask of a run (launch_options.cpp fills it, App::tests): none of it is
// the game's; with the defaults nothing happens.
#pragma once
#include <string>
#include <vector>

struct TestScript
{
    bool autoFire = false;          // --autofire: the player fires whatever it holds
    std::string saveReplayStem;     // --save-replay <path stem>: writes each finished recording
    bool viewReplay = false;        // --view-replay: plays the race back at its end
    std::vector<int> hideMeshes;    // --hide-mesh n: track meshes not drawn
    int seats = 0;                  // --mp-seats n: ports 0..n-1 count as connected and join FE CHARSELECT
    unsigned eggs = 0;              // --egg <i>: egg i switched on whatever the profile says, bit i
    float leaveAt = -1.f;           // --finish-leave: two A presses on the race-end page at that glfwGetTime
    float knockOutAt = -1.f;        // --knockout-at: the player knocked out that many seconds into the race
    float storySceneAt = 0.f;       // --story-at t: a story scene's clock starts there
    int startPowerups[3]{};         // --powerups r b y: levels given at race start
    float specialAt = -1.f;         // --special-at: the player holds a special N s into the race
};
