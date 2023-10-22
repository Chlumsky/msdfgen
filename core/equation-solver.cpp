
#include "equation-solver.h"

#define _USE_MATH_DEFINES
#include <cmath>

#define LARGE_RATIO ::msdfgen::real(1e10)

#ifndef MSDFGEN_CUBE_ROOT
#define MSDFGEN_CUBE_ROOT(x) pow((x), ::msdfgen::real(1)/::msdfgen::real(3))
#endif

namespace msdfgen {

int solveQuadratic(real x[2], real a, real b, real c) {
    // a == 0 -> linear equation
    if (a == real(0) || fabs(b) > LARGE_RATIO*fabs(a)) {
        // a == 0, b == 0 -> no solution
        if (b == real(0)) {
            if (c == real(0))
                return -1; // 0 == 0
            return 0;
        }
        x[0] = -c/b;
        return 1;
    }
    real dscr = b*b-real(4)*a*c;
    if (dscr > real(0)) {
        dscr = sqrt(dscr);
        x[0] = (-b+dscr)/(a+a);
        x[1] = (-b-dscr)/(a+a);
        return 2;
    } else if (dscr == 0) {
        x[0] = -b/(a+a);
        return 1;
    } else
        return 0;
}

static int solveCubicNormed(real x[3], real a, real b, real c) {
    real a2 = a*a;
    real q = real(1)/real(9)*(a2-real(3)*b);
    real r = real(1)/real(54)*(a*(real(2)*a2-real(9)*b)+real(27)*c);
    real r2 = r*r;
    real q3 = q*q*q;
    a *= real(1)/real(3);
    if (r2 < q3) {
        real t = r/sqrt(q3);
        if (t < real(-1)) t = -1;
        if (t > real(1)) t = 1;
        t = real(1)/real(3)*acos(t);
        q = real(-2)*sqrt(q);
        x[0] = q*cos(t)-a;
        x[1] = q*cos(t+real(2)/real(3)*real(M_PI))-a;
        x[2] = q*cos(t-real(2)/real(3)*real(M_PI))-a;
        return 3;
    } else {
        real u = (r < real(0) ? real(1) : real(-1))*MSDFGEN_CUBE_ROOT(fabs(r)+sqrt(r2-q3));
        real v = u == real(0) ? real(0) : q/u;
        x[0] = (u+v)-a;
        if (u == v || LARGE_RATIO*fabs(u-v) < fabs(u+v)) {
            x[1] = real(-.5)*(u+v)-a;
            return 2;
        }
        return 1;
    }
}

int solveCubic(real x[3], real a, real b, real c, real d) {
    if (a != 0) {
        real bn = b/a;
        if (bn*bn < LARGE_RATIO) // Above this ratio, the numerical error gets larger than if we treated a as zero
            return solveCubicNormed(x, bn, c/a, d/a);
    }
    return solveQuadratic(x, b, c, d);
}

}
