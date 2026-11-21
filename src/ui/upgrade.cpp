// Argon: UPGRADEMENU (upgrademenu.cpp; build 0x9dfb0, messages / update 0x9e7d0, draw 0x9f040, row text
// 0x9fd90, buy 0x9fe40, leave 0x9ffc0). Opened from the tournament's WINMENU / LOSEMENU item "Upgrade Menu". Per
// character three levels 0..7 (0x1bb0ec + character x 0xb0 + 0 Weapon Strength / 1 Turbo / 2 Shield) bought from the
// character's cash (0x1bb0e8); prices 0x1bc518 = 50000 + 75000 x level (filled at start-up by 0x82a70).
// docs\frontend\tournament.md "UPGRADEMENU".
#include "ui/upgrade.hpp"

#include "app/weapons.hpp"
#include "audio/audio.hpp"
#include "files/string_table.hpp"
#include "hud/hud_painter.hpp"
#include "logic/game_progress.hpp"
#include "logic/upgrade_rules.hpp"
#include "render/pause_background.hpp"
#include "saves/viewer_settings.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_hosts.hpp"

using namespace menu::race;

namespace
{
constexpr float kPanelX = 296.f, kPanelW = 272.f;   // 630 - 62 - 272
constexpr float kInsufficientFundsWrap = 262.f;   // 0x9f788: the text's wrap width
const float kRowY[5] = {140.f, 175.f, 210.f, 270.f, 305.f};
const float kPanelY[3] = {140.f, 240.f, 310.f}, kPanelH[3] = {90.f, 60.f, 60.f}, kJoinY[3] = {175.f, 255.f, 335.f};
constexpr float kIconX = 430.f, kIconY = 150.f;   // 0x9f45e: panel x + 272 (0x1a390c) - 138 (0x1a3908)

std::u16string withNumber(const std::u16string &text, int v)
{
    return text + argon::widenLatin1(std::to_string(v).c_str());
}

float rowScale(int row)
{
    return row < 3 ? 0.6f : 0.7f;
}
}  // namespace

// "UPGRADEMENU ON" (0x9e7d0): the cursor on Continue (+0x608 = 3); Clear Changes restores the levels and cash kept in
// 0x1bc87c.. (the viewer keeps them when the menu opens); the blurred background is made again from the current frame
void UpgradeMenu::open()
{
    const int character = upgradeHost.upgradeChar();
    for (int k = 0; k < 3; ++k)
    {
        snapshotLevels[k] = sources.progress.upgrades[character][k];
    }
    snapshotCash = sources.progress.cash[character];
    selected = kContinue;
    sources.background.blurAgain();
}

int UpgradeMenu::level(int upgrade) const
{
    return sources.progress.upgrades[upgradeHost.upgradeChar()][upgrade];
}

bool UpgradeMenu::affordable(int upgrade) const
{
    return level(upgrade) < argon::upgrades::kMaxLevel && argon::upgrades::priceOfLevel(level(upgrade)) <= sources.progress.cash[upgradeHost.upgradeChar()];
}

// input (0x9e7d0, messages 5 / 6 / 9 only; B is not passed on): up / down through the five rows, wrapping; A on a row
// buys the next level, on Continue leaves (0x9e8f0: a second of "Saving Race", the music dipping, then WINMENU /
// LOSEMENU again, 0x9ffc0, on Upgrade Menu), on Clear Changes restores the snapshot
void UpgradeMenu::input(const menu::MenuInput &in)
{
    if (stepLeaving())
    {
        return;
    }
    if (MenuKit::stepCursor(selected, kRowCount, in))
    {
        audio.playUi(2);
    }
    if (!in.a)
    {
        return;
    }
    if (selected < kContinue)
    {
        buy(selected);
        return;
    }
    audio.playUi(0);
    if (selected == kClearChanges)
    {
        clearChanges();
    }
    else
    {
        startLeaving(argon::MusicFadeShape::Dip);
    }
}

void UpgradeMenu::leave()
{
    sources.pages.closeUpgrade();
}

// 0x9fe40: level < 7 and price <= cash (sound 0x18, else 0x19)
void UpgradeMenu::buy(int upgrade)
{
    if (!affordable(upgrade))
    {
        audio.playUi(1);
        return;
    }
    const int character = upgradeHost.upgradeChar();
    sources.progress.cash[character] -= argon::upgrades::priceOfLevel(sources.progress.upgrades[character][upgrade]);
    ++sources.progress.upgrades[character][upgrade];
    sources.progressStore.storeProgress();
    upgradeHost.applyUpgradeCaps();
    audio.playUi(0);
}

