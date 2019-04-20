
#define _CRT_SECURE_NO_WARNINGS

#include "save-tiff.h"

#include <cstdio>

#ifdef MSDFGEN_USE_CPP11
    #include <cstdint>
#else
    typedef int int32_t;
    typedef unsigned uint32_t;
    typedef unsigned short uint16_t;
    typedef unsigned char uint8_t;
#endif

namespace msdfgen {

template <typename T>
static bool writeValue(FILE *file, T value) {
    return fwrite(&value, sizeof(T), 1, file) == 1;
}

static bool writeTiffHeader(FILE *file, int width, int height, int channels) {
    #ifdef __BIG_ENDIAN__
        writeValue<uint16_t>(file, 0x4d4du);
    #else
        writeValue<uint16_t>(file, 0x4949u);
    #endif
    writeValue<uint16_t>(file, 42);
    writeValue<uint32_t>(file, 0x0008u); // Offset of first IFD
    // Offset = 0x0008

    writeValue<uint16_t>(file, 15); // Number of IFD entries

    // ImageWidth
    writeValue<uint16_t>(file, 0x0100u);
    writeValue<uint16_t>(file, 0x0004u);
    writeValue<uint32_t>(file, 1);
    writeValue<int32_t>(file, width);
    // ImageLength
    writeValue<uint16_t>(file, 0x0101u);
    writeValue<uint16_t>(file, 0x0004u);
    writeValue<uint32_t>(file, 1);
    writeValue<int32_t>(file, height);
    // BitsPerSample
    writeValue<uint16_t>(file, 0x0102u);
    writeValue<uint16_t>(file, 0x0003u);
    writeValue<uint32_t>(file, channels);
    if (channels == 3)
        writeValue<uint32_t>(file, 0x00c2u); // Offset of 32, 32, 32
    else {
        writeValue<uint16_t>(file, 32);
        writeValue<uint16_t>(file, 0);
    }
    // Compression
    writeValue<uint16_t>(file, 0x0103u);
    writeValue<uint16_t>(file, 0x0003u);
    writeValue<uint32_t>(file, 1);
    writeValue<uint16_t>(file, 1);
    writeValue<uint16_t>(file, 0);
    // PhotometricInterpretation
    writeValue<uint16_t>(file, 0x0106u);
    writeValue<uint16_t>(file, 0x0003u);
    writeValue<uint32_t>(file, 1);
    writeValue<uint16_t>(file, channels == 3 ? 2 : 1);
    writeValue<uint16_t>(file, 0);
    // StripOffsets
    writeValue<uint16_t>(file, 0x0111u);
    writeValue<uint16_t>(file, 0x0004u);
    writeValue<uint32_t>(file, 1);
    writeValue<uint32_t>(file, channels == 3 ? 0x00f6u : 0x00d2u); // Offset of pixel data
    // SamplesPerPixel
    writeValue<uint16_t>(file, 0x0115u);
    writeValue<uint16_t>(file, 0x0003u);
    writeValue<uint32_t>(file, 1);
    writeValue<uint16_t>(file, channels);
    writeValue<uint16_t>(file, 0);
    // RowsPerStrip
    writeValue<uint16_t>(file, 0x0116u);
    writeValue<uint16_t>(file, 0x0004u);
    writeValue<uint32_t>(file, 1);
    writeValue<int32_t>(file, height);
    // StripByteCounts
    writeValue<uint16_t>(file, 0x0117u);
    writeValue<uint16_t>(file, 0x0004u);
    writeValue<uint32_t>(file, 1);
    writeValue<int32_t>(file, sizeof(float)*channels*width*height);
    // XResolution
    writeValue<uint16_t>(file, 0x011au);
    writeValue<uint16_t>(file, 0x0005u);
    writeValue<uint32_t>(file, 1);
    writeValue<uint32_t>(file, channels == 3 ? 0x00c8u : 0x00c2u); // Offset of 300, 1
    // YResolution
    writeValue<uint16_t>(file, 0x011bu);
    writeValue<uint16_t>(file, 0x0005u);
    writeValue<uint32_t>(file, 1);
    writeValue<uint32_t>(file, channels == 3 ? 0x00d0u : 0x00cau); // Offset of 300, 1
    // ResolutionUnit
    writeValue<uint16_t>(file, 0x0128u);
    writeValue<uint16_t>(file, 0x0003u);
    writeValue<uint32_t>(file, 1);
    writeValue<uint16_t>(file, 2);
    writeValue<uint16_t>(file, 0);
    // SampleFormat
    writeValue<uint16_t>(file, 0x0153u);
    writeValue<uint16_t>(file, 0x0003u);
    writeValue<uint32_t>(file, channels);
    if (channels == 3)
        writeValue<uint32_t>(file, 0x00d8u); // Offset of 3, 3, 3
    else {
        writeValue<uint16_t>(file, 3);
        writeValue<uint16_t>(file, 0);
    }
    // SMinSampleValue
    writeValue<uint16_t>(file, 0x0154u);
    writeValue<uint16_t>(file, 0x000bu);
    writeValue<uint32_t>(file, channels);
    if (channels == 3)
        writeValue<uint32_t>(file, 0x00deu); // Offset of 0.f, 0.f, 0.f
    else
        writeValue<float>(file, 0.f);
    // SMaxSampleValue
    writeValue<uint16_t>(file, 0x0155u);
    writeValue<uint16_t>(file, 0x000bu);
    writeValue<uint32_t>(file, channels);
    if (channels == 3)
        writeValue<uint32_t>(file, 0x00eau); // Offset of 1.f, 1.f, 1.f
    else
        writeValue<float>(file, 1.f);
    // Offset = 0x00be

    writeValue<uint32_t>(file, 0);

    if (channels == 3) {
        // 0x00c2 BitsPerSample data
        writeValue<uint16_t>(file, 32);
        writeValue<uint16_t>(file, 32);
        writeValue<uint16_t>(file, 32);
        // 0x00c8 XResolution data
        writeValue<uint32_t>(file, 300);
        writeValue<uint32_t>(file, 1);
        // 0x00d0 YResolution data
        writeValue<uint32_t>(file, 300);
        writeValue<uint32_t>(file, 1);
        // 0x00d8 SampleFormat data
        writeValue<uint16_t>(file, 3);
        writeValue<uint16_t>(file, 3);
        writeValue<uint16_t>(file, 3);
        // 0x00de SMinSampleValue data
        writeValue<float>(file, 0.f);
        writeValue<float>(file, 0.f);
        writeValue<float>(file, 0.f);
        // 0x00ea SMaxSampleValue data
        writeValue<float>(file, 1.f);
        writeValue<float>(file, 1.f);
        writeValue<float>(file, 1.f);
        // Offset = 0x00f6
    } else {
        // 0x00c2 XResolution data
        writeValue<uint32_t>(file, 300);
        writeValue<uint32_t>(file, 1);
        // 0x00ca YResolution data
        writeValue<uint32_t>(file, 300);
        writeValue<uint32_t>(file, 1);
        // Offset = 0x00d2
    }

    return true;
}

bool saveTiff(const BitmapConstRef<float, 1> &bitmap, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file)
        return false;
    writeTiffHeader(file, bitmap.width, bitmap.height, 1);
    for (int y = bitmap.height-1; y >= 0; --y)
        fwrite(bitmap(0, y), sizeof(float), bitmap.width, file);
    return !fclose(file);
}

bool saveTiff(const BitmapConstRef<float, 3> &bitmap, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file)
        return false;
    writeTiffHeader(file, bitmap.width, bitmap.height, 3);
    for (int y = bitmap.height-1; y >= 0; --y)
        fwrite(bitmap(0, y), sizeof(float), 3*bitmap.width, file);
    return !fclose(file);
}

}
