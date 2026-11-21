// Argon: FE LEVELMUSIC "Edit Playlist" (vtable 0x19dee8: build 0x26af0, enter 0x27930, input 0x27380,
// update 0x27650, lists 0x27b80 / 0x27e10, move 0x27fe0, preview 0x28550). The music itself: app\app_music.cpp.
#include "ui/playlist.hpp"

#include "audio/audio.hpp"
#include "files/string_table.hpp"
#include "hud/hud_painter.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_id.hpp"
#include "ui/fe_shell.hpp"
#include "ui/menu_common.hpp"

using namespace menu::fe;

namespace
{
// the lists' 250 x 200 areas at y 160: the playlist from the left margin + 10, the other songs ending there from the
// right (630 - 62 - 250, 0x1a36c8 = 630)
constexpr float kLeftX = 62.f, kRightX = 630.f - 62.f - 250.f, kAreaY = 160.f, kAreaW = 250.f, kAreaH = 200.f;
constexpr uint32_t kAreaOutline = 0xff000000u, kAreaFill = 0xa0204060u;
constexpr int kEditPlaylistRow = 1;                              // FE MUSICSELECT's row, where B leaves the cursor
}  // namespace

// enter (0x27930): hints Back / Select Song (0x45), Select Column (0x47) / Preview Song (0x46); the cursor starts in
// the songs not in the playlist (right), or in the playlist when every song is in it
void LevelMusicPage::open()
{
    musicStore.scanCustomMusic();
    inPlaylist = music.songsNotInPlaylist().empty();
    selected[0] = selected[1] = firstShown[0] = firstShown[1] = 0;
    shell.goTo(menu::PageLevelMusic);
}

std::u16string LevelMusicPage::title() const
{
    return strings.frontEnd(0x21c, "Edit Playlist");
}

std::vector<SongRef> LevelMusicPage::songsOf(Column column) const
{
    return column == kInPlaylist ? music.playlist() : music.songsNotInPlaylist();
}

// input (0x27380): up / down in the column (10 rows shown, scrolling), left / right the column (only to a column
// with songs), A moves the song to the other list (into the first free of 32 playlist slots; 0x27fe0), Y previews
// it (0x28550), B back to FE MUSICSELECT (the front-end music again, 0x27650)
void LevelMusicPage::input(const menu::MenuInput &in)
{
    if (in.b)
    {
        leave();
        return;
    }
    switchColumn(in);
    std::vector<SongRef> songs = songsOf(column());
    moveCursor(in, int(songs.size()));
    if (songs.empty())
    {
        return;
    }
    const SongRef song = songs[size_t(selected[column()])];
    if (in.y)
    {
        audio.preview(music.file(song));
    }
    if (!in.a || !moveSong(song))
    {
        return;
    }
    keepCursorInColumn();
    musicStore.storeMusic();
}

void LevelMusicPage::leave()
{
    play(menu::UiEvent::LeftBack);
    audio.stopPreview();
    musicStore.storeMusic();
    shell.musicSelect().openAt(kEditPlaylistRow);
}

void LevelMusicPage::switchColumn(const menu::MenuInput &in)
{
    if (in.left && !inPlaylist && !music.playlist().empty())
    {
        inPlaylist = true;
    }
    else if (in.right && inPlaylist && !music.songsNotInPlaylist().empty())
    {
        inPlaylist = false;
    }
}

void LevelMusicPage::moveCursor(const menu::MenuInput &in, int count)
{
    int &sel = selected[column()];
    int &top = firstShown[column()];
    sel = std::clamp(sel, 0, std::max(0, count - 1));
    if (in.down && sel < count - 1)
    {
        ++sel;
        if (sel - top > kRowsShown - 2)
        {
            ++top;
        }
    }
    if (in.up && sel > 0)
    {
        --sel;
        top = std::min(top, sel);
    }
}

// the song to the other list: out of the playlist, or into it while a slot is free
bool LevelMusicPage::moveSong(const SongRef &song)
{
    if (inPlaylist)
    {
        music.removeFromPlaylist(size_t(selected[kInPlaylist]));
    }
    else if (!music.playlistFull())
    {
        music.addToPlaylist(song);
    }
    else
    {
        return false;
    }
    return true;
}

// the column emptied: the cursor moves to the other one; else it stays on the songs left
void LevelMusicPage::keepCursorInColumn()
{
    int left = int(songsOf(column()).size());
    if (left == 0)
    {
        inPlaylist = !inPlaylist;
        return;
    }
    int &sel = selected[column()];
    sel = std::min(sel, left - 1);
    firstShown[column()] = std::clamp(firstShown[column()], 0, std::max(0, sel));
}