void UpgradeMenu::clearChanges()
{
    const int character = upgradeHost.upgradeChar();
    for (int k = 0; k < 3; ++k)
    {
        sources.progress.upgrades[character][k] = snapshotLevels[k];
    }
    sources.progress.cash[character] = snapshotCash;
    sources.progressStore.storeProgress();
    upgradeHost.applyUpgradeCaps();
}

// draw (0x9f040, over the paused race background 0x7e5a0): title "Upgrade" (0x3fe) font 1 right-aligned at (568, 36);
// "Bank Balance:%d" (0x51b) yellow centred at (320, 103); the rows left, a panel per upgrade right; the menu bars and
// logo (0xc2870) as the other menus
void UpgradeMenu::draw(const FeCanvas &canvas)
{
    ImDrawList *drawList = canvas.drawList();
    const float originX = canvas.originX();
    sources.background.draw(drawList);
    host.drawMenuBars(canvas, 640.f + 2.f * originX);
    canvas.text({atRight(originX, kRight), 36.f}, strings.race(0x3fe, "Upgrade"), {1, kText, 1, kFe, kFe});
    canvas.text(
        {originX + 320.f, 103.f}, withNumber(strings.race(0x51b, "Bank Balance:"), sources.progress.cash[upgradeHost.upgradeChar()]), {1, kYellow, 2, kFe, kFe});
    float rowRight[3]{};
    drawRows(canvas, rowRight);
    for (int i = 0; i < 3; ++i)
    {
        drawUpgradePanel(canvas, i, rowRight[i]);
    }
    const std::u16string hint = leaving() ? std::u16string() : strings.race(0x408, "Select - [BUTA]");
    menus.drawHints(canvas, {savingText(), hint, 1.f});   // leaving: +0xa2c shown, +0x1674 / +0x20c0 hidden (0x9ee1e)
}

// left the rows "Upgrade Weapon / Turbo / Shield" (0x44c..) at (62, 140 + 35 n) x 0.6, Continue (0x3ed) at (62, 270)
// and Clear Changes (0x4c8) at (62, 305) x 0.7, grey 0xff555555 unless affordable (0xff85dde5); every row has its panel
// (build 0x9dfb0: element flag 2), the selected one highlighted and filled. rowRight: the upgrade rows' right ends
void UpgradeMenu::drawRows(const FeCanvas &canvas, float rowRight[3]) const
{
    const float originX = canvas.originX();
    static const int ids[kRowCount] = {0x44c, 0x44d, 0x44e, 0x3ed, 0x4c8};
    static const char *fallbacks[kRowCount] = {"Upgrade Weapon", "Upgrade Turbo", "Upgrade Shield", "Continue", "Clear Changes"};
    for (int i = 0; i < kRowCount; ++i)
    {
        const float scale = rowScale(i) * kFe;
        const std::u16string text = strings.race(ids[i], fallbacks[i]);
        if (i < kContinue)
        {
            rowRight[i] = kRowX + painter.textWidth(text, 1, scale);
        }
        const uint32_t colour = i >= kContinue || affordable(i) ? kText : kGrey;
        menus.drawMenuRow(canvas, {atLeft(originX, kRowX), kRowY[i]}, text, {1, scale, menus.fontLine(1) * rowScale(i), i == selected, colour});
    }
}

// a panel per upgrade at x 296, 272 wide: (140, 90), (240, 60), (310, 60), fill 0x80404080, highlighted on its row;
// in the game's order (0x9f6c5..0x9f836): the weapon / stat details, then, too dear, an opaque panel 0xff404080 over
// them with "Insufficient Funds" (0x516) centred on it (flags 0x112), then "Upgrade Cost: %d" (0x4c6) yellow at
// (306, 142 / 242 / 312) x 0.8, or "Maximum" (0x4c7) font 1 x 0.7
void UpgradeMenu::drawUpgradePanel(const FeCanvas &canvas, int upgrade, float rowRight) const
{
    const float originX = canvas.originX();
    const bool sel = upgrade == selected;
    if (sel)
    {
        drawJoinLine(canvas, upgrade, rowRight);
    }
    const float x = atRight(originX, kPanelX);
    canvas.panel({x, kPanelY[upgrade], kPanelW, kPanelH[upgrade]}, {sel, sel, kYellow, true, 0x80404080u, 0x80404080u});
    if (upgrade == kWeapon)
    {
        drawWeaponPanel(canvas);
    }
    else
    {
        drawStatPanel(canvas, upgrade);
    }
    if (level(upgrade) < argon::upgrades::kMaxLevel && !affordable(upgrade))
    {
        drawInsufficientFunds(canvas, x, upgrade);
    }
    drawCost(canvas, x + 10.f, upgrade);
}

