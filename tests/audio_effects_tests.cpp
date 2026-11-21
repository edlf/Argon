// Argon: tests for the game sounds' effects: filters.txt, the low-pass, the HRTF's direction and ear filter, the
// crosstalk, the doppler, the speaker mixbins and their stereo downmix, 5.1's front / rear gains and a voice's routing
// (audio/low_pass.hpp, audio/hrtf.hpp, audio/voice_router.hpp,
// audio/crosstalk.hpp, audio/spatial.hpp, audio/mix_paths.hpp, audio/kemar.hpp). Run with test.bat / test.sh.
#include "audio/crosstalk.hpp"
#include "audio/fir_block.hpp"
#include "check.hpp"
#include "audio/hrtf.hpp"
#include "audio/hrtf_set.hpp"
#include "audio/kemar.hpp"
#include "audio/low_pass.hpp"
#include "audio/mix_paths.hpp"
#include "audio/spatial.hpp"
#include "audio/voice_router.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{
using argon::test::checks;
using argon::test::failures;

const int kRate = 48000;

float energy(const std::vector<float> &s, size_t from, size_t to)
{
    float e = 0.f;
    for (size_t i = from * 2; i < to * 2; ++i)
    {
        e += s[i] * s[i];
    }
    return e;
}

void filtersAreRead()
{
    const argon::ListenerFilters f = argon::parseListenerFilters("Internal\n65280 1400 10 1000 0 0\nUnderwater\n65280 440 0 1000 0 0\n");
    CHECK(f.internal.on && f.internal.cutoffHz == 1400.f && f.internal.resonanceDb == 10.f && f.underwater.cutoffHz == 440.f);
}

void theLowPassDampsHighTones()
{
    argon::ResonantLowPass lp;
    lp.configure({true, 440.f, 0.f}, kRate);
    std::vector<float> s(size_t(kRate) * 2);
    for (size_t i = 0; i < size_t(kRate); ++i)
    {
        s[i * 2] = s[i * 2 + 1] = std::sin(2.f * 3.14159265f * 8000.f * float(i) / float(kRate));
    }
    const float before = energy(s, 1000, size_t(kRate));
    lp.process(s.data(), size_t(kRate), 2);
    CHECK(energy(s, 1000, size_t(kRate)) < before * 0.01f);
}

void anOffLowPassChangesNothing()
{
    argon::ResonantLowPass lp;
    lp.configure({}, kRate);
    std::vector<float> s = {0.5f, -0.25f};
    lp.process(s.data(), 1, 2);
    CHECK(s[0] == 0.5f && s[1] == -0.25f);
}

void aSourceOnTheRightIsAtNinetyDegrees()
{
    const argon::hrtf::Direction d = argon::hrtf::directionOf(argon::hrtf::Head{}, glm::vec3(10.f, 0.f, 0.f));
    CHECK(std::abs(d.azimuth - 90.f) < 1e-4f && std::abs(d.elevation) < 1e-4f);
}

void aSourceBehindIsAtOneEightyDegrees()
{
    CHECK(std::abs(argon::hrtf::directionOf(argon::hrtf::Head{}, glm::vec3(0.f, 0.f, -10.f)).azimuth - 180.f) < 1e-4f);
}

void aSourceOnTheLeftFrontIsAtMinusFortyFive()
{
    const argon::hrtf::Direction d = argon::hrtf::directionOf(argon::hrtf::Head{}, glm::vec3(-10.f, 0.f, 10.f));
    CHECK(std::abs(d.azimuth + 45.f) < 1e-4f);
}

void aSourceAboveIsAtNinetyDegreesUp()
{
    CHECK(std::abs(argon::hrtf::directionOf(argon::hrtf::Head{}, glm::vec3(0.f, 10.f, 0.f)).elevation - 90.f) < 1e-4f);
}

void withoutTablesTheEarsPassTheSoundThrough()
{
    argon::hrtf::EarFilter f;
    f.jumpTo(argon::hrtf::earsFor(argon::hrtf::Tables{}, {}));
    std::vector<float> in = {0.5f, -0.25f}, out(2);
    f.process(in.data(), out.data(), 1);
    CHECK(out[0] == 0.5f && out[1] == -0.25f);
}

