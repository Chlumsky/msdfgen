
#include "save-png.h"

#include <lodepng.h>
#include "../core/pixel-conversion.hpp"

namespace msdfgen {

bool savePng(const BitmapConstRef<byte, 1> &bitmap, const char *filename) {
    return !lodepng::encode(filename, bitmap.pixels, bitmap.width, bitmap.height, LCT_GREY);
}

bool savePng(const BitmapConstRef<byte, 3> &bitmap, const char *filename) {
    return !lodepng::encode(filename, bitmap.pixels, bitmap.width, bitmap.height, LCT_RGB);
}

bool savePng(const BitmapConstRef<float, 1> &bitmap, const char *filename) {
    std::vector<byte> pixels(bitmap.width*bitmap.height);
    std::vector<byte>::iterator it = pixels.begin();
    for (int y = bitmap.height-1; y >= 0; --y)
        for (int x = 0; x < bitmap.width; ++x)
            *it++ = pixelFloatToByte(*bitmap(x, y));
    return !lodepng::encode(filename, pixels, bitmap.width, bitmap.height, LCT_GREY);
}

bool savePng(const BitmapConstRef<float, 3> &bitmap, const char *filename) {
    std::vector<byte> pixels(3*bitmap.width*bitmap.height);
    std::vector<byte>::iterator it = pixels.begin();
    for (int y = bitmap.height-1; y >= 0; --y)
        for (int x = 0; x < bitmap.width; ++x) {
            *it++ = pixelFloatToByte(bitmap(x, y)[0]);
            *it++ = pixelFloatToByte(bitmap(x, y)[1]);
            *it++ = pixelFloatToByte(bitmap(x, y)[2]);
        }
    return !lodepng::encode(filename, pixels, bitmap.width, bitmap.height, LCT_RGB);
}

}
