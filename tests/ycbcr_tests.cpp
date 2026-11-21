// Argon: tests for the video frames' colour conversion (movie/ycbcr.hpp: Theora's Y'CbCr to BGRX). No window
// or GPU: run with test.bat / test.sh (or ctest in build).
#include "movie/ycbcr.hpp"
#include "check.hpp"

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace
{
using argon::test::checks;
using argon::test::failures;

struct Bgrx
{
    int b, g, r, x;
};

// one pixel of the given colour, chroma at full size
Bgrx convertOne(uint8_t y, uint8_t cb, uint8_t cr)
{
    YCbCrImage image;
    image.y = {&y, 1};
    image.cb = {&cb, 1};
    image.cr = {&cr, 1};
    image.chromaShiftX = image.chromaShiftY = 0;
    uint8_t out[4]{};
    convertToBgrx(image, {0, 0, 1, 1}, out);
    return {out[0], out[1], out[2], out[3]};
}

bool near(int value, int expected)
{
    return std::abs(value - expected) <= 2;
}

void testStudioBlackIsBlack()
{
    const Bgrx p = convertOne(16, 128, 128);
    CHECK(p.b == 0 && p.g == 0 && p.r == 0);
}

void testStudioWhiteIsWhite()
{
    const Bgrx p = convertOne(235, 128, 128);
    CHECK(p.b == 255 && p.g == 255 && p.r == 255);
}

void testRedComesOutRed()
{   // BT.601 red: Y' 81, Cb 90, Cr 240
    const Bgrx p = convertOne(81, 90, 240);
    CHECK(near(p.r, 255) && near(p.g, 0) && near(p.b, 0));
}

void testFourthByteIsOpaque()
{
    CHECK(convertOne(100, 100, 100).x == 255);
}

// 4 x 2 luma, 4:2:0 chroma (2 x 1): the left half blue-ish, the right half red-ish; the picture skips column 0
void testSubsampledChromaFollowsThePicture()
{
    const std::vector<uint8_t> luma(8, 128);
    const uint8_t cb[2] = {240, 16}, cr[2] = {16, 240};
    YCbCrImage image;
    image.y = {luma.data(), 4};
    image.cb = {cb, 2};
    image.cr = {cr, 2};
    std::vector<uint8_t> out(3 * 2 * 4);
    convertToBgrx(image, {1, 0, 3, 2}, out.data());
    const bool firstIsBlue = out[0] > out[2], secondIsRed = out[4 + 2] > out[4 + 0];
    CHECK(firstIsBlue && secondIsRed);
}
}  // namespace

int main()
{
    testStudioBlackIsBlack();
    testStudioWhiteIsWhite();
    testRedComesOutRed();
    testFourthByteIsOpaque();
    testSubsampledChromaFollowsThePicture();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