bool nearGain(float a, float b)
{
    return std::abs(a - b) < 1e-3f;
}

void aLeftFileAlsoReachesTheRightThroughTheCentre()
{
    const glm::vec2 gains = argon::mix::stereoDownmix(argon::mix::speakerFileGains("silver_l"));
    CHECK(nearGain(gains.x, 1.3976f) && nearGain(gains.y, 0.3976f));
}

void aRearFileIsDownmixedThreeDecibelsDown()
{
    const glm::vec2 gains = argon::mix::stereoDownmix(argon::mix::speakerFileGains("silver_rs"));
    CHECK(gains.x == 0.f && nearGain(gains.y, 0.7071f));
}

void aNonPositionalSoundsSpeakerVoicesAddAboutTwoThirdsPerSide()
{
    const glm::vec2 share = argon::mix::stereoDownmix(argon::mix::speakerShare(argon::mix::kNoPositionSpeakers));
    CHECK(nearGain(share.x, 0.6286f) && nearGain(share.y, 0.6286f));
}

void theLsFlagsSpeakerVoiceIsTheBackRight()
{
    argon::mix::SpeakerFlags speakers;
    speakers.ls = true;
    const glm::vec2 share = argon::mix::stereoDownmix(argon::mix::speakerShare(speakers));
    CHECK(share.x == 0.f && nearGain(share.y, 0.1776f));
}

void aFileWithoutASpeakerSuffixPlaysOnBothSides()
{
    const glm::vec2 gains = argon::mix::stereoDownmix(argon::mix::speakerFileGains("water_enter"));
    CHECK(gains.x == 1.f && gains.y == 1.f);
}

void theLsFlagsShareIsOnTheBackRightMixbin()
{
    argon::mix::SpeakerFlags speakers;
    speakers.ls = true;
    const argon::mix::SpeakerGains share = argon::mix::speakerShare(speakers);
    CHECK(share[argon::mix::kBackLeft] == 0.f && nearGain(share[argon::mix::kBackRight], 0.2512f));
}

void theAmbiencesLfeFileIsOnTheLfe()
{
    CHECK(argon::mix::speakerFileGains("amb_8.lfe")[argon::mix::kLfe] == 1.f);
}

void theStereoDownmixDropsTheLfe()
{
    const glm::vec2 stereo = argon::mix::stereoDownmix(argon::mix::speakerFileGains("amb_8.lfe"));
    CHECK(stereo.x == 0.f && stereo.y == 0.f);
}

void anAmbienceCentreFileIsOnTheCentre()
{
    CHECK(argon::mix::speakerFileGains("amb_8.c")[argon::mix::kCentre] == 1.f);
}

void fiveOneKeepsTheSpeakerMixbinsAsTheyAre()
{
    const std::vector<float> bins = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f};
    std::vector<float> out(6);
    argon::mix::foldSpeakers(bins.data(), out.data(), 1, argon::mix::SpeakerLayout::Surround);
    CHECK(out == bins);
}

void stereoFoldsTheCentreAndARearIntoTheirSide()
{
    const std::vector<float> bins = {0.f, 0.f, 1.f, 1.f, 1.f, 0.f};
    std::vector<float> out(2);
    argon::mix::foldSpeakers(bins.data(), out.data(), 1, argon::mix::SpeakerLayout::Stereo);
    CHECK(nearGain(out[0], 2.f * 0.7071f) && nearGain(out[1], 0.7071f));
}

argon::hrtf::Direction at(float azimuth, float distance)
{
    return {azimuth, 0.f, distance};
}

void aSourceAheadIsAllFront()
{
    const argon::hrtf::FrontRear gains = argon::hrtf::surroundGains(at(0.f, 10.f));
    CHECK(gains.front == 1.f && gains.rear == 0.f);
}

