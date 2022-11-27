
#include "save-png.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../core/pixel-conversion.hpp"

#ifdef MSDFGEN_USE_LIBPNG

#include <png.h>

namespace msdfgen {

class PngGuard {
    png_structp png;
    png_infop info;
    FILE *file;

public:
    inline PngGuard(png_structp png, png_infop info) : png(png), info(info), file(NULL) { }
    inline ~PngGuard() {
        png_destroy_write_struct(&png, &info);
        fclose(file);
    }
    inline void setFile(FILE *file) {
        this->file = file;
    }

};

static void pngIgnoreError(png_structp, png_const_charp) { }

static void pngWrite(png_structp png, png_bytep data, png_size_t length) {
    if (fwrite(data, 1, length, reinterpret_cast<FILE *>(png_get_io_ptr(png))) != length)
        png_error(png, "File write error");
}

static void pngFlush(png_structp png) {
    fflush(reinterpret_cast<FILE *>(png_get_io_ptr(png)));
}

static bool pngSave(const byte *pixels, int width, int height, int channels, int colorType, const char *filename) {
    if (!(pixels && width && height))
        return false;
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, &pngIgnoreError, &pngIgnoreError);
    if (!png)
        return false;
    png_infop info = png_create_info_struct(png);
    PngGuard guard(png, info);
    if (!info)
        return false;
    FILE *file = fopen(filename, "wb");
    if (!file)
        return false;
    guard.setFile(file);
    std::vector<const byte *> rows(height);
    for (int y = 0; y < height; ++y)
        rows[y] = pixels+channels*width*(height-y-1);
    if (setjmp(png_jmpbuf(png)))
        return false;
    png_set_write_fn(png, file, &pngWrite, &pngFlush);
    png_set_IHDR(png, info, width, height, 8, colorType, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_compression_level(png, 9);
    png_set_rows(png, info, const_cast<png_bytepp>(&rows[0]));
    png_write_png(png, info, PNG_TRANSFORM_IDENTITY, NULL);
    return true;
}

static bool pngSave(const float *pixels, int width, int height, int channels, int colorType, const char *filename) {
    if (!(pixels && width && height))
        return false;
    int subpixels = channels*width*height;
    std::vector<byte> bytePixels(subpixels);
    for (int i = 0; i < subpixels; ++i)
        bytePixels[i] = pixelFloatToByte(pixels[i]);
    return pngSave(&bytePixels[0], width, height, channels, colorType, filename);
}

bool savePng(const BitmapConstRef<byte, 1> &bitmap, const char *filename) {
    return pngSave(bitmap.pixels, bitmap.width, bitmap.height, 1, PNG_COLOR_TYPE_GRAY, filename);
}

bool savePng(const BitmapConstRef<byte, 3> &bitmap, const char *filename) {
    return pngSave(bitmap.pixels, bitmap.width, bitmap.height, 3, PNG_COLOR_TYPE_RGB, filename);
}

bool savePng(const BitmapConstRef<byte, 4> &bitmap, const char *filename) {
    return pngSave(bitmap.pixels, bitmap.width, bitmap.height, 4, PNG_COLOR_TYPE_RGB_ALPHA, filename);
}

bool savePng(const BitmapConstRef<float, 1> &bitmap, const char *filename) {
    return pngSave(bitmap.pixels, bitmap.width, bitmap.height, 1, PNG_COLOR_TYPE_GRAY, filename);
}

bool savePng(const BitmapConstRef<float, 3> &bitmap, const char *filename) {
    return pngSave(bitmap.pixels, bitmap.width, bitmap.height, 3, PNG_COLOR_TYPE_RGB, filename);
}

bool savePng(const BitmapConstRef<float, 4> &bitmap, const char *filename) {
    return pngSave(bitmap.pixels, bitmap.width, bitmap.height, 4, PNG_COLOR_TYPE_RGB_ALPHA, filename);
}

}

#endif

#ifdef MSDFGEN_USE_LODEPNG

#include <lodepng.h>

namespace msdfgen {

bool savePng(const BitmapConstRef<byte, 1> &bitmap, const char *filename) {
    std::vector<byte> pixels(bitmap.width*bitmap.height);
    for (int y = 0; y < bitmap.height; ++y)
        memcpy(&pixels[bitmap.width*y], bitmap(0, bitmap.height-y-1), bitmap.width);
    return !lodepng::encode(filename, pixels, bitmap.width, bitmap.height, LCT_GREY);
}

bool savePng(const BitmapConstRef<byte, 3> &bitmap, const char *filename) {
    std::vector<byte> pixels(3*bitmap.width*bitmap.height);
    for (int y = 0; y < bitmap.height; ++y)
        memcpy(&pixels[3*bitmap.width*y], bitmap(0, bitmap.height-y-1), 3*bitmap.width);
    return !lodepng::encode(filename, pixels, bitmap.width, bitmap.height, LCT_RGB);
}

bool savePng(const BitmapConstRef<byte, 4> &bitmap, const char *filename) {
    std::vector<byte> pixels(4*bitmap.width*bitmap.height);
    for (int y = 0; y < bitmap.height; ++y)
        memcpy(&pixels[4*bitmap.width*y], bitmap(0, bitmap.height-y-1), 4*bitmap.width);
    return !lodepng::encode(filename, pixels, bitmap.width, bitmap.height, LCT_RGBA);
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

bool savePng(const BitmapConstRef<float, 4> &bitmap, const char *filename) {
    std::vector<byte> pixels(4*bitmap.width*bitmap.height);
    std::vector<byte>::iterator it = pixels.begin();
    for (int y = bitmap.height-1; y >= 0; --y)
        for (int x = 0; x < bitmap.width; ++x) {
            *it++ = pixelFloatToByte(bitmap(x, y)[0]);
            *it++ = pixelFloatToByte(bitmap(x, y)[1]);
            *it++ = pixelFloatToByte(bitmap(x, y)[2]);
            *it++ = pixelFloatToByte(bitmap(x, y)[3]);
        }
    return !lodepng::encode(filename, pixels, bitmap.width, bitmap.height, LCT_RGBA);
}

}

#endif
