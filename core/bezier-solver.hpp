
#pragma once

#include <cmath>
#include "Vector2.hpp"

// Parameters for iterative search of closest point on a cubic Bezier curve. Increase for higher precision.
#define MSDFGEN_CUBIC_SEARCH_STARTS 4
#define MSDFGEN_CUBIC_SEARCH_STEPS 4

#define MSDFGEN_QUADRATIC_RATIO_LIMIT 1e8

#ifndef MSDFGEN_CUBE_ROOT
#define MSDFGEN_CUBE_ROOT(x) pow((x), 1/3.)
#endif

namespace msdfgen {

/**
 * Returns the parameter for the quadratic Bezier curve (P0, P1, P2) for the point closest to point P. May be outside the (0, 1) range.
 * p = P-P0
 * q = 2*P1-2*P0
 * r = P2-2*P1+P0
 */
inline double quadraticNearPoint(const Vector2 p, const Vector2 q, const Vector2 r) {
    double qq = q.squaredLength();
    double rr = r.squaredLength();
    if (qq >= MSDFGEN_QUADRATIC_RATIO_LIMIT*rr)
        return dotProduct(p, q)/qq;
    double norm = .5/rr;
    double a = 3*norm*dotProduct(q, r);
    double b = norm*(qq-2*dotProduct(p, r));
    double c = norm*dotProduct(p, q);
    double aa = a*a;
    double g = 1/9.*(aa-3*b);
    double h = 1/54.*(a*(aa+aa-9*b)-27*c);
    double hh = h*h;
    double ggg = g*g*g;
    a *= 1/3.;
    if (hh < ggg) {
        double u = 1/3.*acos(h/sqrt(ggg));
        g = -2*sqrt(g);
        if (h >= 0) {
            double t = g*cos(u)-a;
            if (t >= 0)
                return t;
            return g*cos(u+2.0943951023931954923)-a; // 2.094 = PI*2/3
        } else {
            double t = g*cos(u+2.0943951023931954923)-a;
            if (t <= 1)
                return t;
            return g*cos(u)-a;
        }
    }
    double s = (h < 0 ? 1. : -1.)*MSDFGEN_CUBE_ROOT(fabs(h)+sqrt(hh-ggg));
    return s+g/s-a;
}

/**
 * Returns the parameter for the cubic Bezier curve (P0, P1, P2, P3) for the point closest to point P. Squared distance is provided as optional output parameter.
 * p = P-P0
 * q = 3*P1-3*P0
 * r = 3*P2-6*P1+3*P0
 * s = P3-3*P2+3*P1-P0
 */
inline double cubicNearPoint(const Vector2 p, const Vector2 q, const Vector2 r, const Vector2 s, double &squaredDistance) {
    squaredDistance = p.squaredLength();
    double bestT = 0;
    for (int i = 0; i <= MSDFGEN_CUBIC_SEARCH_STARTS; ++i) {
        double t = 1./MSDFGEN_CUBIC_SEARCH_STARTS*i;
        Vector2 curP = p-(q+(r+s*t)*t)*t;
        for (int step = 0; step < MSDFGEN_CUBIC_SEARCH_STEPS; ++step) {
            Vector2 d0 = q+(r+r+3*s*t)*t;
            Vector2 d1 = r+r+6*s*t;
            t += dotProduct(curP, d0)/(d0.squaredLength()-dotProduct(curP, d1));
            if (t <= 0 || t >= 1)
                break;
            curP = p-(q+(r+s*t)*t)*t;
            double curSquaredDistance = curP.squaredLength();
            if (curSquaredDistance < squaredDistance) {
                squaredDistance = curSquaredDistance;
                bestT = t;
            }
        }
    }
    return bestT;
}

inline double cubicNearPoint(const Vector2 p, const Vector2 q, const Vector2 r, const Vector2 s) {
    double squaredDistance;
    return cubicNearPoint(p, q, r, s, squaredDistance);
}

}
