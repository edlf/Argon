// Argon: FE NAME ENTRY's cursor moves on its keyboard (input 0x38f10: up 0x390d0, down 0x39230, left 0x39360,
// right 0x393f0; docs\frontend\profiles.md "Profiles"), without the drawing (ui\keyboard.cpp)
#pragma once

namespace menu
{
// the keys of the page shown: 5 rows; a key's code (a character or 0x10000 + n), 0 where a row has no key
class KeyLayout
{
  public:
    static constexpr int kRows = 5;
    static constexpr int kAccentsKey = 0x10004;

    virtual int code(int row, int col) const = 0;   // 0 past the row's end
    virtual int rowLength(int row) const = 0;       // the keys counted for left / right (0xa5ec)

  protected:
    ~KeyLayout() = default;
};

struct KeyCursor
{
    int row = 4, col = 1;   // Done
    int remembered = 0;     // +0x14a30: the column left when moving onto a wide key (Space, Done)
};

class KeyboardMoves
{
  public:
    // German and French (the game's languages 1 and 2) skip the Accents key
    KeyboardMoves(const KeyLayout &layout, bool skipAccents) : keys(layout), skipAccentsKey(skipAccents)
    {
    }

    KeyCursor up(KeyCursor at) const;
    KeyCursor down(KeyCursor at) const;
    KeyCursor left(KeyCursor at) const;
    KeyCursor right(KeyCursor at) const;

  private:
    bool skipped(const KeyCursor &at) const;   // no key there (or the skipped Accents key): the move goes on
    static void upFromTopRow(KeyCursor &at);
    static void upFromSpaceRow(KeyCursor &at);
    static void upFromDoneRow(KeyCursor &at);
    static void downFromThirdRow(KeyCursor &at);
    static void downFromSpaceRow(KeyCursor &at);
    static void downFromDoneRow(KeyCursor &at);

    const KeyLayout &keys;
    bool skipAccentsKey;
};
}  // namespace menu
