// Argon: FE OPTIONS CREDITS (vtable 0x19e1c8), the credits player (ui\credits.cpp)
#pragma once
#include "ui/fe_canvas.hpp"
#include <string>
#include <vector>

#include "ui/fe_page.hpp"

class CreditsPage : public FePage
{
  public:
    CreditsPage(const PageKit &kit, const std::string &gameFolder) : FePage(kit), gameFolder(gameFolder)
    {
    }
    void open();

    void input(const menu::MenuInput &in) override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kCredits;
    }
    void draw(const FeCanvas &canvas) override;
    void step() override;
    bool bare() const override
    {
        return true;
    }
    bool overBlack() const override
    {
        return true;
    }

  private:
    const std::string &gameFolder;   // the pictures: <game>\data\charbitmaps
    struct Segment
    {
        std::u16string text;
        uint32_t colour;
        float scale;
    };
    struct Line
    {
        std::vector<Segment> segments;
        float width = 0, height = 0;
    };
    struct Style
    {
        uint32_t colour = 0xffffffffu;
        float scale = 1.f;
    };   // the tags carry on to the next lines
    struct PictureSlot
    {
        int picture = 0, motion = 0;
        double start = 0;
    };

    bool started() const;
    std::vector<Line> layout(int page) const;
    std::vector<Segment> splitTags(const std::u16string &line, Style &style) const;
    Line measure(std::vector<Segment> segments, const Style &style) const;
    static float height(const std::vector<Line> &lines);
    void dropPagesAboveTop();
    void drawText(const FeCanvas &canvas) const;
    void drawLines(const FeCanvas &canvas, float y, const std::vector<Line> &lines) const;
    void drawPictures(const FeCanvas &canvas);
    void drawPicture(const FeCanvas &canvas, const PictureSlot &slot, float progress) const;
    void loadPictures();
    void leave();

    double startTime = 0;   // the text starts moving 2 s after enter
    float textY = 50.f;
    int firstPage = 0;
    PictureSlot slots[2];
};
