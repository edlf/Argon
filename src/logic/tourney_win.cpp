// Argon: a tournament won (tourney_win.hpp).
#include "logic/tourney_win.hpp"

#include <cstdint>

namespace argon::tourney
{
namespace
{
enum Level
{
    Novice,
    Amateur,
    Expert,
    Master,
    Redshift
};

constexpr int kCharacters = 16, kOriginals = 8, kEggs = 8;
constexpr int kDracoOwner = 7, kDracoNemesis = 15;
constexpr uint32_t kDracoTrack = 1u << 7;

// one win's changes to the progress and the record of what is new
class TournamentWin
{
  public:
    explicit TournamentWin(GameProgress &progress) : progress_(progress)
    {
    }

    TourneyAwards apply(int level, int character)
    {
        if (!progress_.trophies[character][level])
        {
            progress_.trophies[character][level] = true;
            awards_.trophy = level;
        }
        if (level != Novice)
        {
            unlockWithWin(level, character);
        }
        openLevels(level, character);
        return awards_;
    }

  private:
    bool unlocked(int character) const
    {
        return (progress_.characters >> (character & 15)) & 1;
    }

    void unlockWithWin(int level, int character)
    {
        if (character < kOriginals)
        {
            unlockNemesisAndRoute(character);
        }
        else if (level > Amateur && int(progress_.trophies[character][Expert]) + int(progress_.trophies[character][Master]) == 1)
        {
            unlockNextEasterEgg();
        }
    }

    // the nemesis is character + 8 (not for Draco's owner: Kutoo comes with Master); the other route is track + 8
    void unlockNemesisAndRoute(int character)
    {
        const int nemesis = character + kOriginals;
        if (character != kDracoOwner && !unlocked(nemesis))
        {
            progress_.characters |= uint16_t(1u << nemesis);
            awards_.character = nemesis;
        }
        if (!(progress_.tracks & (1u << nemesis)))
        {
            progress_.tracks |= 1u << nemesis;
            awards_.track = nemesis;
        }
    }

    void unlockNextEasterEgg()
    {
        for (int egg = 0; egg < kEggs; ++egg)
        {
            if (!progress_.eggs[egg])
            {
                progress_.eggs[egg] = true;
                progress_.eggsOn[egg] = false;
                awards_.easterEgg = egg;
                return;
            }
        }
    }

    // Amateur opens Expert, Expert opens Master; Master and Redshift have their own (0x84160's switch)
    void openLevels(int level, int character)
    {
        if (level == Amateur || level == Expert)
        {
            openLevel(level + 1);
        }
        else if (level == Master)
        {
            masterWon(character);
        }
        else if (level == Redshift)
        {
            redshiftWon();
        }
    }

    void openLevel(int level)
    {
        if (progress_.speeds >= level)
        {
            return;
        }
        progress_.speeds = level;
        awards_.level = level;
    }

    // Draco Island; Draco's owner once the seven originals hold Master, or with it open, Draco's nemesis when the owner
    // wins it; Redshift once every open character (0..15, the loop to 0x1bbb42) holds Master
    void masterWon(int character)
    {
        if (!(progress_.tracks & kDracoTrack))
        {
            progress_.tracks |= kDracoTrack;
            awards_.draco = true;
        }
        if (!unlocked(kDracoOwner))
        {
            if (originalsHoldMaster())
            {
                progress_.characters |= uint16_t(1u << kDracoOwner);
                awards_.kaHekili = true;
            }
        }
        else if (character == kDracoOwner && !unlocked(kDracoNemesis))
        {
            progress_.characters |= uint16_t(1u << kDracoNemesis);
            awards_.kutoo = true;
        }
        if (everyOpenCharacterHolds(Master))
        {
            openLevel(Redshift);
        }
    }

    // Mars once every open character holds Redshift
    void redshiftWon()
    {
        if (progress_.mars || !everyOpenCharacterHolds(Redshift))
        {
            return;
        }
        progress_.mars = true;
        awards_.mars = true;
    }

    bool originalsHoldMaster() const
    {
        for (int character = 0; character < kDracoOwner; ++character)
        {
            if (!progress_.trophies[character][Master])
            {
                return false;
            }
        }
        return true;
    }

    bool everyOpenCharacterHolds(int level) const
    {
        for (int character = 0; character < kCharacters; ++character)
        {
            if (unlocked(character) && !progress_.trophies[character][level])
            {
                return false;
            }
        }
        return true;
    }

    GameProgress &progress_;
    TourneyAwards awards_;
};
}  // namespace

TourneyAwards winTournament(GameProgress &progress, int level, int character)
{
    return TournamentWin(progress).apply(level, character);
}
}  // namespace argon::tourney
