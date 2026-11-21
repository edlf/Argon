// Argon: a cheat code typed in FE NAME ENTRY (enter 0x1abd0; docs\frontend\cheats.md "Easter Eggs and Cheats"): the
// whole text against fe 0x3d4 + i (case-sensitive); a match unlocks that cheat, then FE CHEATMENU opens.
#pragma once
#include <memory>

#include "ui/page_hosts.hpp"

class FeShell;
class GameStrings;
namespace argon
{
struct GameProgress;
}

class CheatCodeEntry final : public CheatCodes
{
  public:
    CheatCodeEntry(const GameStrings &strings, argon::GameProgress &progress, ProgressStore &progressStore, const std::unique_ptr<FeShell> &fe)
        : strings_(strings), progress_(progress), progressStore_(progressStore), fe_(fe)
    {
    }
    void cheatCodeDone(const std::u16string &code) override;

  private:
    const GameStrings &strings_;
    argon::GameProgress &progress_;
    ProgressStore &progressStore_;          // the cheat unlocked written back
    const std::unique_ptr<FeShell> &fe_;    // made with App, before the shell
};
