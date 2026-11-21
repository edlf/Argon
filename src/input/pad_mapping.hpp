// Argon: what a gamepad's state means, without reading any device (input\player_input.hpp reads them). For a craft
// the game's pad handler (0x70ac0): RT throttle, the left stick or d-pad steer (past 12288 / 32768, rescaled), stick
// forward / back or d-pad up / down dive / climb, LT the airbrake on the side steered towards, A turbo, BACK reset;
// Mirror Tracks swaps left and right (player_pad_input 0x71306). For the menus: the d-pad or stick, A or START, B, X, Y,
// counted on their press.
#pragma once
#include "craft/craftsim.hpp"
#include "ui/menu_common.hpp"

#include <GLFW/glfw3.h>

namespace argon::input
{

void addGamepad(GameCraft::Pad &pad, const GLFWgamepadstate &state);
void mirrorSteering(GameCraft::Pad &pad);

menu::MenuInput menuButtonsHeld(const GLFWgamepadstate &state);
void mergePresses(menu::MenuInput &into, const menu::MenuInput &more);
bool anyButtonHeld(const GLFWgamepadstate &state);   // a button, or a trigger or stick off its rest

// the buttons that went down since the last step
class PressEdges
{
  public:
    menu::MenuInput step(const menu::MenuInput &held);

  private:
    menu::MenuInput previous_;
};

}  // namespace argon::input
