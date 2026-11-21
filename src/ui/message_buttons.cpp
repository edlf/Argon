// Argon: FE MESSAGEBOX's buttons per kind (ui\message_buttons.hpp)
#include "ui/message_buttons.hpp"

namespace menu
{
namespace
{
const ButtonLabel kContinueWithoutSaving{0x244, "Continue without saving."};
const ButtonLabel kLaunchDashboard{0x245, "Launch the Xbox Dashboard."};
}  // namespace

// open 0x2b000: the buttons' texts per kind
std::vector<ButtonLabel> MessageButtons::labels() const
{
    switch (kind)
    {
    case MessageKind::Info:
        return {{0x3c, "Press [BUTA] to continue"}};
    case MessageKind::YesNo:
        return {{3, "Yes"}, {2, "No"}};
    case MessageKind::ReplaceGhost:
        return {{0x1a7, "Yes, replace the old ghost"}, {0x1a8, "No, keep the old ghost"}};
    case MessageKind::NoFreeBlocksProfile:
        return {kContinueWithoutSaving, {0x246, "Return to Select Profile"}, kLaunchDashboard};
    case MessageKind::NoFreeBlocks:
    case MessageKind::NoFreeBlocksGhost:
        return {kContinueWithoutSaving, {0x247, "Return to Select Speed"}, kLaunchDashboard};
    default:
        return {};
    }
}

int MessageButtons::first() const
{
    return count() > 1 ? 1 : 0;
}

bool MessageButtons::takesCursorInput() const
{
    return count() > 1 && kind != MessageKind::NoFreeBlocks;
}

int MessageButtons::moved(int at, bool down) const
{
    const int buttons = count();
    if (buttons < 2)
    {
        return at;
    }
    return (at + (down ? 1 : buttons - 1)) % buttons;
}
}  // namespace menu
