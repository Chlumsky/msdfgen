
#pragma once

#include <cmath>
#include "types.h"
#include "Vector2.hpp"

// Parameters for iterative search of closest point on a cubic Bezier curve. Increase for higher precision.
#define MSDFGEN_CUBIC_SEARCH_STARTS 4
#define MSDFGEN_CUBIC_SEARCH_STEPS 4

#define MSDFGEN_QUADRATIC_RATIO_LIMIT ::msdfgen::real(1e8)

#ifndef MSDFGEN_CUBE_ROOT
#define MSDFGEN_CUBE_ROOT(x) pow((x), ::msdfgen::real(1)/::msdfgen::real(3))
#endif

namespace msdfgen {

/**
 * Returns the parameter for the quadratic Bezier curve (P0, P1, P2) for the point closest to point P. May be outside the (0, 1) range.
 * p = P-P0
 * q = 2*P1-2*P0
 * r = P2-2*P1+P0
 */
inline real quadraticNearPoint(const Vector2 p, const Vector2 q, const Vector2 r) {
    real qq = q.squaredLength();
    real rr = r.squaredLength();
    if (qq >= MSDFGEN_QUADRATIC_RATIO_LIMIT*rr)
        return dotProduct(p, q)/qq;
    real norm = real(.5)/rr;
    real a = real(3)*norm*dotProduct(q, r);
    real b = norm*(qq-real(2)*dotProduct(p, r));
    real c = norm*dotProduct(p, q);
    real aa = a*a;
    real g = real(1)/real(9)*(aa-real(3)*b);
    real h = real(1)/real(54)*(a*(aa+aa-real(9)*b)-real(27)*c);
    real hh = h*h;
    real ggg = g*g*g;
    a *= real(1)/real(3);
    if (hh < ggg) {
        real u = real(1)/real(3)*acos(h/sqrt(ggg));
        g = real(-2)*sqrt(g);
        if (h >= real(0)) {
            real t = g*cos(u)-a;
            if (t >= real(0))
                return t;
            return g*cos(u+real(2.0943951023931954923))-a; // 2.094 = PI*2/3
        } else {
            real t = g*cos(u+real(2.0943951023931954923))-a;
            if (t <= real(1))
                return t;
            return g*cos(u)-a;
        }
    }
    real s = (h < real(0) ? real(1) : real(-1))*MSDFGEN_CUBE_ROOT(fabs(h)+sqrt(hh-ggg));
    return s+g/s-a;
}

/**
 * Returns the parameter for the cubic Bezier curve (P0, P1, P2, P3) for the point closest to point P. Squared distance is provided as optional output parameter.
 * p = P-P0
 * q = 3*P1-3*P0
 * r = 3*P2-6*P1+3*P0
 * s = P3-3*P2+3*P1-P0
 */
inline real cubicNearPoint(const Vector2 p, const Vector2 q, const Vector2 r, const Vector2 s, real &squaredDistance) {
    squaredDistance = p.squaredLength();
    real bestT = 0;
    for (int i = 0; i <= MSDFGEN_CUBIC_SEARCH_STARTS; ++i) {
        real t = real(1)/real(MSDFGEN_CUBIC_SEARCH_STARTS)*real(i);
        Vector2 curP = p-(q+(r+s*t)*t)*t;
        for (int step = 0; step < MSDFGEN_CUBIC_SEARCH_STEPS; ++step) {
            Vector2 d0 = q+(r+r+real(3)*s*t)*t;
            Vector2 d1 = r+r+real(6)*s*t;
            t += dotProduct(curP, d0)/(d0.squaredLength()-dotProduct(curP, d1));
            if (t <= real(0) || t >= real(1))
                break;
            curP = p-(q+(r+s*t)*t)*t;
            real curSquaredDistance = curP.squaredLength();
            if (curSquaredDistance < squaredDistance) {
                squaredDistance = curSquaredDistance;
                bestT = t;
            }
        }
    }
    return bestT;
}

inline real cubicNearPoint(const Vector2 p, const Vector2 q, const Vector2 r, const Vector2 s) {
    real squaredDistance;
    return cubicNearPoint(p, q, r, s, squaredDistance);
}

}
