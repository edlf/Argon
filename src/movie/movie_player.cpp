#include "movie/movie_player.hpp"
#include "audio/song_files.hpp"
#include "files/game_path.hpp"
#include "movie/vorbis_track.hpp"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <imgui.h>

namespace
{
const double kLastFrameSeconds = 1.0 / 30.0;   // how long the last frame stays (the game's videos run at 30 fps)

// the Vorbis stream muxed in the video (tools\assets\convert_assets.py); without one the video plays on the wall clock
std::shared_ptr<argon::Pcm> readSoundtrack(const std::filesystem::path &video)
{
    auto pcm = std::make_shared<argon::Pcm>();
    std::ifstream in(video, std::ios::binary);
    const std::vector<uint8_t> track = argon::vorbisTrackOf(in);
    if (track.empty())
    {
        return pcm;
    }
    if (argon::decodeOggVorbis(track, *pcm))
    {
        std::fprintf(stderr, "movie: %s: soundtrack %.1f s\n", video.filename().string().c_str(), pcm->seconds());
    }
    else
    {
        std::fprintf(stderr, "movie: %s: bad Vorbis soundtrack\n", video.string().c_str());
    }
    return pcm;
}
}  // namespace

void MoviePlayer::prepare(const std::filesystem::path &requested)
{
    const std::filesystem::path video = argon::findGamePath(requested);
    if (!audio_.ok() || soundtracks_.count(video))
    {
        return;
    }
    soundtracks_[video] = std::async(std::launch::async, readSoundtrack, video);
}

// the prepared soundtrack (waiting for it if still decoding), else decoded now
std::shared_ptr<argon::Pcm> MoviePlayer::takeSoundtrack(const std::filesystem::path &video)
{
    const auto prepared = soundtracks_.find(video);
    if (prepared == soundtracks_.end())
    {
        return readSoundtrack(video);
    }
    std::shared_ptr<argon::Pcm> pcm = prepared->second.get();
    soundtracks_.erase(prepared);
    return pcm;
}

bool MoviePlayer::play(const std::filesystem::path &requested)
{
    stop();
    const std::filesystem::path video = argon::findGamePath(requested);
    if (!std::filesystem::exists(video))
    {
        std::fprintf(stderr, "movie: %s missing (convert the .bik with tools\\assets\\convert_assets.py)\n", video.string().c_str());
        return false;
    }
    try
    {
        open(video);
    }
    catch (const std::exception &e)
    {
        std::fprintf(stderr, "movie: %s: %s\n", video.string().c_str(), e.what());
        stop();
        return false;
    }
    return true;
}

void MoviePlayer::open(const std::filesystem::path &video)
{
    reader_ = std::make_unique<TheoraVideoReader>(video);
    const VideoSize size{uint32_t(reader_->width()), uint32_t(reader_->height())};
    if (!texture_.ready() || texture_.size() != size)
    {
        device_.waitForIdle();
        texture_.create(device_, ui_, size);
    }
    videoEnded_ = !reader_->readFrame(next_);
    shown_ = VideoFrame{};
    newFrame_ = false;
    if (audio_.ok())
    {
        audio_.playMovie(takeSoundtrack(video));
    }
    startedAt_ = ImGui::GetTime();
    clock_ = 0.0;
}

// the texture stays (the next movie reuses it); the GPU may still read it
void MoviePlayer::stop()
{
    audio_.stopMovie();
    reader_.reset();
    newFrame_ = false;
}

// the soundtrack's position while it plays, else the wall clock since the start
double MoviePlayer::measuredClock() const
{
    const double soundtrack = audio_.movieSeconds();
    return soundtrack >= 0.0 ? soundtrack : ImGui::GetTime() - startedAt_;
}

bool MoviePlayer::finished() const
{
    return playing() && videoEnded_ && clock_ >= shown_.seconds + kLastFrameSeconds;
}

void MoviePlayer::update()
{
    if (!playing())
    {
        return;
    }
    clock_ = std::max(clock_, measuredClock());
    while (!videoEnded_ && next_.seconds <= clock_)
    {
        std::swap(shown_, next_);
        newFrame_ = true;
        videoEnded_ = !reader_->readFrame(next_);
    }
}

void MoviePlayer::draw(ImDrawList *drawList) const
{
    if (!playing() || !texture_.ready())
    {
        return;
    }
    drawList->AddImage(texture_.image(), ImVec2(0, 0), ImGui::GetIO().DisplaySize);
}

void MoviePlayer::recordUpload(argon::gfx::CommandList &commands, int frameSlot)
{
    if (!newFrame_ || shown_.bgrx.empty())
    {
        return;
    }
    texture_.upload(commands, frameSlot, shown_.bgrx.data());
    newFrame_ = false;
}

void MoviePlayer::destroy()
{
    stop();
    soundtracks_.clear();   // waits for any still decoding
    texture_.destroy();
}
