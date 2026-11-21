// Argon: the rules of the race's event sounds, see sound_rules.hpp.
#include "logic/sound_rules.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <sstream>

namespace argon::sound
{
namespace
{
constexpr float kQuietestMb = -10000.f, kLoudestMb = 0.f;
constexpr float kScrapeDot = 0.35f, kSilentScrapeDot = 0.1f;   // 0x1a3778 / 0x1a3770
constexpr int kTallyFirstStep = 0x2e1, kTallyStepGrowth = 0x18;
constexpr float kChingRate = 22050.f, kChingHighestRate = 0x2edef;

constexpr float kLongestImpactDelay = 10.f;   // 0x1a35a4: a longer delay falls back to 0.25

// sound_load_f_settings 0xc9890: one fscanf("<name> %d\n") per line in a fixed order. A line that does not match,
// or a %d stopped short by a fraction (the shipped "scrape trail 0.1"), leaves the stream stuck: every later setting
// keeps its default.
class SettingsScanner
{
  public:
    explicit SettingsScanner(const std::string &text) : in_(text)
    {
    }

    void scanInteger(const char *name, float &value)
    {
        std::string number;
        if (!nextNumber(name, number))
        {
            return;
        }
        char *end = nullptr;
        const long scanned = std::strtol(number.c_str(), &end, 10);
        if (end == number.c_str())
        {
            stuck_ = true;
            return;
        }
        value = float(scanned);
        stuck_ = *end != '\0';
    }

    // the file's water forces are negative speeds
    void scanDownwardSpeed(const char *name, float &value)
    {
        float stored = -value;
        scanInteger(name, stored);
        value = -stored;
    }

    void scanReal(const char *name, float &value)
    {
        std::string number;
        if (!nextNumber(name, number))
        {
            return;
        }
        char *end = nullptr;
        value = std::strtof(number.c_str(), &end);
        stuck_ = end == number.c_str() || *end != '\0';
    }

  private:
    bool nextNumber(const char *name, std::string &number)
    {
        std::string line;
        if (stuck_ || !std::getline(in_, line))
        {
            stuck_ = true;
            return false;
        }
        const std::string prefix = std::string(name) + " ";
        if (line.rfind(prefix, 0) != 0)
        {
            stuck_ = true;
            return false;
        }
        std::istringstream(line.substr(prefix.size())) >> number;
        return true;
    }

