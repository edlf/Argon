// Argon: FE INTROSCREENS (vtable 0x19e368: enter 0x12770, update 0x12050, input 0x12630; docs\frontend\README.md
// "Intro screens"), the shell's home page at boot: media\videos\ms_game_studios.bik, media\videos\curly_seq.bik, then
// Locale\<language>\intro.bik, each started when the last one reaches its last frame, then FE TITLE. A or START skips
// to the next video (after intro.bik: FE TITLE). The game skips the page's videos when another title launched it
// (XGetLaunchInfo, 0x52c8bf); the viewer's --no-intro and settings.json's skipIntroVideos do the same.
// The language folder follows the dashboard (2 Japanese, 3 German, 4 French, 6 Italian, else English: the Spanish
// intro.bik on the disc is never played); the viewer's follows settings.json.
// The viewer plays the .ogv (picture + soundtrack) next to each .bik (tools\assets\convert_assets.py); a missing one is skipped.
#include "ui/intro_screens.hpp"

#include "audio/audio.hpp"
#include "files/string_table.hpp"
#include "hud/hud_painter.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_id.hpp"
#include "ui/fe_shell.hpp"

namespace fs = std::filesystem;

namespace
{
const size_t kVideoCount = 3;
}

fs::path IntroScreensPage::videoPath(size_t index) const
{
    const fs::path game(gameFolder);
    if (index == 0)
    {
        return game / "media/videos/ms_game_studios.ogv";
    }
    if (index == 1)
    {
        return game / "media/videos/curly_seq.ogv";
    }
    return game / settings.localeFolder() / "intro.ogv";
}

void IntroScreensPage::open()
{
    door.openMenu();
    shell.goTo(menu::PageIntroScreens);
}

// 0x12770: video state 0; the update (0x12050) opens ms_game_studios.bik. The soundtracks decode on workers meanwhile.
void IntroScreensPage::enter()
{
    for (size_t i = 0; i < kVideoCount; ++i)
    {
        movie->prepare(videoPath(i));
    }
    playFrom(0);
}

void IntroScreensPage::playFrom(size_t index)
{
    for (current_ = index; current_ < kVideoCount; ++current_)
    {
        if (movie->play(videoPath(current_)))
        {
            return;
        }
    }
    leave();
}

// 0x12050 sequence 7: shell state 0, FE TITLE
void IntroScreensPage::leave()
{
    movie->stop();
    shell.title().open();
}

// 0x12050 (bink_is_last_frame moves on) and 0x12630 (A or START skips; the viewer's: once the front end's models are
// loaded, ui\fe_models.hpp)
void IntroScreensPage::input(const menu::MenuInput &in)
{
    movie->update();
    const bool skip = in.a && shell.models().preloaded();
    if (skip || movie->finished())
    {
        playFrom(current_ + 1);
    }
}

void IntroScreensPage::draw(const FeCanvas &canvas)
{
    movie->draw(canvas.drawList());
}
