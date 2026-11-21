// Argon: the on-screen keyboard, FE NAME ENTRY (vtable 0x1a2478: build 0x37c70 / keys 0x37d90, enter 0x39630,
// update 0x39480, input 0x38f10, draw 0x3ab30, key press 0x39ca0, done 0x3a110): a profile's name, or a cheat code from
// FE CHEATMENU; docs\frontend\profiles.md "Profiles". Profiles themselves: app_profile.cpp.
#include "audio/audio.hpp"
#include "files/string_table.hpp"
#include "hud/hud_painter.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_id.hpp"
#include "ui/fe_shell.hpp"
#include "files/utf.hpp"
#include "ui/keyboard_moves.hpp"
#include "ui/menu_common.hpp"

using namespace menu::fe;

namespace
{
// the special keys' codes (0x10000 + n), 0: no key
enum KeyCode
{
    kNoKey = 0,
    kDelete = 0x10000,
    kCaps = 0x10002,
    kAlphabet = 0x10003,
    kAccents = 0x10004,
    kDone = 0x10005
};
constexpr int kKeyRows = 5, kDoneRow = 4, kDoneCol = 1;

// FE NAME ENTRY keyboard (0x37d90): two pages of 5 rows; code (character, or a KeyCode), slot width.
// Letters page: [Delete] 1..0 / [CAPS] A..J / [Accents] K..T / [ ] U..Z Space / [ ] Done.
// Accents page: [Delete] Ã Å Æ Ø / [CAPS] À..Ì / [Alphabet] Í..ß / [ ] Ù..Ÿ Space / [ ] Done.
struct Key
{
    int code, slot;
};
using KeyRows = std::vector<std::vector<Key>>;

const KeyRows &keyRows(int page)
{
    static const KeyRows letters = {
        {{kDelete, 110}, {'1', 34}, {'2', 34}, {'3', 34}, {'4', 34}, {'5', 34}, {'6', 34}, {'7', 34}, {'8', 34}, {'9', 34}, {'0', 34}},
        {{kCaps, 110}, {'A', 34}, {'B', 34}, {'C', 34}, {'D', 34}, {'E', 34}, {'F', 34}, {'G', 34}, {'H', 34}, {'I', 34}, {'J', 34}},
        {{kAccents, 110}, {'K', 34}, {'L', 34}, {'M', 34}, {'N', 34}, {'O', 34}, {'P', 34}, {'Q', 34}, {'R', 34}, {'S', 34}, {'T', 34}},
        {{kNoKey, 110}, {'U', 34}, {'V', 34}, {'W', 34}, {'X', 34}, {'Y', 34}, {'Z', 34}, {' ', 136}},
        {{kNoKey, 110}, {kDone, 204}}};
    static const KeyRows accents = {
        {{kDelete, 110}, {0xc3, 34}, {0xc5, 34}, {0xc6, 34}, {0xd8, 34}},
        {{kCaps, 110}, {0xc0, 34}, {0xc1, 34}, {0xc2, 34}, {0xc4, 34}, {0xc7, 34}, {0xc8, 34}, {0xc9, 34}, {0xca, 34}, {0xcb, 34}, {0xcc, 34}},
        {{kAlphabet, 110}, {0xcd, 34}, {0xce, 34}, {0xcf, 34}, {0xd1, 34}, {0xd2, 34}, {0xd3, 34}, {0xd4, 34}, {0xd5, 34}, {0xd6, 34}, {0xdf, 34}},
        {{kNoKey, 110}, {0xd9, 34}, {0xda, 34}, {0xdb, 34}, {0xdc, 34}, {0xdd, 34}, {0x178, 34}, {' ', 136}},
        {{kNoKey, 110}, {kDone, 204}}};
    return page ? accents : letters;
}

// the key's x (0x389d0: from 320 - 250, + slot + 6, + 4 more after the first key) and panel width (0x37a90)
float keyX(const std::vector<Key> &row, size_t key)
{
    float x = 70.f;
    for (size_t k = 0; k < key; ++k)
    {
        x += float(row[k].slot) + 6.f + (k == 0 ? 4.f : 0.f);
    }
    return x;
}

float keyWidth(int code)
{
    if (code == ' ')
    {
        return 140.f;
    }
    if (code == kDone)
    {
        return 380.f;
    }
    if (code >= kDelete)
    {
        return 85.f;
    }
    return 16.f;
}

// the page's keys as the cursor moves see them (the row lengths: 0xa5ec)
class PageKeys : public menu::KeyLayout
{
  public:
    explicit PageKeys(int page) : rows(keyRows(page))
    {
    }
    int code(int row, int col) const override
    {
        const auto &keys = rows[size_t(row)];
        return col >= 0 && col < int(keys.size()) ? keys[size_t(col)].code : kNoKey;
    }
    int rowLength(int row) const override
    {
        return int(rows[size_t(row)].size());
    }

