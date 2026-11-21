// Argon: the viewer's own menu text, which has no string in the game's tables (Argon Options, the main menu's viewer
// rows, the pause menu's HUD Size), in the game's languages: English, and French, German and Italian (the EU release's
// tables), Japanese. The words follow the game's own (French "VTH" for the HUD, German without the full stop, Japanese
// "On" / "Off" and もどる). The European texts keep to the Square721 fonts' characters (U+0020..U+0177,
// original::FontMap), the Japanese to MSPGoth's 777 (original::AbcGlyph).
#pragma once
#include "saves/viewer_settings.hpp"

#include <span>
#include <string>

struct ViewerText
{
    const char16_t *english = nullptr;
    const char16_t *french = nullptr;
    const char16_t *german = nullptr;
    const char16_t *italian = nullptr;
    const char16_t *japanese = nullptr;

    std::u16string in(Language language) const;   // the English where the language has none
};

namespace viewer_text
{

// an Argon Options row: its label and its ticker
struct RowText
{
    ViewerText label, description;
};

extern const RowText kSave, kLanguage, kRenderer, kWidescreen, kHudSize, kResolution, kFullscreen, kVsync, kMsaa, kPostAa,
    kTranslucency, kSpeedBlur, kMarsBlurFix, kFmvs, kFpsMeter, kSpeakers, kHrtf, kKemarTaps;

// Argon Options' own rows: its pages, their title and ticker
extern const RowText kGeneral, kGraphics, kAudio, kGameFixes, kKeyBindings;

// Key Bindings: each craft action's label (KeyAction order), the rows' ticker, the ticker while a key is awaited
extern const ViewerText kKeyActions[kKeyActionCount];
extern const ViewerText kKeyBindingTicker, kPressAKey;
extern const ViewerText kResetPageQuestion;   // Reset to Default's question on Argon Options' pages: only that page's

extern const ViewerText kGameOrder, kOrderIndependent, kStereo;   // option words
extern const ViewerText kArgonOptions, kArgonOptionsTicker;       // the page's title, the main menu row and its ticker
extern const ViewerText kRendererRestart;                         // Save's message box when the Renderer changed
extern const ViewerText kSavingSettings;                          // the pause menu's hint while it writes
extern const ViewerText kDebug, kDebugTicker;                     // the developer build's main menu row
extern const ViewerText kExitWithProfile, kExitToLauncher;        // the main menu Exit's ticker

// each language's name in the Language row: its own in the European menus, but drawable in the menu's font (MSPGoth
// has no accented letters, the Square721 fonts no kanji): フランス語 in Japanese, "Japanese" in English
extern const ViewerText kLanguageNames[kLanguageCount];

std::span<const ViewerText *const> all();   // every text above, for the tests

}  // namespace viewer_text