// build 0x26af0: headers "Songs in Playlist" (fe 0x26c) at (62, 130) and "Songs not in Playlist" (0x26d) at
// (630 - 62 - 250, 130); the lists in their areas, rows y 160 + 20 i (10 shown)
void LevelMusicPage::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    for (Column column : {kInPlaylist, kNotInPlaylist})
    {
        drawArea(canvas, columnX(originX, column));
    }
    canvas.text({columnX(originX, kInPlaylist), 130.f}, strings.frontEnd(0x26c, "Songs in Playlist"), {0, kText, 0, kFe, kFe});
    canvas.text({columnX(originX, kNotInPlaylist), 130.f}, strings.frontEnd(0x26d, "Songs not in Playlist"), {0, kText, 0, kFe, kFe});
    drawColumn(canvas, kNotInPlaylist, columnX(originX, kNotInPlaylist));
    drawColumn(canvas, kInPlaylist, columnX(originX, kInPlaylist));
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), strings.frontEnd(0x45, "Select Song - [BUTA]")});
    menus.drawHints(
        canvas,
        {strings.frontEnd(0x47, "[BUTLFIX][BUTRFIX] - Select Column"), strings.frontEnd(0x46, "Preview Song - [BUTY]"), kHintScale, kHintY + 24.f});
}

float LevelMusicPage::columnX(float originX, Column column)
{
    return column == kInPlaylist ? atLeft(originX, kLeftX) : atRight(originX, kRightX);
}

// 0x27190 (vtable +0x2c, under the page's elements): a 1 px black outline round the area (0xab110), filled 1 px inside
// with translucent blue 0xa0204060 (0xaad40)
void LevelMusicPage::drawArea(const FeCanvas &canvas, float x)
{
    ImDrawList *drawList = canvas.drawList();
    const float pixelsPerUnit = canvas.scale();
    drawList->AddRect(ImVec2(x * pixelsPerUnit, kAreaY * pixelsPerUnit), ImVec2((x + kAreaW) * pixelsPerUnit, (kAreaY + kAreaH) * pixelsPerUnit), imColour(kAreaOutline), 0.f, 0, std::max(1.f, pixelsPerUnit));
    drawList->AddRectFilled(ImVec2((x + 1.f) * pixelsPerUnit, (kAreaY + 1.f) * pixelsPerUnit), ImVec2((x + kAreaW - 1.f) * pixelsPerUnit, (kAreaY + kAreaH - 1.f) * pixelsPerUnit), imColour(kAreaFill));
}

void LevelMusicPage::drawColumn(const FeCanvas &canvas, Column columnId, float x) const
{
    ImDrawList *drawList = canvas.drawList();
    const float pixelsPerUnit = canvas.scale();
    const std::vector<SongRef> songs = songsOf(columnId);
    drawList->PushClipRect(ImVec2((x - 5.f) * pixelsPerUnit, (kAreaY - 2.f) * pixelsPerUnit), ImVec2((x + kAreaW - 5.f) * pixelsPerUnit, (kAreaY + kAreaH - 2.f) * pixelsPerUnit), true);
    for (int i = 0; i < kRowsShown && firstShown[columnId] + i < int(songs.size()); ++i)
    {
        int row = firstShown[columnId] + i;
        drawSongRow(canvas, x, kAreaY + 20.f * float(i), music.label(songs[size_t(row)]), columnId == column() && row == selected[columnId]);
    }
    drawList->PopClipRect();
}

// the cursor's panel: half the menu rows' margin, at most 230 wide (the column's clip)
void LevelMusicPage::drawSongRow(const FeCanvas &canvas, float x, float y, const std::u16string &label, bool isSelected) const
{
    const float line = menus.fontLine(0);
    if (isSelected)
    {
        float w = std::min(painter.textWidth(label, 0, kFe), 230.f);
        canvas.panel({x - line * 0.25f, y, w + line * 0.5f, line}, {true, true, 0xffffff00u, true, 0xffa08040u, 0xffa08040u});
    }
    uint32_t colour = isSelected ? menu::blinkColour(kText, MenuKit::menuBlink()) : kText;
    canvas.text({x, y + (line - line * kFe) * 0.5f}, label, {0, colour, 0, kFe, kFe});
}
