// Argon: FE NAME ENTRY's cursor moves (ui\keyboard_moves.hpp). Each move steps once, then again while it
// landed where there is no key, so a move can pass over empty places. The rows: 0 [Delete] + 10 keys, 1 [CAPS] + 10,
// 2 [Accents] / [Alphabet] + 10, 3 (none) + 6 keys + Space (column 7), 4 (none) + Done (column 1).
#include "ui/keyboard_moves.hpp"

#include <algorithm>

namespace menu
{
namespace
{
constexpr int kTopRow = 0, kThirdRow = 2, kSpaceRow = 3, kDoneRow = 4;
constexpr int kDoneCol = 1, kSpaceCol = 7, kLastCol = 10;
}  // namespace

bool KeyboardMoves::skipped(const KeyCursor &at) const
{
    const int code = keys.code(at.row, at.col);
    return code == 0 || (skipAccentsKey && code == KeyLayout::kAccentsKey);
}

// 0x390d0
KeyCursor KeyboardMoves::up(KeyCursor at) const
{
    do
    {
        if (at.row == kTopRow)
        {
            upFromTopRow(at);
        }
        else if (at.row == kSpaceRow)
        {
            upFromSpaceRow(at);
        }
        else if (at.row == kDoneRow)
        {
            upFromDoneRow(at);
        }
        at.row = at.row == kTopRow ? kDoneRow : at.row - 1;
    } while (skipped(at));
    return at;
}

// from a key past the first two of the top row up (round) onto Done, remembering the column
void KeyboardMoves::upFromTopRow(KeyCursor &at)
{
    if (at.col >= 2)
    {
        at.remembered = at.col;
        at.col = kDoneCol;
    }
}

// from Space up to the remembered column when it was past 6, else straight up
void KeyboardMoves::upFromSpaceRow(KeyCursor &at)
{
    if (at.col == kSpaceCol && at.remembered > 6)
    {
        at.col = at.remembered;
    }
}

// from Done up to the remembered column, at most Space's
void KeyboardMoves::upFromDoneRow(KeyCursor &at)
{
    if (at.col == kDoneCol)
    {
        at.col = std::min(at.remembered, kSpaceCol);
    }
    else if (at.col > kDoneCol)
    {
        at.col = kSpaceCol;
    }
}

// 0x39230
KeyCursor KeyboardMoves::down(KeyCursor at) const
{
    do
    {
        if (at.row == kThirdRow)
        {
            downFromThirdRow(at);
        }
        else if (at.row == kSpaceRow)
        {
            downFromSpaceRow(at);
        }
        else if (at.row == kDoneRow)
        {
            downFromDoneRow(at);
        }
        at.row = at.row == kDoneRow ? kTopRow : at.row + 1;
    } while (skipped(at));
    return at;
}

// from the keys past 6 down onto Space, remembering the column
void KeyboardMoves::downFromThirdRow(KeyCursor &at)
{
    if (at.col > 6)
    {
        at.remembered = at.col;
        at.col = kSpaceCol;
    }
}

// down onto Done: from the letters remembering the column, from Space keeping the one remembered before
void KeyboardMoves::downFromSpaceRow(KeyCursor &at)
{
    if (at.col > 0 && at.col < kSpaceCol)
    {
        at.remembered = at.col;
    }
    if (at.col > 0)
    {
        at.col = kDoneCol;
    }
}

// from Done down (round) to the remembered column, at most the last
void KeyboardMoves::downFromDoneRow(KeyCursor &at)
{
    if (at.col == kDoneCol)
    {
        at.col = std::min(at.remembered, kLastCol);
    }
}

// 0x39360: left along the row, round from the first key to the last counted one
KeyCursor KeyboardMoves::left(KeyCursor at) const
{
    do
    {
        at.col = (at.col == 0 ? keys.rowLength(at.row) : at.col) - 1;
    } while (skipped(at));
    return at;
}

// 0x393f0
KeyCursor KeyboardMoves::right(KeyCursor at) const
{
    do
    {
        at.col = at.col == keys.rowLength(at.row) - 1 ? 0 : at.col + 1;
    } while (skipped(at));
    return at;
}
}  // namespace menu
