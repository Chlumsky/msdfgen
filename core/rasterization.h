
#pragma once

#include "Vector2.h"
#include "Shape.h"
#include "Bitmap.h"

namespace msdfgen {

/// Fill rule dictates how intersection total is interpreted during rasterization.
enum FillRule {
    FILL_NONZERO,
    FILL_ODD, // "even-odd"
    FILL_POSITIVE,
    FILL_NEGATIVE
};

/// Rasterizes the shape into a monochrome bitmap.
void rasterize(Bitmap<float> &output, const Shape &shape, const Vector2 &scale, const Vector2 &translate, FillRule fillRule = FILL_NONZERO);
/// Fixes the sign of the input signed distance field, so that it matches the shape's rasterized fill.
void distanceSignCorrection(Bitmap<float> &sdf, const Shape &shape, const Vector2 &scale, const Vector2 &translate, FillRule fillRule = FILL_NONZERO);
void distanceSignCorrection(Bitmap<FloatRGB> &sdf, const Shape &shape, const Vector2 &scale, const Vector2 &translate, FillRule fillRule = FILL_NONZERO);

}
