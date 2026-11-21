// Argon: FE LEVELMUSIC "Edit Playlist" (vtable 0x19dee8), from FE MUSICSELECT with Custom music (ui\playlist.cpp)
#pragma once
#include "ui/fe_canvas.hpp"
#include <vector>

#include "audio/custom_music.hpp"

#include "ui/page_hosts.hpp"
#include "ui/fe_page.hpp"

class LevelMusicPage : public FePage
{
  public:
    LevelMusicPage(const PageKit &kit, CustomMusic &music, MusicStore &musicStore) : FePage(kit), music(music), musicStore(musicStore)
    {
    }
    void open();

    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    std::u16string title() const override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kLevelMusic;
    }

  private:
    CustomMusic &music;
    MusicStore &musicStore;   // the folder read, the profile written
    enum Column
    {
        kNotInPlaylist,
        kInPlaylist
    };
    static constexpr int kRowsShown = 10;
    Column column() const
    {
        return inPlaylist ? kInPlaylist : kNotInPlaylist;
    }
    std::vector<SongRef> songsOf(Column c) const;
    void switchColumn(const menu::MenuInput &in);
    void moveCursor(const menu::MenuInput &in, int count);
    bool moveSong(const SongRef &song);
    void keepCursorInColumn();
    void leave();
    static float columnX(float originX, Column c);   // the column's area's left in the window (the 640 frame at originX)
    static void drawArea(const FeCanvas &canvas, float x);
    void drawColumn(const FeCanvas &canvas, Column c, float x) const;
    void drawSongRow(const FeCanvas &canvas, float x, float y, const std::u16string &label, bool selected) const;

    bool inPlaylist = false;     // the cursor in the playlist (left) column
    int selected[2]{}, firstShown[2]{};   // per Column
};
