// Argon: a cheat code typed in FE NAME ENTRY (app\cheat_code_entry.hpp).
#include "app/cheat_code_entry.hpp"

#include "logic/game_progress.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"

namespace
{
constexpr int kCheats = 7;
constexpr int kFirstCheatString = 0x3d4;
}  // namespace

void CheatCodeEntry::cheatCodeDone(const std::u16string &code)
{
    for (int i = 0; i < kCheats; ++i)
    {
        if (code == strings_.frontEnd(kFirstCheatString + i, ""))
        {
            progress_.cheats[i] = true;
            progressStore_.storeProgress();
            break;
        }
    }
    fe_->cheatMenu().open();
}