void aSourceBehindIsAllRear()
{
    const argon::hrtf::FrontRear gains = argon::hrtf::surroundGains(at(180.f, 10.f));
    CHECK(gains.front == 0.f && gains.rear == 1.f);
}

void aSourceToTheSideIsHalfPowerEachWay()
{
    const argon::hrtf::FrontRear gains = argon::hrtf::surroundGains(at(-90.f, 10.f));
    CHECK(nearGain(gains.front, 0.7071f) && nearGain(gains.rear, 0.7071f));
}

void aSourceUnderFortyFiveDegreesStaysFront()
{
    CHECK(argon::hrtf::surroundGains(at(40.f, 10.f)).rear == 0.f);
}

void aSourceOnTheListenerIsHalfPowerEachWay()
{
    const argon::hrtf::FrontRear gains = argon::hrtf::surroundGains(at(0.f, 0.f));
    CHECK(nearGain(gains.front, 0.7071f) && nearGain(gains.rear, 0.7071f));
}

void surroundTakesABehindSourcesFilterFromItsFrontMirror()
{
    CHECK(argon::hrtf::mirroredToFront(at(-150.f, 10.f)).azimuth == -30.f);
}

argon::VoiceRouting passThroughRouting(const argon::hrtf::FrontRear &frontRear)
{
    argon::VoiceRouting routing;
    routing.ears = argon::hrtf::earsFor(argon::hrtf::Tables{}, {});
    routing.frontRear = frontRear;
    return routing;
}

void fiveOneRoutesAVoiceToTheFrontAndRearPairs()
{
    argon::VoiceRouter router(argon::mix::SpeakerLayout::Surround);
    router.jumpTo(passThroughRouting({0.6f, 0.8f}));
    const std::vector<float> in = {1.f, 1.f};
    std::vector<float> out(6);
    router.process(in.data(), out.data(), 1);
    CHECK(nearGain(out[argon::mix::kFrontLeft], 0.6f) && nearGain(out[argon::mix::kBackRight], 0.8f) && out[argon::mix::kCentre] == 0.f);
}

void fiveOnePutsTheSpeakerVoicesOnTheirOwnChannels()
{
    argon::VoiceRouting routing = passThroughRouting({1.f, 0.f});
    routing.speakers[argon::mix::kCentre] = 0.5f;
    argon::VoiceRouter router(argon::mix::SpeakerLayout::Surround);
    router.jumpTo(routing);
    const std::vector<float> in = {1.f, 1.f};
    std::vector<float> out(6);
    router.process(in.data(), out.data(), 1);
    CHECK(nearGain(out[argon::mix::kCentre], 0.5f) && nearGain(out[argon::mix::kFrontLeft], 1.f));
}

void stereoAddsTheSpeakerVoicesDownmix()
{
    argon::VoiceRouting routing = passThroughRouting({1.f, 0.f});
    routing.speakers[argon::mix::kCentre] = 1.f;
    argon::VoiceRouter router(argon::mix::SpeakerLayout::Stereo);
    router.jumpTo(routing);
    const std::vector<float> in = {1.f, 1.f};
    std::vector<float> out(2);
    router.process(in.data(), out.data(), 1);
    CHECK(nearGain(out[0], 1.7071f) && nearGain(out[1], 1.7071f));
}

void theRearCrosstalkWorksOnItsPairInTheSixChannels()
{
    argon::Crosstalk x;
    x.configure(kRate);
    std::vector<float> s(32 * 6, 0.f);
    s[argon::mix::kBackLeft] = 1.f;
    x.process(s.data() + argon::mix::kBackLeft, 32, 6);
    CHECK(s[11 * 6 + argon::mix::kBackRight] < 0.f && s[11 * 6 + argon::mix::kFrontRight] == 0.f);
}

void aSmallTapCodeIsItsMantissa()
{
    CHECK(argon::hrtf::decodeTap(0x0b) == 11.f / 2048.f);
}

void aTapCodesExponentShiftsTheMantissaWithItsHiddenBit()
{
    CHECK(argon::hrtf::decodeTap(0x55) == float((16 + 5) << 4) / 2048.f);
}

