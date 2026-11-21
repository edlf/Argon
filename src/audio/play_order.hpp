// Argon: which song a MusicSequence plays next. The sequence asks next() every frame (a changed answer
// replaces the queued song while the stream has not reached it) and reports each song the stream moves on to.
#pragma once
#include <cstddef>
#include <cstdint>

namespace argon
{

class PlayOrder
{
  public:
    virtual ~PlayOrder() = default;
    virtual void restart(size_t songs) = 0;   // a new playlist of that many songs
    virtual size_t next() const = 0;          // the song to follow the one playing (the first one before any)
    virtual void started(size_t song) = 0;    // the stream moved on to that song
    virtual void unreadable(size_t)           // that song could not be decoded
    {
    }
};

// the playlist in its order, looping
class FolderOrder : public PlayOrder
{
  public:
    void restart(size_t songs) override
    {
        songs_ = songs;
        next_ = 0;
    }
    size_t next() const override
    {
        return next_;
    }
    void started(size_t song) override
    {
        next_ = songs_ ? (song + 1) % songs_ : 0;
    }
    void unreadable(size_t song) override
    {
        started(song);   // skipped
    }

  private:
    size_t songs_ = 0, next_ = 0;
};

// the custom soundtrack in a race (CSoundtrack::OpenSong 0xdfe20 -> 0xdf150): the first song and each one after a song
// ends is rand() % the playlist's songs, the C runtime's rand (0xf96ea): independent picks, so a song can come again
// at once and the playlist never runs out
class RandomPickOrder : public PlayOrder
{
  public:
    explicit RandomPickOrder(uint32_t seed = 1) : seed_(seed)
    {
    }
    void restart(size_t songs) override
    {
        songs_ = songs;
        pick();
    }
    size_t next() const override
    {
        return next_;
    }
    void started(size_t) override
    {
        pick();
    }
    void unreadable(size_t) override
    {
        pick();
    }

  private:
    void pick()
    {
        next_ = songs_ ? size_t(nextRand()) % songs_ : 0;
    }
    unsigned nextRand()
    {
        seed_ = seed_ * 214013u + 2531011u;
        return (seed_ >> 16) & 0x7fffu;
    }

    uint32_t seed_;
    size_t songs_ = 0, next_ = 0;
};

}  // namespace argon
