// Argon: custom music in a race: the playlist's songs (audio\custom_music.hpp). The folder read and the profile's
// Music Source and playlist: app\front_end_stores.hpp (CustomMusicStore); the page: ui\playlist.cpp.

#include "app/app.hpp"

// in a race: Custom with a playlist plays random picks of its songs (fe 0x3e9 "Play random music from your ripped
// tracks."): each song rand() % the playlist's count (0xdf150, from CSoundtrack::OpenSong 0xdfe20 at the race's load
// and each song's end), repeats allowed. Else the track's music
void App::applyRaceMusic()
{
    if (!customMusic.customChosen() || customMusic.playlist().empty())
    {
        return;
    }
    audio.setMusicList(customMusic.playlistFiles());
}
