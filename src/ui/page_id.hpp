// Argon: the front end's pages (the game's FE page names; the numbers are --menu-page's, and the ranges below are
// used): the shell keeps one page object per id (ui\fe_shell.hpp)
#pragma once

namespace menu
{

enum Page : int
{
    PageMain,            //  0 FE MAIN (ui\mainmenu.cpp)
    PageCharSelect,      //  1 FE CHARSELECT (ui\charsel.cpp)
    PageCharInfo,        //  2 FE CHAR INFO
    PageShipInfo,        //  3 FE SHIP INFO
    PageCourseSelect,    //  4 FE COURSE SELECT
    PageLoader,          //  5 LOADER, the race loading screen (ui\loader.cpp)
    PageTitle,           //  6 FE TITLE (ui\title.cpp)
    PageChooseSave,      //  7 FE CHOOSESAVE (ui\choosesave.cpp)
    PageNameEntry,       //  8 FE NAME ENTRY
    PageOptions,         //  9 FE OPTIONS (ui\options.cpp)
    PageGameSettings,    // 10 FE OPTIONS MAIN
    PageSoundSettings,   // 11 FE OPTIONS SOUND
    PageCredits,         // 12 FE OPTIONS CREDITS (ui\credits.cpp)
    PageMusicSelect,     // 13 FE MUSICSELECT (ui\options.cpp)
    PageTrackRecords,    // 14 FE FASTESTLAP (ui\records.cpp); 14..16 follow the Options rows 2..4
    PageHighScores,      // 15 FE HIGHSCORES
    PageGhosts,          // 16 FE GHOSTMANAGE
    PageEggs,            // 17 FE EASTEREGGS (ui\eggs.cpp)
    PageCheats,          // 18 FE CHEATMENU
    PageLevelMusic,      // 19 FE LEVELMUSIC (ui\playlist.cpp)
    PageSpeedSelect,     // 20 FE SPEEDSELECT (ui\speed.cpp)
    PageTrackInfo,       // 21 FE TRACK INFO
    PageTourneyType,     // 22 FE TOURNEYTYPE (ui\tourney.cpp)
    PageGhostSelect,     // 23 FE GHOSTSELECT (ui\ghostselect.cpp)
    PageGhostSave,       // 24 FE GHOSTSAVE (ui\ghostsave.cpp)
    PageIntroScreens,    // 25 FE INTROSCREENS, the videos before FE TITLE (ui\intro_screens.cpp)
    PageAwards,          // 26 FE AWARDS, the rewards after a tournament win (ui\awards.cpp)
    PageArgonSettings,   // 27 Argon Options, the viewer's own settings, a FE MAIN row (ui\argon_options.cpp; docs\argon\differences.md)
    PageArgonGeneral,    // 28 its lists (ui\options.cpp): General
    PageArgonGraphics,   // 29 Graphics
    PageArgonAudio,      // 30 Audio
    PageArgonFixes,      // 31 Game Fixes
    PageKeyBindings,     // 32 Key Bindings (ui\key_bindings_page.cpp)
    PageCount
};

}  // namespace menu
