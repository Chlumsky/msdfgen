
#include "save-fl32.h"

#include <cstdio>

namespace msdfgen {

// Requires byte reversal for floats on big-endian platform
#ifndef __BIG_ENDIAN__

template <int N>
bool saveFl32(BitmapConstSection<float, N> bitmap, const char *filename) {
    if (FILE *f = fopen(filename, "wb")) {
        bitmap.reorient(Y_UPWARD);
        byte header[16] = { byte('F'), byte('L'), byte('3'), byte('2') };
        header[4] = byte(bitmap.height);
        header[5] = byte(bitmap.height>>8);
        header[6] = byte(bitmap.height>>16);
        header[7] = byte(bitmap.height>>24);
        header[8] = byte(bitmap.width);
        header[9] = byte(bitmap.width>>8);
        header[10] = byte(bitmap.width>>16);
        header[11] = byte(bitmap.width>>24);
        header[12] = byte(N);
        fwrite(header, 1, 16, f);
        for (int y = 0; y < bitmap.height; ++y)
            fwrite(bitmap(0, y), sizeof(float), N*bitmap.width, f);
        fclose(f);
        return true;
    }
    return false;
}

template bool saveFl32(BitmapConstSection<float, 1> bitmap, const char *filename);
template bool saveFl32(BitmapConstSection<float, 2> bitmap, const char *filename);
template bool saveFl32(BitmapConstSection<float, 3> bitmap, const char *filename);
template bool saveFl32(BitmapConstSection<float, 4> bitmap, const char *filename);

#endif

}
