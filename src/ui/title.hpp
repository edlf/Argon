// Argon: the title screen FE TITLE (vtable 0x19dba0), the page the viewer opens on (ui\title.cpp)
#pragma once
#include "ui/fe_canvas.hpp"
#include "ui/page_hosts.hpp"
#include "ui/fe_page.hpp"

namespace menu::fe
{
struct TitleText;
}

class TitlePage : public FePage
{
  public:
    struct ModelClocks
    {
        float logo = 0, dodecahedron = 0;
    };   // the animation times of frontend\titlelogo / dodechstart

    TitlePage(const PageKit &kit, MenuDoor &door) : FePage(kit), door(door)
    {
    }
    void open();

    void input(const menu::MenuInput &in) override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kTitle;
    }
    void draw(const FeCanvas &canvas) override;
    bool bare() const override
    {
        return true;
    }
    FeScene scene() override;   // list +0x6958, camera 1

  private:
    MenuDoor &door;   // START: FE MAIN
    // the shell's page state (current_sequence)
    enum class Sequence
    {
        Intro = 1,
        WaitForStart = 5,
        Leaving = 7,
        LogoIn = 8
    };
    void advance(double now);
    void startSequence(Sequence next, double now);
    ModelClocks modelClocks() const;
    void leave();
    void drawStartPrompt(const FeCanvas &canvas);
    void drawText(const FeCanvas &canvas, const menu::fe::TitleText &text, uint32_t colour);
    void drawLine(const FeCanvas &canvas, float y, uint32_t colour, const std::u16string &line);

    Sequence sequence = Sequence::Intro;
    double enterTime = 0, sequenceStart = 0, logoShown = 0, startPressed = 0;
};