    std::istringstream in_;
    bool stuck_ = false;
};

// an int's high 16 bits, as the short after the one sscanf("%d") was given
float highHalfOf(int value)
{
    return float(int16_t(uint32_t(value) >> 16));
}

// the surfaces whose hits use the track sounds (0, 4 slippery, 5) and their scrape
bool hardSurface(int surface)
{
    return surface == 0 || surface == 4 || surface == 5;
}

GenericSound scrapeOf(int surface)
{
    if (hardSurface(surface))
    {
        return GenericSound::ScrapeMetal;
    }
    return surface == 1 || surface == 2 ? GenericSound::ScrapeWall : GenericSound::None;
}

GenericSound hitOf(int surface, bool heavy)
{
    if (hardSurface(surface))
    {
        return heavy ? GenericSound::HeavyImpactTrack : GenericSound::LightImpactTrack;
    }
    if (surface == 1)
    {
        return heavy ? GenericSound::HeavyImpactRough : GenericSound::LightImpactRough;
    }
    return heavy ? GenericSound::HeavyImpactLoose : GenericSound::LightImpactLoose;
}

ContactSound scrapeSound(const CraftContact &contact, float normalDot, float speedChange)
{
    ContactSound sound;
    if (std::abs(normalDot) < kSilentScrapeDot)
    {
        return sound;
    }
    sound.sound = scrapeOf(contact.surface);
    sound.scrape = true;
    sound.force = speedChange;
    return sound;
}

// a head-on contact; water only ever hits lightly (0xc2d0e's case 3)
ContactSound hitSound(const CraftContact &contact, float force, const ForceRange &forces)
{
    ContactSound sound;
    sound.stopScrapes = true;
    sound.force = force;
    if (contact.surface == 3)
    {
        sound.sound = force > forces.light ? GenericSound::LightImpactWater : GenericSound::None;
        return sound;
    }
    if (contact.surface < 0 || contact.surface > 5)
    {
        return sound;
    }
    if (force > forces.heavy)
    {
        sound.sound = hitOf(contact.surface, true);
    }
    else if (force > forces.light)
    {
        sound.sound = hitOf(contact.surface, false);
    }
    return sound;
}
}  // namespace

ImpactSettings parseImpactSettings(const std::string &text)
{
    ImpactSettings settings;
    float unusedScrapeForce = 0.f, unusedScrapeTrail = 0.f;
    SettingsScanner scanner(text);
    scanner.scanInteger("light impact force", settings.world.light);
    scanner.scanInteger("heavy impact force", settings.world.heavy);
    scanner.scanReal("impact time delay", settings.impactDelay);
    if (settings.impactDelay > kLongestImpactDelay)
    {
        settings.impactDelay = ImpactSettings().impactDelay;
    }
    scanner.scanInteger("scrape force", unusedScrapeForce);
    scanner.scanInteger("scrape trail", unusedScrapeTrail);
    scanner.scanInteger("light impact ship", settings.ship.light);
    scanner.scanInteger("heavy impact ship", settings.ship.heavy);
    scanner.scanDownwardSpeed("light impact water", settings.water.light);
    scanner.scanDownwardSpeed("heavy impact water", settings.water.heavy);
    scanner.scanInteger("max impact force", settings.world.max);
    scanner.scanDownwardSpeed("max impact water", settings.water.max);
    scanner.scanInteger("max impact ship", settings.ship.max);
    return settings;
}

void scanLoopsInto(int loops, VolumeRange &volume)
{
    volume.minMb = highHalfOf(loops);
}

void scanMinVolumeInto(int minMb, VolumeRange &volume)
{
    volume.minMb = float(int16_t(minMb));
    volume.maxMb = highHalfOf(minMb);
}

float hitVolumeMb(float force, const ForceRange &forces, const VolumeRange &volume)
{
    float from = forces.light, to = forces.heavy;
    if (force > forces.heavy)
    {
        from = forces.heavy;
        to = forces.max;
    }
    const float span = (to - from) * 0.01f;
    const float percent = span != 0.f ? std::round((force - from) / span) : 0.f;
    const float mb = (volume.maxMb - volume.minMb) * 0.01f * percent + volume.minMb;
    return std::clamp(mb, kQuietestMb, kLoudestMb);
}

ContactSound worldContactSound(const CraftContact &contact, const ImpactSettings &settings)
{
    if (contact.count < 1)
    {
        ContactSound none;
        none.stopScrapes = true;
        return none;
    }
    const float speedChange = glm::length(contact.velocityChange);
    const float normalDot = contact.normalDotSum / float(contact.count);   // 0xd0a30
    if (std::abs(normalDot) < kScrapeDot)
    {
        return scrapeSound(contact, normalDot, speedChange);
    }
    return hitSound(contact, normalDot * speedChange, settings.world);
}

GenericSound shipImpactSound(float velocityChange, const ImpactSettings &settings)
{
    if (velocityChange > settings.ship.heavy)
    {
        return GenericSound::HeavyImpactMetal;
    }
    return velocityChange > settings.ship.light ? GenericSound::LightImpactMetal : GenericSound::None;
}

Splash waterSplash(float verticalSpeed, const ImpactSettings &settings)
{
    const float down = -verticalSpeed;
    if (down > settings.water.heavy)
    {
        return {GenericSound::HeavyImpactWater, GenericSound::LightImpactWater};
    }
    if (down > settings.water.light)
    {
        return {GenericSound::LightImpactWater, GenericSound::HeavyImpactWater};
    }
    return {};
}

// DirectSound's CalcDistanceVolume (0x13d8a7): the n points split MINDIST..MAXDIST into n equal steps, point k at
// the end of step k + 1, so the curve runs from 1 at MINDIST to the first point, then point to point. A range with
// MAXDIST at MINDIST has steps of width 0: beyond MINDIST the distance 0 / 0 is NaN and the voice keeps its full
// volume (Silver's ocean, MINDIST = MAXDIST = 1000: xemu's VP shows its voice at the SFX option's -4 dB from 2.8 km)
float rolloffGain(float distance, float minDist, float maxDist)
{
    static const std::array<float, 5> kCurve = {1.f, 0.5f, 0.25f, 0.125f, 0.f};
    if (distance <= minDist || maxDist <= minDist)
    {
        return 1.f;
    }
    if (distance >= maxDist)
    {
        return kCurve.back();
    }
    const float stepWidth = (maxDist - minDist) / float(kCurve.size());
    const float steps = (distance - minDist) / stepWidth;
    const size_t step = std::min(size_t(steps), kCurve.size() - 1);
    const float from = step == 0 ? 1.f : kCurve[step - 1];
    return from + (kCurve[step] - from) * (steps - float(step));
}

bool shieldLaunchSounds(int slot, bool splitScreen)
{
    return slot == 0 || splitScreen;
}

void ScoreTally::start(int target)
{
    target_ = target;
    shown_ = 0;
    step_ = kTallyFirstStep;
    counting_ = true;
}

bool ScoreTally::step()
{
    if (!counting_)
    {
        return false;
    }
    shown_ += step_;
    step_ += kTallyStepGrowth;
    if (shown_ < target_)
    {
        return false;
    }
    finish();
    return true;
}

void ScoreTally::finish()
{
    shown_ = target_;
    counting_ = false;
}

float ScoreTally::chingPitch() const
{
    return std::min(kChingRate + 2.f * float(step_), kChingHighestRate) / kChingRate;
}

}  // namespace argon::sound
