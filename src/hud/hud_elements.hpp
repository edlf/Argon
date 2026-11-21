// Argon: the HUD's elements (gamehud.cpp; docs\physics\hud.md "HUD"): the game's element vtables share slots
// +0x2c draw, +0x40 pos, +0x44 size, +0x48 scale, +0x4c visible; group 0x19aec0, text 0x19afc0, sprite 0x19b0d0,
// minimap 0x1a0df8 (hud\hud_minimap.hpp). CGameHud (hud\game_hud.hpp) owns them.
#pragma once
#include <array>
#include <cstdint>
#include <string>

class HudCanvas;

class HudElement
{
  public:
    virtual ~HudElement() = default;
    void place(float x, float y)
    {
        posX = x;
        posY = y;
    }   // +0x40
    void show(bool on)
    {
        shown = on;
    }                      // +0x4c
    void drawIfShown(const HudCanvas &canvas) const
    {
        if (shown)
        {
            draw(canvas);
        }
    }

  protected:
    virtual void draw(const HudCanvas &c) const = 0;       // +0x2c
    float posX = 0, posY = 0;

  private:
    bool shown = true;
};

// text (vtable 0x19afc0): colour +0x1a0, font +0x1d4 (0 = 18 px, 1 = 34 px), flags +0x204 (&0xf 0 left / 1 right /
// 2 centre, &0xf000 per line), the string set by +0x64
class HudText : public HudElement
{
  public:
    HudText(int font, int flags, uint32_t colour) : font(font), flags(flags), colour(colour)
    {
    }
    void setText(const std::u16string &content)
    {
        text = content;
    }    // +0x64
    void setScale(float x, float y)
    {
        scaleX = x;
        scaleY = y;
    }   // +0x48

  protected:
    void draw(const HudCanvas &canvas) const override;

  private:
    int font, flags;
    uint32_t colour;
    float scaleX = 1.f, scaleY = 1.f;
    std::u16string text;
};

// sprite (vtable 0x19b0d0): a data\hud texture at its position, its size
class HudSprite : public HudElement
{
  public:
    HudSprite(std::string texture, float w, float h) : texture(std::move(texture)), width(w), height(h)
    {
    }

  protected:
    void draw(const HudCanvas &canvas) const override;

  private:
    std::string texture;
    float width, height;
};

// the power-up panel (element +0x3950, draw 0x4eb20): 128 x 64, weaponbase (blend mode 1); then additive (mode 2) per
// colour its cap, the most it can hold this race (0x52cd0c / 12 / 18), as weapon<colour><cap> in 0xff404040, and the
// held level (+0x74 / +0x78 / +0x7c) as weapon<colour><level> pulsing grey g = min(155 + 100 sin(15 t), 255), t the
// clock in seconds; a ready special (+0x8c) shows weaponspecial (mode 1, white) instead of the held levels
class HudPowerUpPanel : public HudElement
{
  public:
    struct Weapons
    {
        std::array<int, 3> held{}, caps{};   // red / blue / yellow
        bool specialReady = false;
    };
    void setWeapons(const Weapons &held)
    {
        weapons = held;
    }
    void setClock(float seconds)
    {
        clock = seconds;
    }
    void setScale(float value)
    {
        scale = value;
    }   // split screen: 0.5 (0x4eb20)

  protected:
    void draw(const HudCanvas &canvas) const override;

  private:
    void layer(const HudCanvas &c, const std::string &texture, uint32_t argb) const;
    void levelLayers(const HudCanvas &canvas, const std::array<int, 3> &levels, uint32_t argb) const;
    uint32_t heldColour() const;

    Weapons weapons;
    float clock = 0, scale = 1.f;
};

// the RESET overlay (sprite +0xa1ac: data\hud\static.dds over the whole screen, gamehud_setup 0x46040): colour
// 0x00ffffff + alpha (+0xa34c), uv from the offset (+0x1fc / +0x200) over an extent of 1 (draw 0xab9b0)
class HudStaticOverlay : public HudElement
{
  public:
    void setSize(float w, float h)
    {
        width = w;
        height = h;
    }
    void setFrame(uint32_t alpha, float u, float v)
    {
        argb = alpha << 24 | 0xffffffu;
        offsetU = u;
        offsetV = v;
    }

  protected:
    void draw(const HudCanvas &canvas) const override;

  private:
    float width = 640.f, height = 480.f, offsetU = 0, offsetV = 0;
    uint32_t argb = 0x00ffffff;
};

// the points popup (element +0x9380 at (centre, T + 120), scale 0.5625; update 0x4cc30). The first queued entry
// shows for 0.8 s, a = age / 0.8: points "%d" (blank for 0) yellow at y - 150 a, scale (7, 6) x a x s; label green at
// y + 30 s - 50 a, scale (3.5, 3) x a x s, both alpha 255 (1 - a); after a = 0.5 a second copy: points at y - 75
// scale (3.5, 3) s, label at y + 30 s - 25 scale (1.75, 1.5) s, alpha fading over a 0.75..1. All centred, font 0.
class HudPointsPopup : public HudElement
{
  public:
    void setEntry(const std::u16string &pointsText, const std::u16string &labelText, float age)
    {
        points = pointsText;
        label = labelText;
        alpha = age;
    }

  protected:
    void draw(const HudCanvas &canvas) const override;

  private:
    std::u16string points, label;
    float alpha = 0;
};