// 0x9f771: the opaque panel hides the details under it
void UpgradeMenu::drawInsufficientFunds(const FeCanvas &canvas, float x, int upgrade) const
{
    const bool sel = upgrade == selected;
    const float y = kPanelY[upgrade], h = kPanelH[upgrade];
    canvas.panel({x, y, kPanelW, h}, {sel, false, kYellow, true, 0xff404080u, 0xff404080u});
    // 0xb1230: font 0, a wrap width of 262, the lines centred as a block on the panel (flags 0x112)
    const std::vector<std::u16string> lines = menus.wrapText(strings.race(0x516, "Insufficient Funds"), 0, kFe, kInsufficientFundsWrap);
    const float lineH = menus.fontLine(0) * kFe;
    float lineY = y + (h - lineH * float(lines.size())) * 0.5f;
    for (const std::u16string &line : lines)
    {
        canvas.text({x + kPanelW * 0.5f, lineY}, line, {0, 0xffffffffu, 2, kFe, kFe});
        lineY += lineH;
    }
}

void UpgradeMenu::drawCost(const FeCanvas &canvas, float textX, int upgrade) const
{
    const int levelNow = level(upgrade);
    if (levelNow >= argon::upgrades::kMaxLevel)
    {
        canvas.text({textX, kPanelY[upgrade]}, strings.race(0x4c7, "Maximum"), {1, kYellow, 0, 0.7f * kFe, 0.7f * kFe});
        return;
    }
    const std::u16string cost = withNumber(strings.race(0x4c6, "Upgrade Cost:") + u" ", argon::upgrades::priceOfLevel(levelNow));
    canvas.text({textX, kPanelY[upgrade] + 2.f}, cost, {0, kYellow, 0, 0.8f * kFe, 0.8f * kFe});
}

// the line from the selected row to its panel (0xaaca0, yellow)
void UpgradeMenu::drawJoinLine(const FeCanvas &canvas, int upgrade, float rowRight) const
{
    ImDrawList *drawList = canvas.drawList();
    const float pixelsPerUnit = canvas.scale();
    const float originX = canvas.originX();
    const float x0 = atLeft(originX, rowRight) + 14.f, y0 = kRowY[upgrade] + 14.f, x1 = atRight(originX, kPanelX), xm = (x1 - x0) * 0.5f + x0;
    const float y1 = kJoinY[upgrade];
    const ImU32 yellow = IM_COL32(255, 255, 0, 255);
    drawList->AddLine(ImVec2(x0 * pixelsPerUnit, y0 * pixelsPerUnit), ImVec2(xm * pixelsPerUnit, y0 * pixelsPerUnit), yellow, pixelsPerUnit);
    drawList->AddLine(ImVec2(xm * pixelsPerUnit, y0 * pixelsPerUnit), ImVec2(xm * pixelsPerUnit, y1 * pixelsPerUnit), yellow, pixelsPerUnit);
    drawList->AddLine(ImVec2(xm * pixelsPerUnit, y1 * pixelsPerUnit), ImVec2(x1 * pixelsPerUnit, y1 * pixelsPerUnit), yellow, pixelsPerUnit);
}

// Weapon: the icons, "Next upgrade:" (0x422) at (306, 198; 194 when the language 0x5293b0 is 2, Japanese) x 0.8 and the
// next weapon's name at (306, 210): infotext character x 10 + level + 3 (level 6: the special)
void UpgradeMenu::drawWeaponPanel(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const int levelNow = level(kWeapon);
    drawWeaponIcons(canvas, levelNow);
    if (levelNow >= argon::upgrades::kMaxLevel)
    {
        return;
    }
    const float x = atRight(originX, kPanelX) + 10.f, nextY = sources.viewerSettings.language() == Language::Japanese ? 194.f : 198.f;
    canvas.text({x, nextY}, strings.race(0x422, "Next upgrade:"), {0, 0xffffffffu, 0, 0.8f * kFe, 0.8f * kFe});
    canvas.text({x, 210.f}, strings.info(upgradeHost.upgradeChar() * 10 + levelNow + 3, ""), {0, 0xffffffffu, 0, kFe, kFe});
}

