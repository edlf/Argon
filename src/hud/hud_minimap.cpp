// Argon: the minimap element (hud\hud_minimap.hpp; hud_minimap_draw 0x4f090)
#include "hud/hud_minimap.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "hud/hud_canvas.hpp"

namespace
{
constexpr float kD2R = 3.14159265f / 180.f;
// the map's UV extent (0.125, 0.125 x 228 / 96)
constexpr float kSu = 0.125f, kSv = 0.125f * 228.f / 96.f, kHu = kSu * 0.5f, kHv = kSv * 0.5f;

// the map's rotation: the player's heading points up; world = 1 / the world units per texel (0x4f215)
struct MapFrame
{
    float a0, f9, heading, uc, vc;
    MapFrame(const glm::mat4 &player, float world)
    {
        const glm::vec3 p(player[3]), f(player[2]);
        a0 = -std::atan2(kSu, kSv) / kD2R;                // fVar1 (degrees)
        f9 = 90.f - a0;
        heading = 180.f + std::atan2(f.x, f.z) / kD2R;    // +0x210
        uc = (world * p.x + 256.f) / 512.f;
        vc = (world * p.z + 256.f) / 512.f;
    }
    glm::vec2 uv(float th) const
    {
        const float cosine = std::cos(th * kD2R), sine = std::sin(th * kD2R);
        return glm::vec2(cosine * kHv + kHu * sine + uc, cosine * kHu + vc - sine * kHv);
    }
};

ImVec2 imVec(glm::vec2 v)
{
    return ImVec2(v.x, v.y);
}

// world x / z -> the map texture's uv (512 texels, the world's origin at its centre) -> the screen through the inverse
// of the map's own screen -> uv mapping (corners 0 / 1 / 2)
struct MapToScreen
{
    float worldPerUnit;
    glm::vec2 uv0;
    glm::mat2 uvToCorners;
    glm::vec2 corner0, alongX, alongY;

    glm::vec2 at(const glm::vec3 &world) const
    {
        const glm::vec2 uv((worldPerUnit * world.x + 256.f) / 512.f, (worldPerUnit * world.z + 256.f) / 512.f);
        const glm::vec2 corners = uvToCorners * (uv - uv0);
        return corner0 + alongX * corners.x + alongY * corners.y;
    }
};

// an icon's corner at `degrees` around its centre (12 px out), turned by its heading
ImVec2 iconCorner(const HudCanvas &canvas, glm::vec2 centre, float heading, float degrees)
{
    const float angle = heading + degrees * kD2R;
    return canvas.at(centre.x + std::sin(angle) * 12.f, centre.y + std::cos(angle) * 12.f);
}
}  // namespace

void HudMinimap::draw(const HudCanvas &canvas) const
{
    if (racers.empty())
    {
        return;
    }
    drawMap(canvas);
    drawIcons(canvas);
    drawPositionList(canvas);
}

// the track texture under the ship, rotated so the ship points up, faded towards the bottom
void HudMinimap::drawMap(const HudCanvas &canvas) const
{
    const Found<const HudTexture> mt = showMap ? canvas.texture(mapTexture) : std::nullopt;
    if (!mt)
    {
        return;
    }
    const MapFrame m(racers[0].pose, 1.f / worldPerTexel);
    const float w = kWidth, h = kHeight;
    const float cx = posX + w * 0.5f + mapOffset(), top = posY + h - h * 0.5f - h * 0.5f + kDrop, bot = posY + h * 0.5f + h * 0.5f + kDrop;
    const float cy = posY + h * 0.5f + kDrop;
    const float half = mirrored ? -w * 0.5f : w * 0.5f;
    const ImVec2 pv[5] = {canvas.at(cx - half, top), canvas.at(cx + half, top), canvas.at(cx - half, bot), canvas.at(cx + half, bot), canvas.at(cx, cy)};
    const ImVec2 tv[5] = {imVec(m.uv(m.heading - m.a0 + m.f9)),
                          imVec(m.uv(m.a0 + m.heading + m.f9)),
                          imVec(m.uv(180.f + m.a0 + m.heading + m.f9)),
                          imVec(m.uv(180.f - m.a0 + m.heading + m.f9)),
                          ImVec2(m.uc, m.vc)};
    const ImU32 cv[5] = {imColour(0xb0ffffff), imColour(0xb0ffffff), imColour(0x00ffffff), imColour(0x00ffffff), imColour(0xffffffff)};
    const int idx[9] = {0, 1, 2, 2, 1, 3, 0, 1, 4};
    ImDrawList *drawList = canvas.drawList();
    drawList->PushTextureID(mt->image);
    drawList->PrimReserve(9, 9);
    for (int racer : idx)
    {
        drawList->PrimVtx(pv[racer], tv[racer], cv[racer]);
    }
    drawList->PopTextureID();
}