void aTapCodesHighBitIsItsSign()
{
    CHECK(argon::hrtf::decodeTap(0x9f) == -31.f / 2048.f);
}

void theLargestTapCodeStaysUnderFullScale()
{
    CHECK(argon::hrtf::decodeTap(0x7f) == 1984.f / 2048.f);
}

bool sameTables(const argon::hrtf::Tables &a, const argon::hrtf::Tables &b)
{
    if (a.index != b.index || a.filters.size() != b.filters.size())
    {
        return false;
    }
    for (size_t f = 0; f < a.filters.size(); ++f)
    {
        if (a.filters[f].codes != b.filters[f].codes || a.filters[f].delay != b.filters[f].delay)
        {
            return false;
        }
    }
    return true;
}

// tools\assets\extract_hrtf.py's hrtf.bin holds the same tables as default.xbe (skipped without the game's files)
void theExtractedTablesMatchTheXbe()
{
    const std::filesystem::path game = QR_SOURCE_DIR "/game_data", none = game / "no such file";
    argon::hrtf::Tables extracted, xbe;
    try
    {
        extracted = argon::hrtf::loadTables({game / "audio" / "hrtf.bin", none});
        xbe = argon::hrtf::loadTables({none, game / "default.xbe"});
    }
    catch (const std::runtime_error &e)
    {
        std::printf("skipped: %s\n", e.what());
        return;
    }
    CHECK(sameTables(extracted, xbe));
}

void aPositiveDelayHoldsBackTheLeftEar()
{
    argon::hrtf::Ears ears = argon::hrtf::earsFor(argon::hrtf::Tables{}, {});
    ears.delay = 3.f;
    argon::hrtf::EarFilter f;
    f.jumpTo(ears);
    std::vector<float> in(16, 0.f), out(16);
    in[0] = in[1] = 1.f;
    f.process(in.data(), out.data(), 8);
    CHECK(out[1] == 1.f && out[0] == 0.f && out[3 * 2] == 1.f);
}

void theLongestEarsLastTapIsHeardThatManySamplesLate()
{
    argon::hrtf::Ears ears;
    ears.taps = argon::hrtf::kMaxTaps;
    ears.left[argon::hrtf::kMaxTaps - 1] = ears.right[argon::hrtf::kMaxTaps - 1] = 1.f;
    argon::hrtf::EarFilter f;
    f.jumpTo(ears);
    std::vector<float> in(argon::hrtf::kMaxTaps * 2, 0.f), out(argon::hrtf::kMaxTaps * 2);
    in[0] = in[1] = 1.f;
    f.process(in.data(), out.data(), argon::hrtf::kMaxTaps);
    CHECK(out[(argon::hrtf::kMaxTaps - 1) * 2] == 1.f && out[(argon::hrtf::kMaxTaps - 2) * 2] == 0.f);
}

// 100 taps over 37 frames of a ramp with 99 samples of history before it
struct FirExample
{
    std::vector<float> taps, signal;
    argon::fir::Block block() const
    {
        return {taps.data(), int(taps.size()), signal.data() + taps.size() - 1, 37};
    }
};

FirExample firExample()
{
    constexpr size_t kTaps = 100, kHistory = kTaps - 1;
    FirExample example;
    for (size_t k = 0; k < kTaps; ++k)
    {
        example.taps.push_back(std::sin(float(k)) / float(k + 1));
    }
    for (size_t at = 0; at < kHistory + argon::fir::kBlockFrames; ++at)
    {
        example.signal.push_back(std::cos(0.3f * float(at)));
    }
    return example;
}

// the largest difference from the plain sum over the block's frames
float firError(void (*kernel)(const argon::fir::Block &, float *))
{
    const FirExample example = firExample();
    const argon::fir::Block block = example.block();
    std::vector<float> out(argon::fir::kBlockFrames);
    kernel(block, out.data());
    float worst = 0.f;
    for (int f = 0; f < block.frames; ++f)
    {
        double sum = 0.0;
        for (int k = 0; k < block.tapCount; ++k)
        {
            sum += double(block.taps[k]) * double(block.input[f - k]);
        }
        worst = std::max(worst, float(std::abs(sum - double(out[size_t(f)]))));
    }
    return worst;
}

