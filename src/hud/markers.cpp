// Argon: the HUD's markers over the race (0x4d9d0): the nemesis marker and the weapon crosshairs
// (docs\weapons\README.md), drawn by GameHud after its elements
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "app/weapons.hpp"
#include "files/string_table.hpp"
#include "hud/game_hud.hpp"
#include "hud/hud_canvas.hpp"
#include "logic/race.hpp"
#include "logic/race_start.hpp"
#include "ui/game_strings.hpp"

namespace
{
constexpr float kDegrees = 3.14159265f / 180.f;

struct Brackets
{
    float left, top, right, bottom, arm;   // the box and each bracket's arm length
};

// four corner brackets, each two arms along the box's edges
void drawBrackets(const HudCanvas &canvas, const Brackets &box, ImU32 colour)
{
    const float width = canvas.pixelsPerUnit();
    for (const float x : {box.left, box.right})
    {
        const float inward = x == box.left ? box.arm : -box.arm;
        for (const float y : {box.top, box.bottom})
        {
            const float down = y == box.top ? box.arm : -box.arm;
            canvas.drawList()->AddLine(canvas.at(x, y), canvas.at(x + inward, y), colour, width);
            canvas.drawList()->AddLine(canvas.at(x, y), canvas.at(x, y + down), colour, width);
        }
    }
}

struct SpunIcon
{
    float x, y, angle, radius;   // the centre, its turn (radians), the corners' distance from it
};

ImVec2 iconCorner(const HudCanvas &canvas, const SpunIcon &icon, float degrees)
{
    const float angle = icon.angle + degrees * kDegrees;
    return canvas.at(icon.x + std::sin(angle) * icon.radius, icon.y + std::cos(angle) * icon.radius);
}
}  // namespace

int GameHud::pilotInSlot(int slot) const
{
    if (slot == 0)
    {
        return sources.playerCraft.pilot();
    }
    for (const auto &drone : sources.race.grid())
    {
        if (drone.slot == slot)
        {
            return drone.craft.pilot();
        }
    }
    return -1;
}

int GameHud::nemesisSlot(int nemesis) const
{
    for (const auto &drone : sources.race.grid())
    {
        if (drone.craft.pilot() == nemesis)
        {
            return drone.slot;
        }
    }
    return -1;
}

// 0x4d9d0, first part: the player's nemesis (pilot id +- 8) when on screen: yellow corner brackets around it,
// s = clamp(2048 / depth x 0.3, 8, 64), box x +- 1.5 s, y +- s, arms 0.25 s; its name (font 0, centred) at
// y - (s + 18). Both fade with the camera distance: alpha = 255 - clamp(distance - 250, 0, 255). During the race
// start (0x52af9c) the game puts the marker on the craft the grid intro shows; the viewer shows only the intro's
// labels there (0x4de42: portrait and name), without the brackets and the small name.
// Not ported: 0x52af9c also covers the countdown, where the marker follows the intro's clock 0x50e458 on.
void GameHud::drawNemesisMarker(const HudCanvas &canvas, const MarkerFrame &frame) const
{
    const int me = craft().pilot();
    if (me < 0)
    {
        return;
    }
    const int nem = me < 8 ? me + 8 : me - 8;
    const bool intro = host.gridIntroRunning();
    const int slot = intro ? host.introCraftSlot() : nemesisSlot(nem);
    if (slot < 0)
    {
        return;
    }
    if (intro)
    {
        drawIntroLabelsOver(canvas, frame, slot, nem);
        return;
    }
    const glm::vec3 position = glm::vec3(sources.poses.slotMatrix(slot)[3]);   // where the ship is drawn (between physics steps)
    const glm::vec4 cl = frame.viewProj * glm::vec4(position, 1.f);
    if (cl.w <= 0.f)
    {
        return;
    }
    const glm::vec2 at = frame.toHud(cl);
    const float x = at.x, y = at.y;
    const float size = std::clamp(2048.f / cl.w * 0.3f, 8.f, 64.f);
    const float fade = std::clamp(glm::length(position - frame.eye) - 250.f, 0.f, 255.f);
    const uint32_t alpha = 255u - uint32_t(fade);
    if (alpha == 0)
    {
        return;
    }
    const int pilot = pilotInSlot(slot);   // 0x4dca1: the marked craft's character
    if (pilot >= 0)
    {
        canvas.text({x, y - (size + 18.f)}, host.racerName(pilot), {0, (alpha << 24) | 0xffffffu, 2});
    }
    const ImU32 colour = imColour((alpha << 24) | 0xffff00u);
    const float x0 = x - size * 1.5f, x1 = x + size * 1.5f, y0 = y - size, y1 = y + size, arm = size * 0.25f;
    drawBrackets(canvas, Brackets{x0, y0, x1, y1, arm}, colour);
}