// data\hud\arrow.dds, 12 px (hud_draw_icon 0xcbc40: corners at angle + 225 / 135 / 315 / 45 deg, 12 from the centre),
// white for the player, yellow for the others, orange for pilot +-8 of the player's; only inside the map minus 12 px.
// Placed through the map's own screen <-> UV mapping.
void HudMinimap::drawIcons(const HudCanvas &canvas) const
{
    const Found<const HudTexture> at = showMap ? canvas.texture("arrow") : std::nullopt;
    if (!at)
    {
        return;
    }
    const float world = 1.f / worldPerTexel;
    const MapFrame m(racers[0].pose, world);
    const float e0 = mapOffset(), w = kWidth, h = kHeight;
    const glm::vec2 t0 = m.uv(m.heading - m.a0 + m.f9), t1 = m.uv(m.a0 + m.heading + m.f9), t2 = m.uv(180.f + m.a0 + m.heading + m.f9);
    const glm::vec2 s0(posX + e0 + (mirrored ? w : 0.f), posY + kDrop), sx(mirrored ? -w : w, 0.f), sy(0.f, h);   // corners 0 / 1 / 2
    const glm::mat2 uvAxes(t1 - t0, t2 - t0);
    if (!(std::abs(glm::determinant(uvAxes)) > 1e-12f))
    {
        return;
    }
    const MapToScreen toScreen{world, t0, glm::inverse(uvAxes), s0, sx, sy};
    const int playerPilot = racers[0].pilot;
    for (int i = int(racers.size()) - 1; i >= 0; --i)
    {
        if (racers[size_t(i)].knockedOut)
        {
            continue;   // Argon's: a knocked-out craft sits in the cage, off the track
        }
        const glm::mat4 &pose = racers[size_t(i)].pose;
        const glm::vec3 p(pose[3]), f(pose[2]);
        const glm::vec2 ctr = toScreen.at(p), d = toScreen.at(p + f * 20.f) - ctr;
        if (!(ctr.x - 12.f > posX + e0 && ctr.x + 12.f < posX + e0 + w && ctr.y - 12.f > posY + kDrop && ctr.y + 12.f < posY + kDrop + h))
        {
            continue;
        }
        // arrow.dds is a "V": its point (texture bottom) shows the heading, as the game's angle = heading - (180 + heading)
        const float th = glm::length(d) > 1e-4f ? std::atan2(d.x, d.y) : 0.f;
        uint32_t colour = 0xffffff00u;
        if (i == 0)
        {
            colour = 0xffffffffu;
        }
        const int pp = racers[size_t(i)].pilot;
        if (i != 0 && (pp == playerPilot + 8 || pp == playerPilot - 8))
        {
            colour = 0xffff8000u;
        }
        canvas.drawList()->AddImageQuad(at->image,
                                   iconCorner(canvas, ctr, th, 225.f),
                                   iconCorner(canvas, ctr, th, 135.f),
                                   iconCorner(canvas, ctr, th, 45.f),
                                   iconCorner(canvas, ctr, th, 315.f),
                                   ImVec2(0, 0),
                                   ImVec2(1, 0),
                                   ImVec2(1, 1),
                                   ImVec2(0, 1),
                                   imColour(colour));
    }
}

