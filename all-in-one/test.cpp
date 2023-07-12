
#include <cstdio>
#include "msdfgen.h"

using namespace msdfgen;

static bool saveRGBA(const char *filename, const BitmapConstRef<float, 4> &bitmap) {
    if (FILE *f = fopen(filename, "wb")) {
        char header[12];
        header[0] = 'R', header[1] = 'G', header[2] = 'B', header[3] = 'A';
        header[4] = char(bitmap.width>>24);
        header[5] = char(bitmap.width>>16);
        header[6] = char(bitmap.width>>8);
        header[7] = char(bitmap.width);
        header[8] = char(bitmap.height>>24);
        header[9] = char(bitmap.height>>16);
        header[10] = char(bitmap.height>>8);
        header[11] = char(bitmap.height);
        fwrite(header, 1, 12, f);
        Bitmap<byte, 4> byteBitmap(bitmap.width, bitmap.height);
        byte *d = (byte *) byteBitmap;
        for (const float *s = bitmap.pixels, *end = bitmap.pixels+4*bitmap.width*bitmap.height; s < end; ++d, ++s)
            *d = pixelFloatToByte(*s);
        fwrite((const byte *) byteBitmap, 1, 4*bitmap.width*bitmap.height, f);
        fclose(f);
        return true;
    }
    return false;
}

int main() {
#ifdef MSDFGEN_USE_FREETYPE
    bool success = false;
    if (FreetypeHandle *ft = initializeFreetype()) {
        if (FontHandle *font = loadFont(ft, "C:\\Windows\\Fonts\\arialbd.ttf")) {
            Shape shape;
            if (loadGlyph(shape, font, 'A')) {
                shape.normalize();
                edgeColoringByDistance(shape, 3.0);
                Bitmap<float, 4> msdf(32, 32);
                generateMTSDF(msdf, shape, 4.0, 1.0, Vector2(4.0, 4.0));
                success = saveRGBA("output.rgba", msdf);
            }
            destroyFont(font);
        }
        deinitializeFreetype(ft);
    }
    return success ? 0 : 1;
#else
    return 0;
#endif
}
