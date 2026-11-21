// Argon: FE SPEEDSELECT "Select Speed" (vtable 0x19d720) and FE TRACK INFO "Track Data" (vtable 0x19e300),
// both opened from FE COURSE SELECT (ui\speed.cpp)
#pragma once
#include "ui/fe_canvas.hpp"
#include "ui/page_hosts.hpp"

#include <string>

class GameModes;

#include "ui/fe_page.hpp"

class SpeedSelectPage : public FePage
{
  public:
    struct Services
    {
        int &speedClass;   // the pick, 0..4
        const UnlockRules &rules;   // the highest open speed
        GameModes &modes;   // the mode told of the pick
    };
    SpeedSelectPage(const PageKit &kit, const Services &services) : FePage(kit), speedClass(services.speedClass), rules(services.rules), modes(services.modes)
    {
    }
    void open(int pilot, int track);   // the cursor on the game speed, at most the highest open one

    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    FeScene scene() override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kSpeedSelect;
    }

  private:
    int &speedClass;   // the pick, 0..4
    const UnlockRules &rules;   // the highest open speed
    GameModes &modes;   // the mode told of the pick
    static constexpr int kSpeeds = 5;
    bool selectedIsOpen() const;
    void pickSpeed();

    int pilot = 0, track = 0, selected = 0;
};

class TrackInfoPage : public FePage
{
  public:
    struct Services
    {
        const UnlockRules &rules;   // the nemesis pictures
        const std::string &gameFolder;
        const bool &speedMph;   // Speed Units
    };
    TrackInfoPage(const PageKit &kit, const Services &services)
        : FePage(kit), rules(services.rules), gameFolder(services.gameFolder), speedMph(services.speedMph)
    {
    }
    void open(int track);

    void enter() override;   // its pictures loaded here: drawing only looks them up
    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    std::u16string title() const override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kInfo;
    }

  private:
    const UnlockRules &rules;   // the nemesis pictures
    const std::string &gameFolder;
    const bool &speedMph;   // Speed Units
    static constexpr int kLinesShown = 4;
    int infoIndex() const;   // the strings' track index: 0..7 (second routes as the first), 0x10 Mars
    bool isMars() const
    {
        return infoIndex() == 0x10;
    }
    std::u16string description() const;
    int descriptionLines() const;
    void drawTrackPicture(const FeCanvas &canvas) const;
    void drawFacts(const FeCanvas &canvas) const;
    void drawDescription(const FeCanvas &canvas) const;
    void drawScrollArrows(const FeCanvas &canvas, int lines) const;

    int track = 0, firstLine = 0;
};
