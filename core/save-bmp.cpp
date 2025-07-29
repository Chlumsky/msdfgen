
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "save-bmp.h"

#include <cstdio>

#ifdef MSDFGEN_USE_CPP11
    #include <cstdint>
#else
namespace msdfgen {
    typedef int int32_t;
    typedef unsigned uint32_t;
    typedef unsigned short uint16_t;
}
#endif

#include "pixel-conversion.hpp"

namespace msdfgen {

static const byte BMP_LINEAR_COLOR_SPACE_SPECIFICATION[48] = {
    0xf8, 0xc2, 0x64, 0x1a, 0x08, 0x3d, 0x9b, 0x0d, 0x11, 0x36, 0x3c, 0x01,
    0x1c, 0xeb, 0xe2, 0x16, 0x39, 0xd6, 0xc5, 0x2d, 0x09, 0xf9, 0xa0, 0x07,
    0xdf, 0x4f, 0x8d, 0x0b, 0xc0, 0xec, 0x9e, 0x04, 0xf4, 0xfd, 0xd4, 0x3c,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
};

template <typename T>
bool writeLE(FILE *file, T value);

template <>
bool writeLE(FILE *f, uint16_t value) {
    byte bytes[2] = { byte(value), byte(value>>8) };
    return fwrite(bytes, 1, 2, f) == 2;
}
template <>
bool writeLE(FILE *f, uint32_t value) {
    byte bytes[4] = { byte(value), byte(value>>8), byte(value>>16), byte(value>>24) };
    return fwrite(bytes, 1, 4, f) == 4;
}
template <>
bool writeLE(FILE *f, int32_t value) {
    byte bytes[4] = { byte(value), byte(value>>8), byte(value>>16), byte(value>>24) };
    return fwrite(bytes, 1, 4, f) == 4;
}

static bool writeBmpHeader(FILE *file, int bytesPerPixel, int width, int height, int &paddedWidth) {
    paddedWidth = (bytesPerPixel*width+3)&~3; // Ceil to multiple of 4 bytes
    uint32_t colorTableEntries = bytesPerPixel == 1 ? 256 : 0;
    uint32_t bitmapStart = 14+108+4*colorTableEntries;
    uint32_t bitmapSize = paddedWidth*height;
    uint32_t fileSize = bitmapStart+bitmapSize;

    // BMP file header
    writeLE<uint16_t>(file, 0x4d42u); // "BM"
    writeLE<uint32_t>(file, fileSize);
    writeLE<uint16_t>(file, 0);
    writeLE<uint16_t>(file, 0);
    writeLE<uint32_t>(file, bitmapStart);

    // DIB header (BITMAPV4HEADER)
    writeLE<uint32_t>(file, 108);
    writeLE<int32_t>(file, width);
    writeLE<int32_t>(file, height);
    writeLE<uint16_t>(file, 1); // planes
    writeLE<uint16_t>(file, uint16_t(8*bytesPerPixel));
    writeLE<uint32_t>(file, bytesPerPixel == 4 ? 3 : 0); // 0 = BI_RGB, 3 = BI_BITFIELDS
    writeLE<uint32_t>(file, bitmapSize);
    writeLE<uint32_t>(file, 2835); // 72 dpi as pixels/meter
    writeLE<uint32_t>(file, 2835);
    writeLE<uint32_t>(file, colorTableEntries);
    writeLE<uint32_t>(file, colorTableEntries);
    writeLE<uint32_t>(file, 0x00ff0000u); // Red channel bit mask
    writeLE<uint32_t>(file, 0x0000ff00u); // Green channel bit mask
    writeLE<uint32_t>(file, 0x000000ffu); // Blue channel bit mask
    writeLE<uint32_t>(file, bytesPerPixel == 4 ? 0xff000000u : 0u); // Alpha channel bit mask
    writeLE<uint32_t>(file, 0); // LCS_CALIBRATED_RGB
    fwrite(BMP_LINEAR_COLOR_SPACE_SPECIFICATION, 1, 48, file);

    // Use indexed color for single channel - color table just lists grayscale colors (#000000, #010101, ... #FFFFFF)
    if (bytesPerPixel == 1) {
        for (uint32_t grayColor = 0; grayColor < 0x01000000u; grayColor += 0x00010101u)
            writeLE<uint32_t>(file, grayColor|0xff000000u);
    }

    return true;
}

bool saveBmp(const BitmapConstRef<byte, 1> &bitmap, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file)
        return false;

