// Argon: the race end's music fades (docs\audio\music.md "Music fades"). A on SCOREMENU / POINTSMENU, Continue on
// UPGRADEMENU and Yes on WINMENU / LOSEMENU start a second of "Saving Race"; each frame of it the page writes the
// music's volume (the music object 0x4406f0 +4, DirectSound millibels, set on the stream by 0xdf4c0) from the
// clock 0x292d58, linear in millibels at 10000 mB/s (0x1a35a8), clamped to -9600 mB (0x1a3968) and to the Music
// Volume setting (0xdee90):
//   Out (WINMENU 0xa0be6, LOSEMENU 0x85007): -t x 10000 for the whole second; the item then runs and the music stays
//     there until it stops (Race Again 0x85140 / 0xa0cf0 at once, the front end with the race) and starts again at
//     the setting (0xdf260).
//   Dip (SCOREMENU 0x953ef / 0x95520, POINTSMENU 0x8d46b / 0x8d58f, UPGRADEMENU 0x9ed9a / 0x9ef02): -t x 10000 up
//     to 0.5 s (0x1a3504), then t x 10000 - 9000 (0x1a3964) back up; past 1 s (0x1a3454) the next page opens with
//     the volume at the setting (0x954e4 / 0x8d54f / 0x9eebb).
#pragma once

namespace argon
{

enum class MusicFadeShape
{
    Out,
    Dip
};

// the music's volume (mB) `elapsed` s into a fade, the Music Volume setting at `settingMb`
float musicFadeMb(MusicFadeShape shape, double elapsed, float settingMb);
// a volume as a gain on the music at the setting (1 = the setting): 10^((mB - setting) / 2000)
float musicFadeGain(float levelMb, float settingMb);

// one page's second of leaving, on the caller's clock (seconds)
class MusicFade
{
  public:
    static constexpr double kSeconds = 1.0;

    void start(MusicFadeShape fadeShape, double now);
    void stop();
    bool active() const
    {
        return running;
    }
    bool over(double now) const;   // the second has passed: the page's next step runs
    double elapsed(double now) const;
    float levelMb(double now, float settingMb) const;

  private:
    bool running = false;
    MusicFadeShape shape = MusicFadeShape::Out;
    double startedAt = 0;
};

}  // namespace argon
