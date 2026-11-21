// Argon: the race HUD positions per language (hud\hud_language_layout.hpp)
#include "hud/hud_language_layout.hpp"

namespace argon::hud
{
namespace
{
constexpr HudLanguageLayout kWestern{14.f, 13.f, 55.f, 40.f, 28.f, 61.f, 0.f, 2.f, 1.f, 1.f};
constexpr HudLanguageLayout kJapanese{19.f, 15.f, 58.f, 36.f, 26.f, 79.f, 3.f, 3.5f, 0.5f, 3.5f};
}  // namespace

HudLanguageLayout hudLanguageLayout(Language language)
{
    return language == Language::Japanese ? kJapanese : kWestern;
}

}  // namespace argon::hud
