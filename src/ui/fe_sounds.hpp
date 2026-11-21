// Argon: the front end's UI sounds (sound_play_ui 0xc3ca0, the ids of audio\frontend\frontend.txt) and, per page,
// the sound each input event plays: every call in 0x10000..0x40000 with the shared list cursor (0xad070) and the
// settings list (0xac040 / 0xac6c0 / 0xac8f0); docs\frontend\README.md "UI sounds". The pages report their events
// (FePage::play), the shell the switch after a slide-out (FeShell::slideOut).
#pragma once
#include <optional>

namespace menu
{
enum class UiSound
{
    Select = 0,          // SF_UI_SELECT1 ui_button1p
    BadSelect = 1,       // SF_UI_BADSELECT1 ui_rollover3
    Rollover = 2,        // SF_UI_ROLLOVER1 ui_rollover1
    ScreenForward = 3,   // SF_UI_SCREENFORWARD ui_xition1_LR
    ScreenBack = 4,      // SF_UI_SCREENBACK ui_xition2
};

enum class UiEvent
{
    Cursor,        // the cursor, a list or a choice moved
    Pick,          // A taken on the page: a key, a box's button, a cheat, a player's character
    Refused,       // A on something locked
    Cancel,        // B undoing the page's own step (a player's pick, the route choice)
    Back,          // B or Exit pressed, leaving for the previous page
    Forward,       // A pressed, leaving for the next page
    LeftBack,      // the switch to the previous page (after the slide-out: the page's update)
    LeftForward,   // the switch to the next page
};

struct PageSounds
{
    std::optional<UiSound> cursor, pick, refused, cancel, back, forward, leftBack, leftForward;

