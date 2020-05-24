
#pragma once

#include "Vector2.h"
#include "Shape.h"
#include "BitmapRef.hpp"

namespace msdfgen {

void msdfInterpolationErrorCorrection(const BitmapRef<float, 3> &sdf, const Shape &shape, double range, const Vector2 &scale, const Vector2 &translate, bool overlapSupport = true);
void msdfInterpolationErrorCorrection(const BitmapRef<float, 4> &sdf, const Shape &shape, double range, const Vector2 &scale, const Vector2 &translate, bool overlapSupport = true);

}