// the intro's labels over the craft in this slot when it is in front of the camera
void GameHud::drawIntroLabelsOver(const HudCanvas &canvas, const MarkerFrame &frame, int slot, int nemesis) const
{
    const glm::vec4 cl = frame.viewProj * glm::vec4(glm::vec3(sources.poses.slotMatrix(slot)[3]), 1.f);
    if (cl.w <= 0.f)
    {
        return;
    }
    const glm::vec2 at = frame.toHud(cl);
    const int pilot = pilotInSlot(slot);
    drawIntroLabels(canvas, at.x, at.y, pilot, pilot == nemesis);
}

// 0x4de42, during the grid intro (0x52af9d) once half a second of the craft's 2 s shot has passed: "[FCHAR<id>] <name>"
// (its portrait, 64 x 64, and the name; font 1, centred, white, x 1.5) 104 above the craft; when it is the player's
// nemesis, "Nemesis" (race string 0x421, font 1, centred, yellow, x 1.5) 24 below, blinking (on 3 of each 4 eighths of
// a second)
void GameHud::drawIntroLabels(const HudCanvas &canvas, float x, float y, int pilot, bool nemesis) const
{
    const float clock = sources.raceStart.introClock();
    if (!(std::fmod(clock, 2.f) > 0.5f) || pilot < 0)
    {
        return;
    }
    char tag[16];
    std::snprintf(tag, sizeof tag, "[FCHAR%d] ", pilot);
    canvas.text({x, y - 104.f}, argon::widenLatin1(tag) + host.racerName(pilot), {1, 0xffffffffu, 0x1002, 1.5f, 1.5f});
    if (nemesis && (int(clock * 8.f) & 3) != 0)
    {
        canvas.text({x, y + 24.f}, sources.strings.race(0x421, "Nemesis"), {1, 0xffffff00u, 2, 1.5f, 1.5f});
    }
}

// 0x4d9d0: each crosshair at its target's screen position, size clamp(2048 / depth, 16, 64); appearing (state 1, 0.5 s):
// a = 0.5 - age, scale 10 a + 1, alpha 255 - 510 a; released (3, 0.25 s): a = age, scale 10 a + 1, alpha 255 - 1020 a;
// hud_draw_icon(x, y, time x -360 deg, size x sqrt 2), additive.
void GameHud::drawCrosshairs(const HudCanvas &canvas, const MarkerFrame &frame) const
{
    const Weapons::PlayerLocks &locks = sources.weapons.locks(size_t(player));
    static const char *const kTexture[3] = {"crosshair", "crosshair_blue", "crosshair_yellow"};
    const float ang = sources.simClock * -360.f * kDegrees;
    bool any = false;
    for (const auto &list : locks.crosshairs())
    {
        for (const auto &crosshair : list)
        {
            any = any || crosshair.state != 0;
        }
    }
    if (!any)
    {
        return;
    }
    canvas.additiveBlend();
    for (int li = 0; li < 3; ++li)
    {
        const Found<const HudTexture> texture = canvas.texture(kTexture[li]);
        if (!texture)
        {
            continue;
        }
        for (const auto &crosshair : locks.crosshairs()[size_t(li)])
        {
            if (crosshair.state == 0)
            {
                continue;
            }
            const glm::vec3 position = glm::vec3(sources.poses.slotMatrix(crosshair.target)[3]);   // where the ship is drawn
            const glm::vec4 cl = frame.viewProj * glm::vec4(position, 1.f);
            if (cl.w <= 0.1f)
            {
                continue;
            }
            const glm::vec2 at = frame.toHud(cl);
            const float sx = at.x, sy = at.y;
            float size = std::clamp(2048.f / cl.w, 16.f, 64.f), scale = 1.f, alpha = 255.f;
            const float age = sources.simClock - crosshair.stateSince;
            if (crosshair.state == 1)
            {
                float grow = 0.5f - age;
                scale = 10.f * grow + 1.f;
                alpha = 255.f - 510.f * grow;
            }
            if (crosshair.state == 3)
            {
                float grow = age;
                scale = 10.f * grow + 1.f;
                alpha = 255.f - 1020.f * grow;
            }
            size *= std::max(scale, 0.f);
            // additive: the colour scaled by its alpha
            const uint32_t grey = uint32_t(255 * (float(uint32_t(std::clamp(alpha, 0.f, 255.f))) / 255.f));
            const uint32_t colour = 0xff000000u | (grey << 16) | (grey << 8) | grey;
            const float radius = size * 1.41421356f;
            const SpunIcon icon{sx, sy, ang, radius};
            canvas.drawList()->AddImageQuad(texture->image,
                                       iconCorner(canvas, icon, 225.f),
                                       iconCorner(canvas, icon, 135.f),
                                       iconCorner(canvas, icon, 45.f),
                                       iconCorner(canvas, icon, 315.f),
                                       ImVec2(0, 0),
                                       ImVec2(1, 0),
                                       ImVec2(1, 1),
                                       ImVec2(0, 1),
                                       imColour(colour));
        }
    }
    canvas.normalBlend();
}
