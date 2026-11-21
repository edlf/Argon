// Argon: the game's music modes (docs\audio\music.md "Music"): the music object 0x4406f0 picks a track folder's
// file by its mode +0x6ac (0xdf350): 0 intro.wav, 1 on_start.wav, 2 on_start_second_lap.wav, 3..7 <folder>1..5.wav
// (capped below the folder's last segment), 8 the last segment. A file ending (the stream 0xdfad0) starts the mode's
// file; when the mode has not changed since that file started, the mode steps up instead (0xded20: 3, 4, 5, 6, 3)
// and the same file plays again, so each segment plays twice. Game events step the mode up or down meanwhile.
// Songs: 0 intro, 1 on_start, 2 on_start_second_lap, 3 + k the (k + 1)th segment (MusicFolder::playlist).
#pragma once
#include <filesystem>
#include <vector>

#include "audio/play_order.hpp"

namespace argon
{

// a music folder's files by their part: intro.wav, on_start.wav, on_start_second_lap.wav, <folder><n>.wav
struct MusicFolder
{
    std::filesystem::path intro, onStart, secondLap;   // empty when the folder has none
    std::vector<std::filesystem::path> segments;       // <folder>1.wav, 2, ... in order

    static MusicFolder read(const std::filesystem::path &dir);   // a folder missing: no files
    std::vector<std::filesystem::path> playlist() const;          // the songs MusicModes picks from
};

class MusicModes : public PlayOrder
{
  public:
    enum Mode
    {
        Intro = 0,
        OnStart = 1,
        SecondLap = 2,
        FirstSegment = 3,
        Finale = 8,
    };

    explicit MusicModes(const MusicFolder &folder);

    void set(int mode);   // 0xdec40: a stinger the folder lacks: on_start / the first segment, or no change
    void stepUp();        // the race's events (0x4ecc6, 0x6318b, 0x635ee): one up while 3..6
    void stepDown();      // (0x4eda1, 0x63610): one down while over 3
    void playFinale();    // FE AWARDS (0x8eb10): mode 8, written directly
    void startOver(int mode);   // 0xc8e10 (the music restarted at mode 0), then set(mode): View Replay's on_start
    int mode() const
    {
        return mode_;
    }

    void restart(size_t songs) override;
    size_t next() const override;
    void started(size_t song) override;

  private:
    bool repeatsTheSong() const;
    size_t songOf(int mode) const;
    void cycle();   // 0xded20

    bool hasIntro_, hasOnStart_, hasSecondLap_;
    int segments_;
    int mode_ = FirstSegment;
    int lastStarted_ = -1;        // +0x6b0: the mode whose file started last
    int afterStinger_ = FirstSegment;   // +0x6b4: the mode after on_start / on_start_second_lap
    size_t playing_ = 0;
};

}  // namespace argon
