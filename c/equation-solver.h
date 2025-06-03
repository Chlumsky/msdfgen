
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// ax^2 + bx + c = 0
int MSDFGEN_solveQuadratic(double x[2], double a, double b, double c);

// ax^3 + bx^2 + cx + d = 0
int MSDFGEN_solveCubic(double x[3], double a, double b, double c, double d);

#ifdef __cplusplus
}
#endif
