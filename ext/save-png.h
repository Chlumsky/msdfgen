
#pragma once

#include "../core/BitmapRef.hpp"

#ifndef MSDFGEN_DISABLE_PNG

namespace msdfgen {

/// Saves the bitmap as a PNG file.
bool savePng(const BitmapConstRef<byte, 1> &bitmap, const char *filename);
bool savePng(const BitmapConstRef<byte, 3> &bitmap, const char *filename);
bool savePng(const BitmapConstRef<byte, 4> &bitmap, const char *filename);
bool savePng(const BitmapConstRef<float, 1> &bitmap, const char *filename);
bool savePng(const BitmapConstRef<float, 3> &bitmap, const char *filename);
bool savePng(const BitmapConstRef<float, 4> &bitmap, const char *filename);

}

#endif