  private:
    const KeyRows &rows;
};

// the game's languages 1 (German) and 2 (French) skip the Accents key (0x12807: 0 Japanese, 3 English, 4 Italian)
bool skipsAccentsKey(const ViewerSettings &settings)
{
    const Language language = settings.language();
    return language == Language::German || language == Language::French;
}

char16_t caseOf(char16_t character, bool upper)
{   // 0x39ca0: letters A-Z / À-Ý (and ÿ / Ÿ) follow CAPS
    if (upper)
    {
        if ((character >= 'a' && character <= 'z') || (character >= 0xe0 && character <= 0xfd))
        {
            return char16_t(character - 0x20);
        }
        if (character == 0xff)
        {
            return 0x178;
        }
    }
    else
    {
        if ((character >= 'A' && character <= 'Z') || (character >= 0xc0 && character <= 0xdd))
        {
            return char16_t(character + 0x20);
        }
        if (character == 0x178)
        {
            return 0xff;
        }
    }
    return character;
}

// typing on the PC keyboard: Backspace deletes (not Back), the X / Y / Space keys type (not the pad buttons)
menu::MenuInput withPcKeyboard(menu::MenuInput in)
{
    if (ImGui::GetIO().WantCaptureKeyboard)
    {
        return in;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Backspace))
    {
        in.b = false;
        in.x = true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_X, false))
    {
        in.x = false;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Y, false))
    {
        in.y = false;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Space, false))
    {
        in.a = false;
    }
    return in;
}
}  // namespace

// enter (0x39630): title fe 0x2d0 "Enter Name", hints Back / Select, Backspace - [BUTX] / [BUTY] - Done, ticker
// fe 0x2dc; the name starts as "PLAYER" (fe 0x4b0) + the first number not taken, replaced by the first key
// (0x39fa0); the cursor on Done (row 4, key 1), letters page, lower case
void NameEntryPage::openForName()
{
    profiles.load();
    const std::u16string base = strings.frontEnd(0x4b0, "PLAYER");
    int number = 1;
    for (const auto &profile : profiles.all())
    {
        if (profile.name.compare(0, base.size(), base) == 0)
        {
            number = std::max(number, std::atoi(utf8(profile.name.substr(base.size())).c_str()) + 1);
        }
    }
    text = base + argon::widenLatin1(std::to_string(number).c_str());
    fresh = true;
    cheatCode = false;
    resetKeyboard();
    shell.goTo(menu::PageNameEntry);
}

// FE CHEATMENU case 7 (0x1b030)
void NameEntryPage::openForCheatCode()
{
    text.clear();
    fresh = false;
    cheatCode = true;
    resetKeyboard();
    shell.goTo(menu::PageNameEntry);
}

void NameEntryPage::resetKeyboard()
{
    keyPage = 0;
    keyRow = kDoneRow;
    keyCol = kDoneCol;
    rememberedCol = 0;
    caps = false;
}

std::u16string NameEntryPage::title() const
{
    return cheatCode ? strings.frontEnd(0x304, "Enter Code") : strings.frontEnd(0x2d0, "Enter Name");
}

std::optional<std::u16string> NameEntryPage::ticker() const
{
    return strings.frontEnd(0x2dc, "Use the virtual keyboard to enter your profile name.");
}

