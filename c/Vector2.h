
#pragma once

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef double MSDFGEN_real;

typedef struct {
    MSDFGEN_real x, y;
} MSDFGEN_Vector2;

inline MSDFGEN_real MSDFGEN_Vector2_squaredLength(MSDFGEN_Vector2 v) {
    return v.x*v.x+v.y*v.y;
}

inline MSDFGEN_real MSDFGEN_Vector2_dot(MSDFGEN_Vector2 a, MSDFGEN_Vector2 b) {
    return a.x*b.x+a.y*b.y;
}

inline MSDFGEN_real MSDFGEN_Vector2_cross(MSDFGEN_Vector2 a, MSDFGEN_Vector2 b) {
    return a.x*b.y-a.y*b.x;
}

inline MSDFGEN_Vector2 MSDFGEN_Vector2_scale(MSDFGEN_real s, MSDFGEN_Vector2 v) {
    v.x *= s;
    v.y *= s;
    return v;
}

inline MSDFGEN_Vector2 MSDFGEN_Vector2_sum(MSDFGEN_Vector2 a, MSDFGEN_Vector2 b) {
    MSDFGEN_Vector2 result;
    result.x = a.x+b.x;
    result.y = a.y+b.y;
    return result;
}

inline MSDFGEN_Vector2 MSDFGEN_Vector2_difference(MSDFGEN_Vector2 a, MSDFGEN_Vector2 b) {
    MSDFGEN_Vector2 result;
    result.x = a.x-b.x;
    result.y = a.y-b.y;
    return result;
}

inline MSDFGEN_Vector2 MSDFGEN_Vector2_normalizeOrZero(MSDFGEN_Vector2 v) {
    MSDFGEN_real length = sqrt(MSDFGEN_Vector2_squaredLength(v));
    if (length != (MSDFGEN_real) 0) {
        v.x /= length;
        v.y /= length;
    }
    return v;
}

#ifdef __cplusplus
}
#endif