    constexpr std::optional<UiSound> soundFor(UiEvent event) const
    {
        switch (event)
        {
        case UiEvent::Cursor:
            return cursor;
        case UiEvent::Pick:
            return pick;
        case UiEvent::Refused:
            return refused;
        case UiEvent::Cancel:
            return cancel;
        case UiEvent::Back:
            return back;
        case UiEvent::Forward:
            return forward;
        case UiEvent::LeftBack:
            return leftBack;
        case UiEvent::LeftForward:
            return leftForward;
        }
        return std::nullopt;
    }
};

// the call sites per page; "list" is the shared cursor 0xad070 (0xad0e1 / 0xad158: a neighbour row to move to)
namespace fe_sounds
{
inline constexpr PageSounds kSilent{};   // FE INTROSCREENS, the loader, FE GHOSTSAVE (only its message boxes)

// FE TITLE update 0x34660: leaving for FE CHOOSESAVE (0x3485e, 0x348a5)
inline constexpr PageSounds kTitle{.leftForward = UiSound::ScreenForward};

// FE CHOOSESAVE input 0x1c070: the list (0x1c644 scrolling, list), A on a profile that loads / New / Y (0x1c259,
// 0x1c6d4), B (0x1c807); X and a profile that does not load open a box without a sound
inline constexpr PageSounds kChooseSave{.cursor = UiSound::Rollover, .back = UiSound::ScreenBack, .forward = UiSound::ScreenForward};

// FE NAME ENTRY input 0x38f10: every d-pad press (0x39031, 0x3905a, 0x39083, 0x390ae), Y / X / A (0x38f3d, 0x38f64,
// 0x38fce), B (0x38f8d); done 0x3a110 with a valid name (0x3a150, 0x3a654)
inline constexpr PageSounds kNameEntry{.cursor = UiSound::Rollover, .pick = UiSound::Select, .back = UiSound::ScreenBack, .forward = UiSound::ScreenForward};

// FE MAIN input 0x28e90: up / down (0x29a51, 0x29b43), Multiplayer with fewer than two pads (0x29904); update
// 0x2a520 once the slide-out is over: back (0x2a5e9), the next page (0x2a664)
inline constexpr PageSounds kMain{
    .cursor = UiSound::Rollover, .refused = UiSound::BadSelect, .leftBack = UiSound::ScreenBack, .leftForward = UiSound::ScreenForward};

// FE CHARSELECT input 0x163b0: the cursor (0x16491, 0x16672, 0x16793), a multiplayer join / pick that does not start
// the race (0x16f81; the last pick goes on silently), a locked character and a multiplayer pick undone (0x173e7), B
// (0x173ff), A / Y / X single player (0x16885, 0x17007, 0x1707e)
inline constexpr PageSounds kCharSelect{.cursor = UiSound::Rollover,
                                        .pick = UiSound::Select,
                                        .refused = UiSound::BadSelect,
                                        .cancel = UiSound::BadSelect,
                                        .back = UiSound::ScreenBack,
                                        .forward = UiSound::ScreenForward};

// FE CHAR INFO input 0x14a50 (0x14a95), FE SHIP INFO 0x32e80 (0x32eb9), FE TRACK INFO 0x371e0 (0x3721d): B only, the
// scrolling is silent
inline constexpr PageSounds kInfo{.back = UiSound::ScreenBack};

// FE COURSE SELECT input 0x1da10 / pick 0x1e5e0 / leave 0x1e920: the list, the route choice opened (0x1e619 Mars,
// 0x1e66b), a locked row (0x1e62b, 0x1e68f), B closing the route choice (0x1df57), B (0x1dfa9), a track / route
// picked (0x1e94a), Y Track Data (0x1dad7)
inline constexpr PageSounds kCourseSelect{.cursor = UiSound::Rollover,
                                          .pick = UiSound::Select,
                                          .refused = UiSound::BadSelect,
                                          .cancel = UiSound::BadSelect,
                                          .back = UiSound::ScreenBack,
                                          .forward = UiSound::ScreenForward};

// FE TOURNEYTYPE input 0x35940: the list, a locked level (0x35af3), B (0x359c6); New / Continue and the start silent
inline constexpr PageSounds kTourneyType{.cursor = UiSound::Rollover, .refused = UiSound::BadSelect, .back = UiSound::ScreenBack};

// FE SPEEDSELECT input 0x33ac0: up / down (0x33ef8, 0x33f88), an open speed (0x33b6e), a locked one (0x33e75), B
// (0x33afa); its update 0x33fa0 goes on silently
inline constexpr PageSounds kSpeedSelect{.cursor = UiSound::Rollover, .pick = UiSound::Select, .refused = UiSound::BadSelect, .back = UiSound::ScreenBack};

// FE GHOSTSELECT input 0x23b40: the list (0x240ce scrolling), A (0x23b96); update 0x239c0: back to FE SPEEDSELECT
// (0x23aaf), on to the loader (0x23a32)
inline constexpr PageSounds kGhostSelect{
    .cursor = UiSound::Rollover, .pick = UiSound::Select, .leftBack = UiSound::ScreenBack, .leftForward = UiSound::ScreenForward};

// FE FASTESTLAP (input 0x20a60, update 0x206c0) and FE HIGHSCORES (0x25f20, 0x25b20): left / right and up / down
// (0x20e90, 0x20efd, 0x20f7a; 0x2635c, 0x263e3, 0x26460), A after a race (0x20e1f; 0x262c3), B plays SELECT
// (0x20aaa; 0x25f84), the switch back (0x20749, 0x207c5; 0x25ba9, 0x25c25)
inline constexpr PageSounds kRecordTable{.cursor = UiSound::Rollover, .pick = UiSound::Select, .back = UiSound::Select, .leftBack = UiSound::ScreenBack};

// FE GHOSTMANAGE input 0x22000: another track, the list scrolling (0x221f8, 0x22286, 0x2238e) and the list; update
// 0x21b30: back to FE OPTIONS (0x21b8d); B and X themselves silent
inline constexpr PageSounds kGhostManage{.cursor = UiSound::Rollover, .leftBack = UiSound::ScreenBack};

// FE OPTIONS input 0x2cef0: the list, B (0x2cf51), Exit (0x2dfd0: 0x2dfe8); update 0x2d830: a row's page (0x2d910)
inline constexpr PageSounds kOptions{.cursor = UiSound::Rollover, .back = UiSound::ScreenBack, .leftForward = UiSound::ScreenForward};

// FE OPTIONS MAIN / SOUND (settings_input 0xac040): up / down, left / right that changed a value (not Engine Volume:
// the engine test plays, 0x2ea60) (0xac337); Reset to Default answered Yes (0xac725); update 0xac8f0: B / Exit
// (0xac98e), Music Options (0xaca02)
inline constexpr PageSounds kSettings{
    .cursor = UiSound::Rollover, .pick = UiSound::Select, .leftBack = UiSound::ScreenBack, .leftForward = UiSound::ScreenForward};

// FE MUSICSELECT input 0x2bd80: the list, Default / Custom (0x2c19d), Edit Custom Playlist with the default music
// (0x2c120); update 0x2c430: back (0x2c504), FE LEVELMUSIC (0x2c48f)
inline constexpr PageSounds kMusicSelect{
    .cursor = UiSound::Rollover, .refused = UiSound::BadSelect, .leftBack = UiSound::ScreenBack, .leftForward = UiSound::ScreenForward};

// FE LEVELMUSIC: its input 0x27380 is silent; update 0x27650 back to FE MUSICSELECT (0x27785)
inline constexpr PageSounds kLevelMusic{.leftBack = UiSound::ScreenBack};

// FE EASTEREGGS input 0x1f180: the list, B / Exit (0x1f1a6, 0x1f1f9); switching an egg is silent
inline constexpr PageSounds kEasterEggs{.cursor = UiSound::Rollover, .back = UiSound::ScreenBack};

// FE CHEATMENU input 0x1a5e0: the list, a cheat activated (0x1a779), a locked row or A on a switch (0x1ab92,
// 0x1a7d1), B / Exit / Enter Cheat Code (0x1a61d, 0x1a86e, 0x1a899); switching is silent
inline constexpr PageSounds kCheatMenu{.cursor = UiSound::Rollover, .pick = UiSound::Select, .refused = UiSound::BadSelect, .back = UiSound::ScreenBack};

// FE OPTIONS CREDITS: input 0x2e110 leaves silently, fe_credits_play 0x2e390 plays the switch (0x2e523)
inline constexpr PageSounds kCredits{.leftBack = UiSound::ScreenBack};

// FE AWARDS update 0x133c0: on to FE MAIN (0x13427)
inline constexpr PageSounds kAwards{.leftForward = UiSound::ScreenForward};

// FE MESSAGEBOX input 0x2b4b0: up / down on the buttons (0x2b841, 0x2b892), A on an info box or a button
// (0x2b5f4); the controller message closes silently
inline constexpr PageSounds kMessageBox{.cursor = UiSound::Rollover, .pick = UiSound::Select};
}  // namespace fe_sounds
}  // namespace menu
