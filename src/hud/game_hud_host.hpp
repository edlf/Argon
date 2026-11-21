// Argon: what the in-race HUD (hud\game_hud.hpp) asks App for: the crafts as drawn, the views' cameras, the names, the
// race start's state, the lap record's sound and the players' pads.
#pragma once
#include <string>

#include <glm/glm.hpp>

#include "render/scene_views.hpp"

class GameHudHost
{
  public:
    virtual HudCamera playerHudCamera(int player) const = 0;    // a split-screen player's view
    virtual int courseId(const std::string &folder) const = 0;
    virtual std::u16string racerName(int pilot) const = 0;
    virtual void playLapRecordSound() = 0;
    virtual bool raceHeld() const = 0;
    virtual bool gridIntroRunning() const = 0;
    virtual int introCraftSlot() const = 0;                     // the craft the grid intro shows

  protected:
    ~GameHudHost() = default;
};
