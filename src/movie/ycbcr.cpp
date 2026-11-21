#include "movie/ycbcr.hpp"

#include <algorithm>

namespace
{

uint8_t toByte(int value)
{
    return uint8_t(std::clamp(value, 0, 255));
}

// BT.601 studio range in 8.8 fixed point: Y' 16..235, Cb / Cr 16..240
void writePixel(int y, int cb, int cr, uint8_t *bgrx)
{
    const int luma = 298 * (y - 16), blue = cb - 128, red = cr - 128;
    bgrx[0] = toByte((luma + 516 * blue + 128) >> 8);
    bgrx[1] = toByte((luma - 100 * blue - 208 * red + 128) >> 8);
    bgrx[2] = toByte((luma + 409 * red + 128) >> 8);
    bgrx[3] = 255;
}

uint8_t sampleAt(const YCbCrPlane &plane, int x, int y)
{
    return plane.data[y * plane.stride + x];
}

}  // namespace

void convertToBgrx(const YCbCrImage &image, const PictureRect &picture, uint8_t *bgrx)
{
    for (int row = 0; row < picture.height; ++row)
    {
        const int y = picture.y + row, chromaY = y >> image.chromaShiftY;
        for (int column = 0; column < picture.width; ++column)
        {
            const int x = picture.x + column, chromaX = x >> image.chromaShiftX;
            writePixel(sampleAt(image.y, x, y), sampleAt(image.cb, chromaX, chromaY), sampleAt(image.cr, chromaX, chromaY), bgrx);
            bgrx += 4;
        }
    }
}
