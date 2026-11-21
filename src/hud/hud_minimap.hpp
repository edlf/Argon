// Argon: the minimap element (vtable 0x1a0df8, element +0x9f8c, draw hud_minimap_draw 0x4f090): the track
// map under the player, the crafts' icons on it and, left of it, the position list with the player's place word
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "hud/hud_elements.hpp"

struct HudRacer
{
    glm::mat4 pose{1.f};   // where the craft is drawn
    int pilot = 0;
    int place = 0;         // 1 = first
    float energy = 100.f;  // weapon energy % (the position list's bar)
    uint32_t hitFlash = 0; // the red additive glow behind its portrait (craft weapon +0x98, 0x4fafe); red 0: none
    bool knockedOut = false; // out of the race: its portrait drawn red, no icon on the map
};

class HudMinimap : public HudElement
{
  public:
    void setMap(const std::string &texture, bool mapShown)
    {
        mapTexture = texture;
        showMap = mapShown;
    }
    // world units per map texel (hud\minimap_rules.hpp)
    void setWorldPerTexel(float units)
    {
        worldPerTexel = units;
    }
    // racers[0] is the player; the position list (and the map's 50 offset right of it) only with opponents;
    // placeWords: "1st" .. "6th" (race strings 607..612), the word by the player's row
    void setRace(std::vector<HudRacer> crafts, bool positionList, const std::array<std::u16string, 6> &placeWords)
    {
        racers = std::move(crafts);
        withPositionList = positionList;
        places = placeWords;
    }

    // Mirror Tracks (0x52c8bc, hud_minimap_draw 0x4f341 / 0x4f6db): the map's left and right corners swap places on
    // screen and the icons' angles turn the other way: the map drawn mirrored, as the 3D view is
    void setMirrored(bool on)
    {
        mirrored = on;
    }

  protected:
    void draw(const HudCanvas &canvas) const override;

  private:
    static constexpr float kWidth = 80.f, kHeight = 160.f, kDrop = 68.f;   // +0x214 = 68
    static constexpr uint32_t kKnockedOutTint = 0xffff2020u;              // Argon's: a knocked-out racer's portrait
    float mapOffset() const
    {
        return withPositionList ? 50.f : 0.f;
    }      // e0 (0x4f090: 50, Time Attack 0)
    void drawMap(const HudCanvas &canvas) const;
    void drawIcons(const HudCanvas &canvas) const;
    void drawPositionList(const HudCanvas &canvas) const;
    static void drawHitFlash(const HudCanvas &canvas, float x, float rowTop, uint32_t colour);

    std::string mapTexture;
    float worldPerTexel = 9.9f;
    bool showMap = true, withPositionList = true, mirrored = false;
    std::vector<HudRacer> racers;
    std::array<std::u16string, 6> places;
};
