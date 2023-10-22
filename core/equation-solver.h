
#pragma once

#include "types.h"

namespace msdfgen {

// ax^2 + bx + c = 0
int solveQuadratic(real x[2], real a, real b, real c);

// ax^3 + bx^2 + cx + d = 0
int solveCubic(real x[3], real a, real b, real c, real d);

}