bool NameEntryPage::keyExists(int row, int col) const
{
    const auto &keys = keyRows(keyPage)[size_t(row)];
    return col >= 0 && col < int(keys.size()) && keys[size_t(col)].code != kNoKey;
}

int NameEntryPage::keyCode() const
{
    return keyRows(keyPage)[size_t(keyRow)][size_t(keyCol)].code;
}

// input (0x38f10): A presses the key under the cursor, X deletes, Y = Done, B back to FE CHOOSESAVE; the d-pad moves
// (one direction a frame: down, up, left, right; ui\keyboard_moves.cpp). The PC keyboard also types directly.
void NameEntryPage::input(const menu::MenuInput &pad)
{
    const menu::MenuInput in = withPcKeyboard(pad);
    if (!keyExists(keyRow, keyCol))
    {
        keyRow = kDoneRow;
        keyCol = kDoneCol;
    }
    moveCursor(in);
    if (in.a)
    {
        play(menu::UiEvent::Pick);
        pressKey(keyCode());
    }
    if (in.x)
    {
        play(menu::UiEvent::Pick);
        pressKey(kDelete);
    }
    if (in.y)
    {
        play(menu::UiEvent::Pick);
        done();
        return;
    }
    if (in.b)
    {
        play(menu::UiEvent::Back);
        back();
        return;
    }
    typePcKeyboard();
}

// every d-pad press plays the cursor sound, a move or not (0x39031 .. 0x390ae)
void NameEntryPage::moveCursor(const menu::MenuInput &in)
{
    const PageKeys keys(keyPage);
    const menu::KeyboardMoves moves(keys, skipsAccentsKey(settings));
    menu::KeyCursor at{keyRow, keyCol, rememberedCol};
    if (in.up || in.down || in.left || in.right)
    {
        play(menu::UiEvent::Cursor);
    }
    if (in.down)
    {
        at = moves.down(at);
    }
    else if (in.up)
    {
        at = moves.up(at);
    }
    else if (in.left)
    {
        at = moves.left(at);
    }
    else if (in.right)
    {
        at = moves.right(at);
    }
    keyRow = at.row;
    keyCol = at.col;
    rememberedCol = at.remembered;
}

void NameEntryPage::pressKey(int code)
{
    switch (code)
    {
    case kDelete:   // 0x3aac0
        if (!text.empty())
        {
            text.pop_back();
        }
        fresh = false;
        break;
    case kCaps:
        caps = !caps;
        break;
    case kAccents:
        keyPage = 1;
        break;
    case kAlphabet:
        keyPage = 0;
        break;
    case kDone:
        done();
        break;
    case kNoKey:
        break;
    default:
        type(caseOf(char16_t(code), caps));
    }
}

void NameEntryPage::type(char16_t character)
{
    if (fresh)
    {
        text.clear();
        fresh = false;
    }
    if (text.size() < kMaxLength)
    {
        text += character;
    }
}

// the PC keyboard: typed characters (Backspace deletes, as X above)
void NameEntryPage::typePcKeyboard()
{
    const ImGuiIO &io = ImGui::GetIO();
    for (int i = 0; i < io.InputQueueCharacters.Size; ++i)
    {
        ImWchar ch = io.InputQueueCharacters[i];
        if (ch >= 0x20 && ch != 0x7f)
        {
            type(char16_t(ch));
        }
    }
}

void NameEntryPage::back()
{
    if (!cheatCode)
    {
        shell.chooseSave().open();
        return;
    }
    cheatCode = false;
    shell.cheatMenu().open();
}

