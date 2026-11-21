// Argon: FE INTROSCREENS, the videos before the title screen (ui\intro_screens.cpp)
#pragma once
#include "ui/fe_canvas.hpp"
#include <cstddef>
#include <filesystem>

#include "movie/movie_player.hpp"
#include "saves/viewer_settings.hpp"
#include "ui/page_hosts.hpp"
#include "memory"
#include "string"
#include "ui/fe_page.hpp"

class IntroScreensPage : public FePage
{
  public:
    struct Services
    {
        MenuDoor &door;   // the videos over: FE TITLE
        const std::unique_ptr<MoviePlayer> &movie;   // made with the renderer
        const std::string &gameFolder;
        const ViewerSettings &settings;   // the language's folder
    };
    IntroScreensPage(const PageKit &kit, const Services &services)
        : FePage(kit), door(services.door), movie(services.movie), gameFolder(services.gameFolder), settings(services.settings)
    {
    }
    void open();   // the shell on this page (start-up)

    void enter() override;
    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    bool bare() const override
    {
        return true;
    }

  private:
    MenuDoor &door;   // the videos over: FE TITLE
    const std::unique_ptr<MoviePlayer> &movie;   // made with the renderer
    const std::string &gameFolder;
    const ViewerSettings &settings;   // the language's folder
    std::filesystem::path videoPath(size_t index) const;
    void playFrom(size_t index);
    void leave();

    size_t current_ = 0;
};
