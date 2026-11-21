// Argon: the HUD's text, sprite, power-up panel and points popup elements (hud\hud_elements.hpp)
#include "hud/hud_elements.hpp"

#include <algorithm>
#include <cmath>

#include "hud/hud_canvas.hpp"

void HudText::draw(const HudCanvas &canvas) const
{
    canvas.text({posX, posY}, text, {font, colour, flags, scaleX, scaleY});
}

void HudSprite::draw(const HudCanvas &canvas) const
{
    if (const Found<const HudTexture> found = canvas.texture(texture))
    {
        canvas.image(*found, posX, posY, posX + width, posY + height);
    }
}

void HudPowerUpPanel::draw(const HudCanvas &canvas) const
{
    layer(canvas, "weaponbase", 0xffffffffu);
    canvas.additiveBlend();
    levelLayers(canvas, weapons.caps, 0xff404040u);
    if (!weapons.specialReady)
    {
        levelLayers(canvas, weapons.held, heldColour());
    }
    canvas.normalBlend();
    if (weapons.specialReady)
    {
        layer(canvas, "weaponspecial", 0xffffffffu);
    }
}

void HudPowerUpPanel::layer(const HudCanvas &canvas, const std::string &texture, uint32_t argb) const
{
    if (const Found<const HudTexture> found = canvas.texture(texture))
    {
        canvas.image(*found, HudQuad{posX, posY, posX + 128.f * scale, posY + 64.f * scale}, argb);
    }
}

void HudPowerUpPanel::levelLayers(const HudCanvas &canvas, const std::array<int, 3> &levels, uint32_t argb) const
{
    static const char *const kColour[3] = {"red", "blue", "yellow"};
    for (size_t i = 0; i < 3; ++i)
    {
        if (levels[i] > 0)
        {
            layer(canvas, std::string("weapon") + kColour[i] + char('0' + std::min(levels[i], 3)), argb);
        }
    }
}

// 0x4eef9: fistp rounds, the unsigned compare keeps 55..255
uint32_t HudPowerUpPanel::heldColour() const
{
    const long grey = std::min(std::lround(std::sin(clock * 15.f) * 100.f + 155.f), 255L);
    return 0xff000000u | uint32_t(grey) * 0x10101u;
}

void HudStaticOverlay::draw(const HudCanvas &canvas) const
{
    if (const Found<const HudTexture> found = canvas.texture("static"))
    {
        canvas.image(*found, HudQuad{posX, posY, posX + width, posY + height, ImVec2(offsetU, offsetV), ImVec2(offsetU + 1.f, offsetV + 1.f)}, argb);
    }
}

namespace
{
// a popup's age 0..1 as its alpha byte: opaque when new, clear at 1
uint32_t fadedAlpha(float age)
{
    const int faded = int(std::lround(age * 255.f));
    return uint32_t(255 - std::clamp(faded, 0, 255)) << 24;
}
}  // namespace

void HudPointsPopup::draw(const HudCanvas &canvas) const
{
    const float scale = 0.5625f, x = posX, y = posY;
    const uint32_t a1 = fadedAlpha(alpha), a2 = fadedAlpha(((alpha - 0.5f) * 2.f - 0.5f) * 2.f);
    if (alpha > 0.5f)
    {
        canvas.text({x, y - 75.f}, points, {0, a2 | 0xffff00u, 2, 3.5f * scale, 3.f * scale});
        canvas.text({x, y + 30.f * scale - 25.f}, label, {0, a2 | 0x00ff00u, 2, 1.75f * scale, 1.5f * scale});
    }
    canvas.text({x, y - alpha * 150.f}, points, {0, a1 | 0xffff00u, 2, 7.f * alpha * scale, 6.f * alpha * scale});
    canvas.text({x, y + 30.f * scale - 50.f * alpha}, label, {0, a1 | 0x00ff00u, 2, 3.5f * alpha * scale, 3.f * alpha * scale});
}
