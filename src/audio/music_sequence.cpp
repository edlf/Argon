#include "audio/music_sequence.hpp"

#include <chrono>
#include <utility>

namespace argon
{
namespace
{
// the worker's job: the song opened and its first seconds decoded, so it can start at once
std::shared_ptr<SongFeed> openForStream(MusicSequence::SongOpener open, std::filesystem::path file)
{
    std::shared_ptr<SongFeed> song = open(file);
    if (song)
    {
        song->fill();
    }
    return song;
}
}  // namespace

MusicSequence::MusicSequence(AudioOutput &output, SongOpener open, std::launch policy) : output_(output), open_(open), policy_(policy)
{
    if (policy_ == std::launch::async)
    {
        filler_ = std::make_unique<SongFeedFiller>();
    }
}

void MusicSequence::setPlaylist(std::vector<std::filesystem::path> files)
{
    setPlaylist(std::move(files), folderOrder_);
}

void MusicSequence::setPlaylist(std::vector<std::filesystem::path> files, PlayOrder &order)
{
    stop();
    files_ = std::move(files);
    order_ = &order;
    order_->restart(files_.size());
    readySong_.reset();
    readyIndex_ = kNone;
    unreadableIndex_ = kNone;
}

void MusicSequence::start()
{
    if (files_.empty() || stream_)
    {
        return;
    }
    stream_ = std::make_unique<SongStream>(output_);
    if (!stream_->ok())
    {
        stream_.reset();
        return;
    }
    stream_->setVolume(volume_);
    switchesSeen_ = 0;
}

void MusicSequence::stop()
{
    if (!stream_)
    {
        return;
    }
    stream_.reset();   // first: the audio thread stops reading the songs
    reading_ = {};     // waits for a song still being decoded
    playingSong_.reset();
    queuedSong_.reset();
    playingIndex_ = queuedIndex_ = kNone;
    keepSongsFilled();
}

void MusicSequence::startOpening(size_t index)
{
    readingIndex_ = index;
    reading_ = std::async(policy_, openForStream, open_, files_[index]);
}

// the song the worker decoded, once it is done (a deferred read decodes here); an unreadable one is told to the order
void MusicSequence::collectRead()
{
    if (!reading_.valid() || reading_.wait_for(std::chrono::seconds(0)) == std::future_status::timeout)
    {
        return;
    }
    std::shared_ptr<SongFeed> song = reading_.get();
    if (!song)
    {
        unreadableIndex_ = readingIndex_;
        order_->unreadable(readingIndex_);
        return;
    }
    readySong_ = std::move(song);
    readyIndex_ = readingIndex_;
}

// one read at a time: a song wanted while another decodes waits for it
void MusicSequence::requestRead(size_t index)
{
    if (reading_.valid() || index == readyIndex_ || index == unreadableIndex_)
    {
        return;
    }
    startOpening(index);
}

// queues that song after the playing one, replacing a queued one the stream has not reached; false: not opened yet (a
// song played again is opened again: a feed plays once)
bool MusicSequence::offer(size_t index)
{
    if (index != readyIndex_ || !readySong_)
    {
        return false;
    }
    std::shared_ptr<SongFeed> song = readySong_;
    if (!queuedSong_)
    {
        stream_->queue(*song);
    }
    else if (!stream_->requeue(*queuedSong_, *song))
    {
        return true;   // the stream moved on to the old one: followStream takes it next frame
    }
    queuedSong_ = std::move(song);
    queuedIndex_ = index;
    readySong_.reset();   // the queue keeps it
    readyIndex_ = kNone;
    return true;
}

// the stream moved on to the queued song: it is the one playing, the one before it can go
void MusicSequence::followStream()
{
    while (switchesSeen_ != stream_->switches())
    {
        ++switchesSeen_;
        playingSong_ = std::move(queuedSong_);
        playingIndex_ = queuedIndex_;
        queuedIndex_ = kNone;
        order_->started(playingIndex_);
    }
}

void MusicSequence::update()
{
    if (!stream_)
    {
        return;
    }
    followStream();
    collectRead();
    const size_t next = order_->next();
    if (next < files_.size() && next != queuedIndex_ && !offer(next))
    {
        requestRead(next);
    }
    keepSongsFilled();
}

// the filler thread is told which songs to fill; without one (tests) they are filled here
void MusicSequence::keepSongsFilled()
{
    if (filler_)
    {
        filler_->keepFilled(playingSong_, queuedSong_);
        return;
    }
    for (SongFeed *song : {playingSong_.get(), queuedSong_.get()})
    {
        if (song)
        {
            song->fill();
        }
    }
}

void MusicSequence::setVolume(float volume)
{
    volume_ = volume;
    if (stream_)
    {
        stream_->setVolume(volume);
    }
}

void MusicSequence::pause()
{
    if (stream_)
    {
        stream_->pause();
    }
}

void MusicSequence::resume()
{
    if (stream_)
    {
        stream_->resume();
    }
}

std::string MusicSequence::nowPlaying() const
{
    if (!stream_ || !playingSong_)
    {
        return {};
    }
    const std::u8string name = files_[playingIndex_].filename().u8string();
    return std::string(name.begin(), name.end());
}

}  // namespace argon