// 0x9f43e..0x9f68d, after the panels and before the texts: the layers of upgrade_icons.cpp, 128 x 64 at (430, 150)
// (panel x + 272 - 138), the pulse from the 60 Hz frame counter 0x439df0
void UpgradeMenu::drawWeaponIcons(const FeCanvas &canvas, int weaponLevel) const
{
    ImDrawList *drawList = canvas.drawList();
    const int pulse = menu::upgradeIconPulse(int(ImGui::GetTime() * 60.0));
    menu::IconBlend blend = menu::IconBlend::Alpha;
    for (const menu::UpgradeIconLayer &layer : menu::upgradeWeaponIcons(weaponLevel, pulse))
    {
        if (layer.blend != blend)
        {
            setIconBlend(drawList, layer.blend);
            blend = layer.blend;
        }
        drawIconLayer(canvas, layer);
    }
    setIconBlend(drawList, menu::IconBlend::Alpha);
}

void UpgradeMenu::setIconBlend(ImDrawList *drawList, menu::IconBlend blend) const
{
    if (blend == menu::IconBlend::Additive)
    {
        painter.blend(drawList, argon::gfx::UiBlend::Additive);
        return;
    }
    drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
}

void UpgradeMenu::drawIconLayer(const FeCanvas &canvas, const menu::UpgradeIconLayer &layer) const
{
    ImDrawList *drawList = canvas.drawList();
    const float pixelsPerUnit = canvas.scale();
    const float originX = canvas.originX();
    if (const Found<const HudTexture> texture = painter.texture(layer.texture))
    {
        const ImVec2 topLeft(atRight(originX, kIconX) * pixelsPerUnit, kIconY * pixelsPerUnit), bottomRight(atRight(originX, kIconX + 128.f) * pixelsPerUnit, (kIconY + 64.f) * pixelsPerUnit);
        drawList->AddImage(texture->image, topLeft, bottomRight, ImVec2(0, 0), ImVec2(1, 1), imColour(layer.argb));
    }
}

// Turbo / Shield: a stat bar (0xc2420) at (306, 260 / 330) 252 x 22 filled (level + 3) x 10 %, the name right-aligned
// at 550 and the value ("%.1fs" 1.5 + 0.5 level / shield 20 x level + 100 / ((100 - armour) x 0.01), 0x64030) at 314,
// "Next upgrade: Turbo +0.5s" (0x423) at (306, 283) / "... Shield +20" (0x424) at (306, 353)
void UpgradeMenu::drawStatPanel(const FeCanvas &canvas, int upgrade) const
{
    const float originX = canvas.originX();
    const int levelNow = level(upgrade);
    const bool turbo = upgrade == kTurbo;
    const float panelX = atRight(originX, kPanelX), x = panelX + 10.f, y = turbo ? 260.f : 330.f;
    canvas.panel({x, y, 252.f, 22.f}, {false, false, kYellow, true, 0xff000000u, 0xff000000u, 1.f});
    canvas.panel({x, y, float(levelNow + 3) * 10.f * 0.01f * 252.f, 22.f}, {false, false, kYellow, true, 0xffff8040u, 0xffff8040u, 1.f});
    char value[32];
    if (turbo)
    {
        std::snprintf(value, sizeof value, "%.1fs", 1.5 + 0.5 * levelNow);
    }
    else
    {
        std::snprintf(value, sizeof value, "%d", int(std::lround(sources.weapons.shieldSize(upgradeHost.upgradeChar(), levelNow))));
    }
    canvas.text({panelX + kPanelW - 18.f, y + 2.f}, turbo ? strings.race(0x400, "Turbo/Lap") : strings.race(0x401, "Shield"), {0, 0xffffffffu, 1, kFe, kFe});
    canvas.text({panelX + 18.f, y + 2.f}, argon::widenLatin1(value), {0, 0xffffffffu, 0, kFe, kFe});
    if (levelNow < argon::upgrades::kMaxLevel)
    {
        canvas.text({x, turbo ? 283.f : 353.f},
                    turbo ? strings.race(0x423, "Next upgrade: Turbo +0.5s") : strings.race(0x424, "Next upgrade: Shield +20"),
                    {0, 0xffffffffu, 0, 0.8f * kFe, 0.8f * kFe});
    }
}