// done (0x3a110): an empty name -> fe 0x2da, only spaces -> 0x2dd, taken -> 0x2db; else the new profile and FE MAIN
void NameEntryPage::done()
{
    if (cheatCode)
    {
        cheatCode = false;
        play(menu::UiEvent::Forward);   // 0x3a150
        cheatCodes.cheatCodeDone(text);
        return;
    }
    if (text.empty())
    {
        shell.messageBox().open(strings.frontEnd(0x2da, "You must enter a name."), MessageBoxPage::Kind::Info);
        return;
    }
    if (text.find_first_not_of(u' ') == std::u16string::npos)
    {
        shell.messageBox().open(strings.frontEnd(0x2dd, "Your profile name must contain characters other than just space."), MessageBoxPage::Kind::Info);
        return;
    }
    profiles.load();
    for (const auto &profile : profiles.all())
    {
        if (profile.name == text)
        {
            shell.messageBox().open(strings.frontEnd(0x2db, "This profile name already exists. Please choose a different name."),
                                      MessageBoxPage::Kind::Info);
            return;
        }
    }
    finishName();
}

void NameEntryPage::finishName()
{
    if (!profileKeeper.createProfile(text))
    {
        shell.messageBox().open(strings.frontEnd(0x1f5, "Could not save the profile."), MessageBoxPage::Kind::Info);
        return;
    }
    play(menu::UiEvent::Forward);   // 0x3a654
    door.openMenu();
}

// draw 0x3ab30: the keys (173, 150) 420 x 190, the special keys (50, 150) 123 x 190, the name (50, 100) 543 x 45.
// The frames and the name stay centred with the keys (no margin shift on a wide window, which split them apart)
void NameEntryPage::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    MenuKit::drawGradientFrame(canvas, {originX + 173.f, 150.f, 420.f, 190.f});
    MenuKit::drawGradientFrame(canvas, {originX + 50.f, 150.f, 123.f, 190.f});
    MenuKit::drawGradientFrame(canvas, {originX + 50.f, 100.f, 543.f, 45.f}, FrameShade::Red);
    // the name (build 0x37c70: at (590, 105), orange) with the "_" cursor; its font / alignment are not decoded
    // (font 1 right-aligned here)
    canvas.text({originX + 590.f, 105.f}, text + u"_", {1, kOrange, 1, kFe, kFe});
    drawKeys(canvas);
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), strings.frontEnd(0x32, "Select - [BUTA]")});
    menus.drawHints(canvas, {strings.frontEnd(0x3e, "Backspace - [BUTX]"), strings.frontEnd(0x3f, "[BUTY] - Done"), kHintScale, kHintY + 24.f});
}

std::u16string NameEntryPage::keyLabel(int code) const
{
    switch (code)
    {
    case kDelete:
        return strings.frontEnd(0x2d2, "Delete");
    case kCaps:
        return strings.frontEnd(0x2d4, "CAPS");
    case kAlphabet:
        return strings.frontEnd(0x2d5, "Alphabet");
    case kAccents:
        return strings.frontEnd(0x2d7, "Accents");
    case kDone:
        return strings.frontEnd(0x2d8, "Done");
    case ' ':
        return strings.frontEnd(0x2d1, "Space");
    default:
        return std::u16string(1, caseOf(char16_t(code), caps));
    }
}

// keys (0x389d0 / 0x37a90): rows at y 160 + 36 r, panels as the menu rows' around the key's label
void NameEntryPage::drawKeys(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const KeyRows &rows = keyRows(keyPage);
    const float h = 20.f;
    for (int r = 0; r < kKeyRows; ++r)
    {
        for (size_t c = 0; c < rows[size_t(r)].size(); ++c)
        {
            const int code = rows[size_t(r)][c].code;
            if (code == kNoKey)
            {
                continue;
            }
            const float x = keyX(rows[size_t(r)], c), y = 160.f + 36.f * float(r), w = keyWidth(code);
            const bool sel = r == keyRow && int(c) == keyCol;
            canvas.panel({originX + x - h * 0.5f, y, w + h, h}, {sel, true, 0xffffff00u, sel, 0xffa08040u, 0xffa08040u});
            uint32_t colour = sel ? menu::blinkColour(kText, MenuKit::menuBlink()) : kText;
            canvas.text({originX + x + w * 0.5f, y + (h - menus.fontLine(0) * kFe) * 0.5f}, keyLabel(code), {0, colour, 2, kFe, kFe});
        }
    }
}
