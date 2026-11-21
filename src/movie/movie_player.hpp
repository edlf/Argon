// Plays one video full screen (the game's Bink videos, converted by tools\assets\convert_assets.py to <name>.ogv, Theora + Vorbis):
// its frames decoded as the clock reaches them, its soundtrack through the audio system, whose position is the clock
// (the wall clock without sound, or once it has played out). The game stretches its videos over the whole screen
// (bink_draw_frame's quad).
#pragma once
#include <filesystem>
#include <future>
#include <map>
#include <memory>

#include "audio/audio.hpp"
#include "movie/theora_reader.hpp"
#include "render/video_texture.hpp"

struct ImDrawList;

class MoviePlayer
{
  public:
    MoviePlayer(argon::gfx::Device &device, argon::gfx::UiRenderer &ui, argon::Audio &audio) : device_(device), ui_(ui), audio_(audio)
    {
    }

    // starts decoding the video's soundtrack on a worker, for play to take (an 83 s intro takes ~0.1 s to decode)
    void prepare(const std::filesystem::path &video);
    bool play(const std::filesystem::path &video);   // false: missing or unreadable (reported)
    void stop();
    void destroy();   // stop and free the texture (shutdown, the device idle)
    bool playing() const
    {
        return reader_ != nullptr;
    }
    bool finished() const;
    void update();                                             // decodes up to the clock
    void draw(ImDrawList *drawList) const;
    void recordUpload(argon::gfx::CommandList &commands, int frameSlot);   // the newest decoded frame, once

  private:
    void open(const std::filesystem::path &video);
    std::shared_ptr<argon::Pcm> takeSoundtrack(const std::filesystem::path &video);
    double measuredClock() const;

    argon::gfx::Device &device_;
    argon::gfx::UiRenderer &ui_;
    argon::Audio &audio_;
    std::map<std::filesystem::path, std::future<std::shared_ptr<argon::Pcm>>> soundtracks_;   // prepared, by video file
    std::unique_ptr<TheoraVideoReader> reader_;
    VideoTexture texture_;
    VideoFrame shown_, next_;
    bool newFrame_ = false, videoEnded_ = false;
    double startedAt_ = 0;
    double clock_ = 0;   // never runs backwards (a played-out soundtrack hands over to the wall clock)
};