    int paddedWidth;
    writeBmpHeader(file, 1, bitmap.width, bitmap.height, paddedWidth);

    byte padding[4] = { };
    int padLength = paddedWidth-bitmap.width;
    for (int y = 0; y < bitmap.height; ++y) {
        fwrite(bitmap(0, y), sizeof(byte), bitmap.width, file);
        fwrite(padding, 1, padLength, file);
    }

    return !fclose(file);
}

bool saveBmp(const BitmapConstRef<byte, 3> &bitmap, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file)
        return false;

    int paddedWidth;
    writeBmpHeader(file, 3, bitmap.width, bitmap.height, paddedWidth);

    byte padding[4] = { };
    int padLength = paddedWidth-3*bitmap.width;
    for (int y = 0; y < bitmap.height; ++y) {
        for (int x = 0; x < bitmap.width; ++x) {
            byte bgr[3] = {
                bitmap(x, y)[2],
                bitmap(x, y)[1],
                bitmap(x, y)[0]
            };
            fwrite(bgr, sizeof(byte), 3, file);
        }
        fwrite(padding, 1, padLength, file);
    }

    return !fclose(file);
}

bool saveBmp(const BitmapConstRef<byte, 4> &bitmap, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file)
        return false;

    int dummyPaddedWidth;
    writeBmpHeader(file, 4, bitmap.width, bitmap.height, dummyPaddedWidth);

    for (int y = 0; y < bitmap.height; ++y) {
        for (int x = 0; x < bitmap.width; ++x) {
            byte bgra[4] = {
                bitmap(x, y)[2],
                bitmap(x, y)[1],
                bitmap(x, y)[0],
                bitmap(x, y)[3]
            };
            fwrite(bgra, sizeof(byte), 4, file);
        }
    }

    return !fclose(file);
}

bool saveBmp(const BitmapConstRef<float, 1> &bitmap, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file)
        return false;

    int paddedWidth;
    writeBmpHeader(file, 1, bitmap.width, bitmap.height, paddedWidth);

    byte padding[4] = { };
    int padLength = paddedWidth-bitmap.width;
    for (int y = 0; y < bitmap.height; ++y) {
        for (int x = 0; x < bitmap.width; ++x) {
            byte px = pixelFloatToByte(*bitmap(x, y));
            fwrite(&px, sizeof(byte), 1, file);
        }
        fwrite(padding, 1, padLength, file);
    }

    return !fclose(file);
}

bool saveBmp(const BitmapConstRef<float, 3> &bitmap, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file)
        return false;

    int paddedWidth;
    writeBmpHeader(file, 3, bitmap.width, bitmap.height, paddedWidth);

    byte padding[4] = { };
    int padLength = paddedWidth-3*bitmap.width;
    for (int y = 0; y < bitmap.height; ++y) {
        for (int x = 0; x < bitmap.width; ++x) {
            byte bgr[3] = {
                pixelFloatToByte(bitmap(x, y)[2]),
                pixelFloatToByte(bitmap(x, y)[1]),
                pixelFloatToByte(bitmap(x, y)[0])
            };
            fwrite(bgr, sizeof(byte), 3, file);
        }
        fwrite(padding, 1, padLength, file);
    }

    return !fclose(file);
}

bool saveBmp(const BitmapConstRef<float, 4> &bitmap, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file)
        return false;

    int dummyPaddedWidth;
    writeBmpHeader(file, 4, bitmap.width, bitmap.height, dummyPaddedWidth);

    for (int y = 0; y < bitmap.height; ++y) {
        for (int x = 0; x < bitmap.width; ++x) {
            byte bgra[4] = {
                pixelFloatToByte(bitmap(x, y)[2]),
                pixelFloatToByte(bitmap(x, y)[1]),
                pixelFloatToByte(bitmap(x, y)[0]),
                pixelFloatToByte(bitmap(x, y)[3])
            };
            fwrite(bgra, sizeof(byte), 4, file);
        }
    }

    return !fclose(file);
}

}
