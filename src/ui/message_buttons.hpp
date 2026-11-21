// Argon: FE MESSAGEBOX's kinds and their buttons (open 0x2b000, input 0x2b4b0; docs\frontend\README.md
// "FE MESSAGEBOX"), without the drawing (ui\messagebox.cpp)
#pragma once
#include <vector>

namespace menu
{
enum class MessageKind
{
    Info,                  // the game's kind 0: one button "Press [BUTA] to continue" (fe 0x3c), A closes it
    Timed,                 // kind 1: no button; closes by itself once its seconds have passed
    Controller,            // kind 2: fe 7 "Please reconnect the controller ...", flashing, A / START closes it
    Saving,                // kinds 4 (profile) / 0xb (ghost): "Saving" (fe 0x200) for 0.5 s, then the caller saves
    YesNo,                 // kind 5: Yes (fe 3) / No (fe 2)
    ReplaceGhost,          // kind 6: fe 0x1a7 "Yes, replace the old ghost" / 0x1a8 "No, keep the old ghost"
    NoFreeBlocks,          // kind 7: the three buttons of kind 9, no input (no caller found)
    NoFreeBlocksProfile,   // kind 8: Continue without saving / Return to Select Profile / Launch the Xbox Dashboard
    NoFreeBlocksGhost,     // kind 9: Continue without saving / Return to Select Speed / Launch the Xbox Dashboard
    Loading,               // kind 0xa (ghost): "Loading" (fe 0x201), as Saving
};

struct ButtonLabel
{
    int feString;
    const char *fallback;
};

class MessageButtons
{
  public:
    explicit MessageButtons(MessageKind boxKind) : kind(boxKind)
    {
    }

    std::vector<ButtonLabel> labels() const;   // empty: the box shows no button
    int count() const
    {
        return int(labels().size());
    }
    // 0x2b408: kinds 5..9 open with the cursor on the second button (No, keep the old ghost, Return to ...)
    int first() const;
    bool takesCursorInput() const;   // 0x2b7cc: kinds 5, 6, 8, 9 (up / down move, A picks; B does nothing)
    // up / down (0x884b0 / 0x884f0): two buttons swap; three go round (up 2 -> 1 -> 0 -> 2)
    int moved(int at, bool down) const;

  private:
    MessageKind kind;
};
}  // namespace menu