// 0x4fafe: a hit's red over the row's portrait area (x+7, r)-(x+43, r+35), additive (blend mode 2), drawn twice, under
// the portrait
void HudMinimap::drawHitFlash(const HudCanvas &canvas, float x, float rowTop, uint32_t colour)
{
    if ((colour & 0x00ffffffu) == 0)
    {
        return;
    }
    canvas.additiveBlend();
    for (int pass = 0; pass < 2; ++pass)
    {
        canvas.rect(x + 7.f, rowTop, x + 43.f, rowTop + 35.f, colour);
    }
    canvas.normalBlend();
}

// one 38-high row per craft in place order, bottom aligned with the map (rows end at B - 4), left of it: frame,
// panel, 36 x 36 portrait (data\hud\tex<table[pilot]>.dds), energy bar; the player's row is highlighted and joined to
// its place word (strings 607..612) above the map; none in Time Attack (0x4f090)
void HudMinimap::drawPositionList(const HudCanvas &canvas) const
{
    const int size = int(racers.size());
    if (size <= 1 || !withPositionList)
    {
        return;
    }
    static const int kPortrait[16] = {12, 14, 7, 2, 4, 11, 9, 6, 1, 13, 0, 15, 5, 3, 10, 8};   // 0x1b93f8
    const float left = posX, e0 = mapOffset();
    for (int row = 0; row < size; ++row)
    {
        int who = 0;
        for (int i = 0; i < size; ++i)
        {
            if (racers[size_t(i)].place == row + 1)
            {
                who = i;
            }
        }
        const bool me = who == 0;
        const float rowY = posY + kHeight + kDrop - float(size) * 38.f + float(row) * 38.f;
        canvas.rect(left - 3.f, rowY - 2.f, left + 43.f, rowY + 39.f, 0xff000000u);
        canvas.rect(left - 2.f, rowY - 1.f, left + 42.f, rowY + 37.f, me ? 0xffffff00u : 0xff808080u);
        canvas.rect(left + 9.f, rowY, left + 40.f, rowY + 35.f, me ? 0xffb07000u : 0xff303030u);
        drawHitFlash(canvas, left, rowY, racers[size_t(who)].hitFlash);
        char key[16];
        std::snprintf(key, sizeof key, "pilot%02d", kPortrait[std::clamp(racers[size_t(who)].pilot, 0, 15)]);
        if (const Found<const HudTexture> pt = canvas.texture(key))
        {
            const uint32_t tint = racers[size_t(who)].knockedOut ? kKnockedOutTint : 0xffffffffu;
            canvas.image(*pt, HudQuad{left + 7.f, rowY, left + 43.f, rowY + 36.f}, tint);
        }
        if (me)
        {
            const float f3 = posY + kDrop - 20.f;
            canvas.text({posX + e0 + 6.f, f3 - 16.f}, places[size_t(std::min(row, 5))], {1, 0xffffffffu, 0});
            canvas.rect(left + 39.f, rowY + 16.f, left + e0 - 0.5f, rowY + 19.f, 0xff000000u);
            canvas.rect(left + e0 - 4.f, f3, left + e0 - 0.5f, rowY + 18.f, 0xff000000u);
            canvas.rect(left + e0 - 4.f, f3 - 1.f, left + e0 + 7.f, f3 + 2.f, 0xff000000u);
            canvas.rect(left + 39.f, rowY + 17.f, left + e0 - 1.5f, rowY + 18.f, 0xffffff00u);
            canvas.rect(left + e0 - 3.f, f3, left + e0 - 1.5f, rowY + 18.f, 0xffffff00u);
            canvas.rect(left + e0 - 3.f, f3, left + e0 + 7.f, f3 + 1.f, 0xffffff00u);
        }
        // energy bar (craft weapon +0: the viewer's crafts have no weapons / damage, so always full)
        const float energy = std::clamp(racers[size_t(who)].energy * 0.01f, 0.f, 1.f) * 32.f;
        canvas.rect(left - 1.f, rowY + 1.f, left + 6.f, rowY + 36.f, 0xff303030u);
        canvas.rect(left + 0.5f, rowY + 2.f, left + 5.f, 34.f - energy + rowY, 0xffff0000u);
        canvas.rect(left + 0.5f, 34.f - energy + rowY, left + 5.f, rowY + 34.f, 0xff80ff20u);
    }
}
