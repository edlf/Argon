// Argon: what App loads into the 2D layer (hud\hud_painter.hpp: App::painter): the HUD textures and fonts (loadHud), the
// string tables (App::strings, ui\game_strings.hpp); the brightness overlay, the lap-time text, the racers' names.
#include "app/app.hpp"

#include <stdexcept>

#include "original/record_read.hpp"
#include "platform/platform.hpp"
#include "original/text_records.hpp"
#include "files/string_table.hpp"

// Brightness (gamma ramp 0x2f100): the game sets ramp[i] = clamp(i + (step * 3 - 12) * 4, 0, 255) on all three
// channels (D3DDevice_SetGammaRamp). The swapchain is UNORM, so the same is a full-screen constant added (step > 4)
// or subtracted (step < 4) over the finished picture, clamped by the framebuffer.
void App::drawBrightness()
{
    int brightnessShift = (applied.brightness * 3 - 12) * 4;
    if (brightnessShift == 0)
    {
        return;
    }
    ImDrawList *drawList = ImGui::GetForegroundDrawList();
    uint32_t v = uint32_t(std::abs(brightnessShift));
    painter.blend(drawList, brightnessShift > 0 ? argon::gfx::UiBlend::Additive : argon::gfx::UiBlend::Subtractive);
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    drawList->AddRectFilled(ImVec2(0, 0), displaySize, IM_COL32(v, v, v, 255));
    drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
}

void App::loadHud()
{
    const char *names[] = {"speedo_overlay",
                           "speedo_bars",
                           "speedo_needle",
                           "weaponbase",
                           "weaponspecial",
                           "weaponred1",
                           "weaponred2",
                           "weaponred3",
                           "weaponblue1",
                           "weaponblue2",
                           "weaponblue3",
                           "weaponyellow1",
                           "weaponyellow2",
                           "weaponyellow3"};
    for (const char *name : names)
    {
        painter.loadTexture(name, game + "/data/hud/" + name + ".dds");
    }
    for (const char *name : {"crosshair", "crosshair_blue", "crosshair_yellow"})
    {
        painter.loadTexture(name, game + "/data/hud/" + name + ".dds");
    }
    painter.loadTexture("arrow", game + "/data/hud/arrow.dds");
    painter.loadTiledTexture("static", game + "/data/hud/static.dds");   // the RESET overlay (gamehud_setup 0x46040)
    painter.loadTiledTexture("raincloud", game + "/data/hud/raincloud.dds");   // the rain's haze (0x90b69)
    for (auto &button : std::vector<std::pair<const char *, const char *>>{
             {"BUTA", "a"}, {"BUTB", "b"}, {"BUTX", "x"}, {"BUTY", "y"}, {"BUTL", "left"}, {"BUTR", "right"}, {"BUTU", "up"}, {"BUTD", "down"}})
    {
        painter.loadTexture(button.first, game + "/icons/" + button.second + ".dds");
    }
    painter.loadTexture("qrlogo", game + "/data/qrlogo.dds");
    for (int i = 0; i < 16; ++i)
    {   // pilot portraits for the position list (0x793a0: tex%02d)
        char nm[32], fl[32];
        std::snprintf(nm, sizeof nm, "pilot%02d", i);
        std::snprintf(fl, sizeof fl, "tex%02d.dds", i);
        painter.loadTexture(nm, game + "/data/hud/" + fl);
    }
    std::error_code ec;
    for (auto &entry : std::filesystem::directory_iterator(argon::findGamePath(game + "/data/hud/minimap"), ec))
    {
        if (entry.path().extension() == ".dds")
        {
            painter.loadTexture("minimap/" + entry.path().stem().string(), entry.path().string());
        }
    }
    loadFonts();
    loadStringTables();
}

// the string tables (string_table.hpp) of settings.json's language, Locale\<language>\ (0x79030)
void App::loadStringTables()
{
    strings = readGameStrings(game + "/" + viewerSettings.localeFolder() + "/", viewerSettings.language());
}

void App::loadFonts()
{
    painter.loadFonts(viewerSettings.language() == Language::Japanese, replacementFontFolders());
}

// Save changes the language; its fonts load before the next frame is built, once no frame in flight draws the old ones
void App::matchFontsToLanguage()
{
    if (painter.japaneseFonts() == (viewerSettings.language() == Language::Japanese))
    {
        return;
    }
    renderer.presenter().waitForSubmittedWork();
    loadFonts();
}

// <project>\fonts (beside game_data and save_data) and the fonts folder beside the executable
std::vector<std::filesystem::path> App::replacementFontFolders() const
{
    return {std::filesystem::path(saves).parent_path() / "fonts", platform::executablePath().parent_path() / "fonts"};
}

// the game's own (Tara, Yuri, Kumiko, Zaki, ... Max, Kutoo), not the data folders' (darius, kaitlyn, lukas, ...)
std::u16string App::racerName(int pilot) const
{
    const char *folderName = argon::pilotName(pilot);
    return strings.info(400 + (pilot & 15), folderName ? folderName : "");
}
