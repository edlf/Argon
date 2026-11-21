// Argon: the race events that change the music's mode (docs\audio\music.md "Music"), as the game tests them.
// The player's craft only.
#pragma once

namespace argon::music
{

// 0x63140: the player starting lap `lap` (1-based) of `finalLap` (0: no lap limit): the final lap steps the music up ...
constexpr bool finalLapStarts(int lap, int finalLap)
{
    return finalLap > 0 && lap == finalLap;
}

// ... else lap 2 plays on_start_second_lap.wav
constexpr bool secondLapStarts(int lap, int finalLap)
{
    return lap == 2 && !finalLapStarts(lap, finalLap);
}

// 0x635c0: the player's place (1-based) going to 1st steps the music up, leaving 1st steps it down
constexpr bool takesTheLead(int previousPlace, int place)
{
    return previousPlace != 1 && place == 1;
}

constexpr bool losesTheLead(int previousPlace, int place)
{
    return previousPlace == 1 && place != 1;
}

// what 0x635c0 does with the player's place this step
enum class LeadCue
{
    None,
    TookTheLead,   // the music steps up
    LostTheLead    // the music steps down
};

// the player's place as the music last saw it; the grid's place (the race's first step) is no change
class LeadWatch
{
  public:
    void reset()
    {
        lastPlace_ = 0;
    }
    LeadCue see(int place)
    {
        const int previous = lastPlace_;
        lastPlace_ = place;
        if (previous == 0)
        {
            return LeadCue::None;
        }
        if (takesTheLead(previous, place))
        {
            return LeadCue::TookTheLead;
        }
        return losesTheLead(previous, place) ? LeadCue::LostTheLead : LeadCue::None;
    }

  private:
    int lastPlace_ = 0;
};

}  // namespace argon::music