void theSseFirMatchesThePlainSum()
{
    CHECK(firError(argon::fir::convolveSse) < 1e-5f);
}

void theAvx2FirMatchesThePlainSum()
{
    if (!argon::fir::cpuHasAvx2Fma())
    {
        std::printf("skipped: no AVX2 / FMA\n");
        return;
    }
    CHECK(firError(argon::fir::convolveAvx2) < 1e-5f);
}

// the left ear's taps 0 and `last` at 1
argon::hrtf::Ears earsHearingTapsZeroAnd(int last)
{
    argon::hrtf::Ears ears;
    ears.taps = last + 1;
    ears.left[0] = ears.left[size_t(last)] = 1.f;
    return ears;
}

// the left ear's output for a click on the first of `frames` frames
std::vector<float> leftEarHearingAClick(argon::hrtf::EarFilter &f, size_t frames)
{
    std::vector<float> in(frames * 2, 0.f), out(frames * 2), left(frames);
    in[0] = 1.f;
    f.process(in.data(), out.data(), frames);
    for (size_t frame = 0; frame < frames; ++frame)
    {
        left[frame] = out[frame * 2];
    }
    return left;
}

void aGlidingTapMovesAHundredthOfTheWayPerSample()
{
    argon::hrtf::EarFilter f;
    f.jumpTo(earsHearingTapsZeroAnd(1));
    f.setTarget(earsHearingTapsZeroAnd(3));
    const std::vector<float> left = leftEarHearingAClick(f, 4);
    CHECK(std::abs(left[3] - (1.f - std::pow(0.99f, 4.f))) < 1e-6f);
}

void aTapCutShortFadesOutAndIsGone()
{
    constexpr int kLong = argon::hrtf::kMaxTaps - 1, kSettled = 2000;
    argon::hrtf::EarFilter f;
    f.jumpTo(earsHearingTapsZeroAnd(kLong));
    f.setTarget(earsHearingTapsZeroAnd(1));
    leftEarHearingAClick(f, kSettled);
    CHECK(leftEarHearingAClick(f, kLong + 1)[kLong] == 0.f);
}

// a two-tap hrtf_kemar.bin (tools\assets\convert_kemar.py's layout): elevation 0 at azimuths 0 and 90, elevation 40 at
// 0; each direction's left taps {azimuth, elevation}, right taps {-azimuth, 0}, delay azimuth / 10
struct KemarDirection
{
    int32_t elevation, azimuth;
};

void appendBytes(std::vector<uint8_t> &bytes, const void *value, size_t count)
{
    const auto *first = static_cast<const uint8_t *>(value);
    bytes.insert(bytes.end(), first, first + count);
}

template <class Value>
void append(std::vector<uint8_t> &bytes, Value value)
{
    appendBytes(bytes, &value, sizeof value);
}

void appendDirection(std::vector<uint8_t> &bytes, const KemarDirection &direction)
{
    append<int32_t>(bytes, direction.azimuth);
    append<float>(bytes, float(direction.azimuth) / 10.f);
    append<float>(bytes, float(direction.azimuth));
    append<float>(bytes, float(direction.elevation));
    append<float>(bytes, -float(direction.azimuth));
    append<float>(bytes, 0.f);
}

void appendRing(std::vector<uint8_t> &bytes, int32_t elevation, uint32_t count)
{
    append<int32_t>(bytes, elevation);
    append<uint32_t>(bytes, count);
}

std::vector<uint8_t> kemarFileBytes()
{
    constexpr uint32_t kVersion = 1, kTaps = 2, kRings = 2;
    std::vector<uint8_t> bytes{'A', 'R', 'K', 'M'};
    append<uint32_t>(bytes, kVersion);
    append<uint32_t>(bytes, kTaps);
    append<uint32_t>(bytes, kRings);
    appendRing(bytes, 0, 2);
    appendDirection(bytes, {0, 0});
    appendDirection(bytes, {0, 90});
    appendRing(bytes, 40, 1);
    appendDirection(bytes, {40, 0});
    return bytes;
}

