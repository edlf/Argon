// Argon: the character and track pages: FE CHARSELECT (vtable 0x19e298), FE CHAR INFO (0x19d7f0),
// FE SHIP INFO (0x19dd48) and FE COURSE SELECT (0x19de18) (ui\charsel.cpp). The character picked (RacePicks::character) and
// the course row (RacePicks::courseRow) are the front end's, shared with the later pages and the race.
#pragma once
#include "ui/fe_canvas.hpp"
#include <glm/glm.hpp>

#include "ui/fe_page.hpp"
#include "ui/multiplayer_select.hpp"

class CharSelectPage : public FePage
{
  public:
    CharSelectPage(const PageKit &kit, const PilotPicks &picks) : FePage(kit), picks(picks), multiplayer(kit, picks, *this)
    {
    }
    void enter() override;
    void step() override;
    void input(const menu::MenuInput &in) override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kCharSelect;
    }
    void draw(const FeCanvas &canvas) override;
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    FeScene scene() override;
    bool hasJoined(int pad) const
    {
        return multiplayer.hasJoined(pad);
    }
    std::vector<FeScene> boxScenes() override;   // multiplayer: the players' boxes

  private:
    PilotPicks picks;
    void moveCursor(const menu::MenuInput &in);
    void pickCharacter();
    void drawSelected(const FeCanvas &canvas) const;
    std::u16string trophyLevelText(int pilot) const;   // 0x19c48
    void drawGrid(const FeCanvas &canvas) const;
    void drawCharSelectHints(const FeCanvas &canvas) const;

    static constexpr float kInfoShown = 560.f, kInfoAway = 1280.f;   // 0x1a36cc / 0x1a36d0
    float infoX = kInfoAway;   // the info rows' right edge (elements +0x1e0..: all five slide together)
    MultiplayerSelect multiplayer;   // game mode 4: a cursor per controller
};

class CharInfoPage : public FePage
{
  public:
    CharInfoPage(const PageKit &kit, const PilotPicks &picks) : FePage(kit), picks(picks)
    {
    }
    void enter() override;
    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kInfo;
    }
    FeScene scene() override;

  private:
    PilotPicks picks;
    static constexpr int kLinesShown = 5, kScrollLines = 4;   // xemu / 0x14a50
    std::u16string biography() const;
    int biographyLines() const;
    void drawFacts(const FeCanvas &canvas) const;
    void drawBiography(const FeCanvas &canvas) const;

    int firstLine = 0;
};

class ShipInfoPage : public FePage
{
  public:
    ShipInfoPage(const PageKit &kit, const PilotPicks &picks) : FePage(kit), picks(picks)
    {
    }
    void enter() override;
    void input(const menu::MenuInput &in) override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kInfo;
    }
    void draw(const FeCanvas &canvas) override;
    void step() override
    {
        spin -= 0.5f;
    }   // 0x33230: the ship turns 0.5 deg a frame
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    FeScene scene() override;

  private:
    PilotPicks picks;
    glm::mat4 shipPlacement() const;
    void drawStatBars(const FeCanvas &canvas) const;
    void drawNameAndPortrait(const FeCanvas &canvas) const;
    void drawShipFacts(const FeCanvas &canvas) const;

    float spin = 0.f;   // degrees
};

class CourseSelectPage : public FePage
{
  public:
    CourseSelectPage(const PageKit &kit, const PilotPicks &picks) : FePage(kit), picks(picks)
    {
    }
    void enter() override;
    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    void step() override;
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kCourseSelect;
    }
    FeScene scene() override;   // the Earth turned to the track, or Mars
    void openRouteChoice();     // 0x1e4d0: the two routes' buttons, the cursor on the first

  private:
    PilotPicks picks;
    static constexpr int kRows = 9, kMarsRow = 8;
    bool rowIsOpen(int row) const;
    bool hasSecondRoute(int row) const;
    glm::mat4 globePlacement() const;
    void turnGlobeToRow();
    void chooseRoute(const menu::MenuInput &in);
    void pickRow();
    void leaveForSpeedSelect(int track);
    int pickedTrack() const;   // the track id of the row and route: + 8 for the second route, Mars 0x10 / 0x11
    void drawRows(const FeCanvas &canvas) const;
    void drawRouteChoice(const FeCanvas &canvas) const;
    void drawGlobeBrackets(const FeCanvas &canvas) const;

    bool choosingRoute = false;   // the route choice (1: normal / 2: alternate) is open
    int route = 0;
    float rowX[kRows]{};
    glm::vec2 globeTurn{0.f}, globeTarget{0.f};   // Earth x / y turn (radians), eased 0.2 a frame toward the track's
};
