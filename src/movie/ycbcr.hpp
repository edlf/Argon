// Y'CbCr (BT.601, studio range: what Theora stores) to BGRX for the video texture. The chroma planes may be half
// size across and / or down (4:2:0, 4:2:2) or full size (4:4:4).
#pragma once
#include <cstdint>

struct YCbCrPlane
{
    const uint8_t *data = nullptr;   // the first row
    int stride = 0;                  // bytes from one row to the next
};

struct YCbCrImage
{
    YCbCrPlane y, cb, cr;
    int chromaShiftX = 1, chromaShiftY = 1;   // 4:2:0
};

struct PictureRect
{
    int x = 0, y = 0, width = 0, height = 0;   // the shown part of the image, from its top-left corner
};

// `bgrx` takes picture.width x picture.height pixels, top-down, 4 bytes each (the 4th set to 255)
void convertToBgrx(const YCbCrImage &image, const PictureRect &picture, uint8_t *bgrx);