std::filesystem::path writtenKemarFile(const std::vector<uint8_t> &bytes)
{
    const std::filesystem::path file = std::filesystem::temp_directory_path() / "argon_audio_effects_tests_kemar.bin";
    std::ofstream(file, std::ios::binary).write(reinterpret_cast<const char *>(bytes.data()), std::streamsize(bytes.size()));
    return file;
}

argon::hrtf::KemarTables loadedKemar()
{
    return argon::hrtf::loadKemar(writtenKemarFile(kemarFileBytes()));
}

void kemarTakesTheNearestMeasuredDirection()
{
    const argon::hrtf::Ears ears = argon::hrtf::kemarEarsFor(loadedKemar(), {70.f, 5.f, 1.f}, argon::hrtf::kDefaultKemarTaps);
    CHECK(ears.left[0] == 90.f && ears.left[1] == 0.f && ears.taps == 2);
}

void kemarTakesTheNearestElevationRing()
{
    const argon::hrtf::Ears ears = argon::hrtf::kemarEarsFor(loadedKemar(), {10.f, 30.f, 1.f}, argon::hrtf::kDefaultKemarTaps);
    CHECK(ears.left[1] == 40.f);
}

void kemarMirrorsASourceOnTheLeft()
{
    const argon::hrtf::Ears ears = argon::hrtf::kemarEarsFor(loadedKemar(), {-90.f, 0.f, 1.f}, argon::hrtf::kDefaultKemarTaps);
    CHECK(ears.right[0] == 90.f && ears.left[0] == -90.f && ears.delay == -9.f);
}

void aCutShortKemarFileIsRefused()
{
    std::vector<uint8_t> bytes = kemarFileBytes();
    bytes.pop_back();
    bool refused = false;
    try
    {
        argon::hrtf::loadKemar(writtenKemarFile(bytes));
    }
    catch (const std::runtime_error &)
    {
        refused = true;
    }
    CHECK(refused);
}

// a hrtf_kemar.bin of one direction (elevation 0, azimuth 0) whose `taps` taps are all 1
argon::hrtf::KemarTables loadedKemarOfOnes(uint32_t taps)
{
    constexpr uint32_t kVersion = 1, kRings = 1;
    std::vector<uint8_t> bytes{'A', 'R', 'K', 'M'};
    append<uint32_t>(bytes, kVersion);
    append<uint32_t>(bytes, taps);
    append<uint32_t>(bytes, kRings);
    appendRing(bytes, 0, 1);
    append<int32_t>(bytes, 0);
    append<float>(bytes, 0.f);
    for (uint32_t tap = 0; tap < 2 * taps; ++tap)
    {
        append<float>(bytes, 1.f);
    }
    return argon::hrtf::loadKemar(writtenKemarFile(bytes));
}

void kemarCutToFewerTapsEndsThere()
{
    const argon::hrtf::Ears ears = argon::hrtf::kemarEarsFor(loadedKemarOfOnes(32), {}, 16);
    CHECK(ears.taps == 16 && ears.left[16] == 0.f && ears.right[31] == 0.f);
}

void kemarCutFadesItsLastEightTapsOut()
{
    const argon::hrtf::Ears ears = argon::hrtf::kemarEarsFor(loadedKemarOfOnes(32), {}, 16);
    const float firstFade = std::cos(3.14159265f / 16.f);
    CHECK(ears.left[7] == 1.f && std::abs(ears.left[8] - firstFade * firstFade) < 1e-6f && std::abs(ears.right[15]) < 1e-6f);
}

void kemarWithFewerTapsThanChosenPlaysThemAsTheyAre()
{
    const argon::hrtf::Ears ears = argon::hrtf::kemarEarsFor(loadedKemarOfOnes(32), {}, 512);
    CHECK(ears.taps == 32 && ears.left[31] == 1.f);
}

