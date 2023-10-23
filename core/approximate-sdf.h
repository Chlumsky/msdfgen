
#pragma once

#include "Projection.h"
#include "Shape.h"
#include "BitmapRef.hpp"

namespace msdfgen {

// Fast SDF approximation (out of range values not computed)
void approximateSDF(const BitmapRef<float, 1> &output, const Shape &shape, const Projection &projection, double outerRange, double innerRange);
void approximateSDF(const BitmapRef<float, 1> &output, const Shape &shape, const Projection &projection, double range);

}
