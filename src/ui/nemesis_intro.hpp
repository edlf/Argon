// Argon: the LOADER's Nemesis Challenge intro (loader_draw_frame 0x808e0, docs\frontend\README.md "Loading
// screen"): the first 6 s of the loading screen before a tournament's nemesis race, race string 0x535 drawn as
// red texts sweeping across, 16 copies turning and shrinking into the centre, then a yellow one zooming out
#pragma once
#include <cstdint>
#include <vector>

namespace menu
{
// one draw of the text in font 1 (font_draw_text 0xb1660, alignment 2): centred on x, its top at y
struct IntroText
{
    float x = 320.f, y = 240.f;
    float scale = 1.f;
    float angleDegrees = 0.f;   // turned about (x, y): the matrix (cos, sin, -sin, cos) of 0xb1270
    uint32_t colour = 0;        // ARGB
};

class NemesisIntro
{
  public:
    static constexpr float kSeconds = 6.f;   // 0x1a36b0: the loader's own clock (+0x1a80) below this

    static bool showing(float seconds)
    {
        return seconds >= 0.f && seconds < kSeconds;
    }
    static std::vector<IntroText> texts(float seconds);   // in drawing order

  private:
    static void addSweeps(std::vector<IntroText> &out, float seconds);
    static void addTurningCopies(std::vector<IntroText> &out, float t);
    static IntroText turningCopy(int copy, float seconds);
    static void addSettled(std::vector<IntroText> &out, float seconds);
};
}  // namespace menu