void theCrosstalkFeedsTheOtherSideElevenSamplesLate()
{
    argon::Crosstalk x;
    x.configure(kRate);
    std::vector<float> s(32 * 2, 0.f);
    s[0] = 1.f;
    x.process(s.data(), 32, 2);
    CHECK(s[10 * 2 + 1] == 0.f && s[11 * 2 + 1] < 0.f);
}

void anApproachingCraftSoundsHigher()
{
    argon::spatial::Listener l;
    CHECK(argon::spatial::dopplerRatio(l, glm::vec3(0.f, 0.f, 100.f), glm::vec3(0.f, 0.f, -50.f)) > 1.f);
}

void aLeavingCraftSoundsLower()
{
    argon::spatial::Listener l;
    CHECK(argon::spatial::dopplerRatio(l, glm::vec3(0.f, 0.f, 100.f), glm::vec3(0.f, 0.f, 50.f)) < 1.f);
}
}  // namespace

int main()
{
    filtersAreRead();
    theLowPassDampsHighTones();
    anOffLowPassChangesNothing();
    aSourceOnTheRightIsAtNinetyDegrees();
    aSourceBehindIsAtOneEightyDegrees();
    aSourceOnTheLeftFrontIsAtMinusFortyFive();
    aSourceAboveIsAtNinetyDegreesUp();
    withoutTablesTheEarsPassTheSoundThrough();
    aLeftFileAlsoReachesTheRightThroughTheCentre();
    aRearFileIsDownmixedThreeDecibelsDown();
    aFileWithoutASpeakerSuffixPlaysOnBothSides();
    aNonPositionalSoundsSpeakerVoicesAddAboutTwoThirdsPerSide();
    theLsFlagsSpeakerVoiceIsTheBackRight();
    theLsFlagsShareIsOnTheBackRightMixbin();
    theAmbiencesLfeFileIsOnTheLfe();
    theStereoDownmixDropsTheLfe();
    anAmbienceCentreFileIsOnTheCentre();
    fiveOneKeepsTheSpeakerMixbinsAsTheyAre();
    stereoFoldsTheCentreAndARearIntoTheirSide();
    aSourceAheadIsAllFront();
    aSourceBehindIsAllRear();
    aSourceToTheSideIsHalfPowerEachWay();
    aSourceUnderFortyFiveDegreesStaysFront();
    aSourceOnTheListenerIsHalfPowerEachWay();
    surroundTakesABehindSourcesFilterFromItsFrontMirror();
    fiveOneRoutesAVoiceToTheFrontAndRearPairs();
    fiveOnePutsTheSpeakerVoicesOnTheirOwnChannels();
    stereoAddsTheSpeakerVoicesDownmix();
    theRearCrosstalkWorksOnItsPairInTheSixChannels();
    aSmallTapCodeIsItsMantissa();
    aTapCodesExponentShiftsTheMantissaWithItsHiddenBit();
    aTapCodesHighBitIsItsSign();
    theLargestTapCodeStaysUnderFullScale();
    theExtractedTablesMatchTheXbe();
    aPositiveDelayHoldsBackTheLeftEar();
    theLongestEarsLastTapIsHeardThatManySamplesLate();
    theSseFirMatchesThePlainSum();
    theAvx2FirMatchesThePlainSum();
    aGlidingTapMovesAHundredthOfTheWayPerSample();
    aTapCutShortFadesOutAndIsGone();
    kemarTakesTheNearestMeasuredDirection();
    kemarTakesTheNearestElevationRing();
    kemarMirrorsASourceOnTheLeft();
    aCutShortKemarFileIsRefused();
    kemarCutToFewerTapsEndsThere();
    kemarCutFadesItsLastEightTapsOut();
    kemarWithFewerTapsThanChosenPlaysThemAsTheyAre();
    theCrosstalkFeedsTheOtherSideElevenSamplesLate();
    anApproachingCraftSoundsHigher();
    aLeavingCraftSoundsLower();
    std::printf("%d of %d checks passed\n", checks - failures, checks);
    return failures == 0 ? 0 : 1;
}
