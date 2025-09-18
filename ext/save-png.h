
#pragma once

#include "../core/BitmapRef.hpp"

#ifndef MSDFGEN_DISABLE_PNG

namespace msdfgen {

/// Saves the bitmap as a PNG file.
bool savePng(BitmapConstSection<byte, 1> bitmap, const char *filename);
bool savePng(BitmapConstSection<byte, 3> bitmap, const char *filename);
bool savePng(BitmapConstSection<byte, 4> bitmap, const char *filename);
bool savePng(BitmapConstSection<float, 1> bitmap, const char *filename);
bool savePng(BitmapConstSection<float, 3> bitmap, const char *filename);
bool savePng(BitmapConstSection<float, 4> bitmap, const char *filename);

}

#endif
