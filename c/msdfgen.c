
#include "msdfgen-c.h"

#include <math.h>
#include <float.h>
#include "equation-solver.h"

#define DISTANCE_DELTA_FACTOR 1.001

//#define MSDFGEN_IF_CACHE_AND(...) // disable cache
#define MSDFGEN_IF_CACHE_AND if

extern long long MSDFGEN_PERFSTATS_CACHE_TESTS;
extern long long MSDFGEN_PERFSTATS_CACHE_MISSES;

long long MSDFGEN_PERFSTATS_CACHE_TESTS = 0;
long long MSDFGEN_PERFSTATS_CACHE_MISSES = 0;

#define MSDFGEN_PERFSTATS_CACHE_TEST() (++MSDFGEN_PERFSTATS_CACHE_TESTS)
#define MSDFGEN_PERFSTATS_CACHE_MISS() (++MSDFGEN_PERFSTATS_CACHE_MISSES)

//#define NO_CACHE_AND_SQUARE_DISTANCE

#ifdef __cplusplus
extern "C" {
#endif

static int nonZeroSign(MSDFGEN_real x) {
    return x > (MSDFGEN_real) 0 ? 1 : -1;
}

static int crossNonZeroSign(MSDFGEN_Vector2 a, MSDFGEN_Vector2 b) {
    return a.x*b.y > b.x*a.y ? 1 : -1;
}

size_t MSDFGEN_TrueDistanceCache_size(const MSDFGEN_CompiledShape *shapePtr) {
    return sizeof(MSDFGEN_TrueDistanceCache) + (shapePtr->nLinearEdges+shapePtr->nQuadraticEdges+shapePtr->nCubicEdges)*sizeof(MSDFGEN_TrueDistanceEdgeCache);
}

void MSDFGEN_TrueDistanceCache_initialize(MSDFGEN_TrueDistanceCache *cachePtr, const MSDFGEN_CompiledShape *shapePtr) {
    MSDFGEN_TrueDistanceEdgeCache *cur, *end;
    cachePtr->origin.x = 0;
    cachePtr->origin.y = 0;
    cachePtr->minDistance = .0625f*FLT_MAX;
    for (cur = cachePtr->edgeData, end = cachePtr->edgeData+(shapePtr->nLinearEdges+shapePtr->nQuadraticEdges+shapePtr->nCubicEdges); cur < end; ++cur) {
        #if MSDFGEN_ABTEST_ALT_CACHE
            cur->origin.x = 0;
            cur->origin.y = 0;
        #else
            cur->edgeDistance = 0;
        #endif
    }
}

#ifdef NO_CACHE_AND_SQUARE_DISTANCE

// INCOMPLETE !!!!

MSDFGEN_real MSDFGEN_signedDistance(const MSDFGEN_CompiledShape *shapePtr, MSDFGEN_Vector2 origin, MSDFGEN_TrueDistanceCache *cachePtr) {
    int sign = -1;
    MSDFGEN_real sqd = FLT_MAX;
    size_t i;

    for (i = 0; i < shapePtr->nLinearEdges; ++i) {
        MSDFGEN_Vector2 originP0 = MSDFGEN_Vector2_difference(shapePtr->linearEdges[i].endpoint0, origin);
        MSDFGEN_real originP0SqD = MSDFGEN_Vector2_squaredLength(originP0);
        MSDFGEN_real t = MSDFGEN_Vector2_dot(originP0, shapePtr->linearEdges[i].invDerivative);
        if (originP0SqD < sqd) {
            sqd = originP0SqD;
            sign = crossNonZeroSign(shapePtr->linearEdges[i].cornerVec0, MSDFGEN_Vector2_sum(shapePtr->linearEdges[i].direction, originP0));
        }
        if (t > (MSDFGEN_real) 0 && t < (MSDFGEN_real) 1) {
            MSDFGEN_real pd = MSDFGEN_Vector2_cross(shapePtr->linearEdges[i].direction, originP0);
            MSDFGEN_real sqpd = pd*pd;
            if (sqpd < sqd) {
                sqd = sqpd;
                sign = nonZeroSign(pd);
            }
        }
    }

    for (i = 0; i < shapePtr->nQuadraticEdges; ++i) {
        MSDFGEN_Vector2 originP0 = MSDFGEN_Vector2_difference(shapePtr->quadraticEdges[i].endpoint0, origin);
        MSDFGEN_real originP0SqD = MSDFGEN_Vector2_squaredLength(originP0);
        MSDFGEN_real a = MSDFGEN_Vector2_squaredLength(shapePtr->quadraticEdges[i].derivative1);
        MSDFGEN_real b = (MSDFGEN_real) 3*MSDFGEN_Vector2_dot(shapePtr->quadraticEdges[i].derivative0, shapePtr->quadraticEdges[i].derivative1);
        MSDFGEN_real c = (MSDFGEN_real) 2*MSDFGEN_Vector2_squaredLength(shapePtr->quadraticEdges[i].derivative0) + MSDFGEN_Vector2_dot(originP0, shapePtr->quadraticEdges[i].derivative1);
        MSDFGEN_real d = MSDFGEN_Vector2_dot(originP0, shapePtr->quadraticEdges[i].derivative0);
        double t[3];
        int solutions = MSDFGEN_solveCubic(t, a, b, c, d);
        if (originP0SqD < sqd) {
            sqd = originP0SqD;
            sign = crossNonZeroSign(shapePtr->quadraticEdges[i].cornerVec0, MSDFGEN_Vector2_sum(shapePtr->quadraticEdges[i].direction0, originP0));
        }
        #define MSDFGEN_SD_RESOLVE_QUADRATIC_SOLUTION(t) if (t > 0 && t < 1) { \
            MSDFGEN_Vector2 originP = MSDFGEN_Vector2_sum(MSDFGEN_Vector2_sum(originP0, MSDFGEN_Vector2_scale((MSDFGEN_real) (2*t), shapePtr->quadraticEdges[i].derivative0)), MSDFGEN_Vector2_scale((MSDFGEN_real) (t*t), shapePtr->quadraticEdges[i].derivative1)); \
            MSDFGEN_real originPSqD = MSDFGEN_Vector2_squaredLength(originP); \
            if (originPSqD < sqd) { \
                MSDFGEN_Vector2 direction = MSDFGEN_Vector2_sum(shapePtr->quadraticEdges[i].derivative0, MSDFGEN_Vector2_scale(t, shapePtr->quadraticEdges[i].derivative1)); \
                sqd = originPSqD; \
                sign = crossNonZeroSign(direction, originP); \
            } \
        }
        if (solutions > 0) {
            MSDFGEN_SD_RESOLVE_QUADRATIC_SOLUTION(t[0]);
            if (solutions > 1) {
                MSDFGEN_SD_RESOLVE_QUADRATIC_SOLUTION(t[1]);
                if (solutions > 2)
                    MSDFGEN_SD_RESOLVE_QUADRATIC_SOLUTION(t[2]);
            }
        }
    }

    return (MSDFGEN_real) sign*sqrt(sqd);
}

#else

#if MSDFGEN_ABTEST_ALT_CACHE
#define MSDFGEN_IF_TRUE_DISTANCE_UNCACHED MSDFGEN_PERFSTATS_CACHE_TEST(); MSDFGEN_IF_CACHE_AND(++edgeCache, edgeCache[-1].edgeDistance-DISTANCE_DELTA_FACTOR*sqrt(MSDFGEN_Vector2_squaredLength(MSDFGEN_Vector2_difference(origin, edgeCache[-1].origin))) <= minDistance)
#else
#define MSDFGEN_IF_TRUE_DISTANCE_UNCACHED MSDFGEN_PERFSTATS_CACHE_TEST(); MSDFGEN_IF_CACHE_AND((edgeCache++->edgeDistance -= cacheDelta) <= minDistance)
#endif

MSDFGEN_real MSDFGEN_signedDistance(const MSDFGEN_CompiledShape *shapePtr, MSDFGEN_Vector2 origin, MSDFGEN_TrueDistanceCache *cachePtr) {

    MSDFGEN_real cacheDelta = DISTANCE_DELTA_FACTOR*sqrt(MSDFGEN_Vector2_squaredLength(MSDFGEN_Vector2_difference(origin, cachePtr->origin)));
    MSDFGEN_real minDistance = cachePtr->minDistance+cacheDelta;
    int distanceSign = -1;
    MSDFGEN_TrueDistanceEdgeCache *edgeCache = cachePtr->edgeData;
    size_t i;

    for (i = 0; i < shapePtr->nLinearEdges; ++i) {
        MSDFGEN_IF_TRUE_DISTANCE_UNCACHED {
            MSDFGEN_Vector2 originP0 = MSDFGEN_Vector2_difference(shapePtr->linearEdges[i].endpoint0, origin);
            MSDFGEN_real originP0Dist = sqrt(MSDFGEN_Vector2_squaredLength(originP0));
            MSDFGEN_real t = MSDFGEN_Vector2_dot(originP0, shapePtr->linearEdges[i].invDerivative);
            if (originP0Dist < minDistance) {
                minDistance = originP0Dist;
                distanceSign = crossNonZeroSign(shapePtr->linearEdges[i].cornerVec0, MSDFGEN_Vector2_sum(shapePtr->linearEdges[i].direction, originP0));
            }
            edgeCache[-1].edgeDistance = originP0Dist;
            if (t > (MSDFGEN_real) 0 && t < (MSDFGEN_real) 1) {
                MSDFGEN_real psd = MSDFGEN_Vector2_cross(shapePtr->linearEdges[i].direction, originP0);
                MSDFGEN_real pd = fabs(psd);
                if (pd < minDistance) {
                    minDistance = pd;
                    distanceSign = nonZeroSign(psd);
                }
                edgeCache[-1].edgeDistance = pd;
            } else {
                MSDFGEN_real originP1Dist = sqrt(MSDFGEN_Vector2_squaredLength(MSDFGEN_Vector2_difference(shapePtr->linearEdges[i].endpoint1, origin)));
                if (originP1Dist < edgeCache[-1].edgeDistance)
                    edgeCache[-1].edgeDistance = originP1Dist;
            }
            MSDFGEN_PERFSTATS_CACHE_MISS();
            #if MSDFGEN_ABTEST_ALT_CACHE
                edgeCache[-1].origin = origin;
            #endif
        }
    }

    for (i = 0; i < shapePtr->nQuadraticEdges; ++i) {
        MSDFGEN_IF_TRUE_DISTANCE_UNCACHED {
            MSDFGEN_Vector2 originP0 = MSDFGEN_Vector2_difference(shapePtr->quadraticEdges[i].endpoint0, origin);
            MSDFGEN_real originP0Dist = sqrt(MSDFGEN_Vector2_squaredLength(originP0));
            MSDFGEN_real originP1Dist = sqrt(MSDFGEN_Vector2_squaredLength(MSDFGEN_Vector2_difference(shapePtr->quadraticEdges[i].endpoint1, origin)));
            MSDFGEN_real a = MSDFGEN_Vector2_squaredLength(shapePtr->quadraticEdges[i].derivative1);
            MSDFGEN_real b = (MSDFGEN_real) 3*MSDFGEN_Vector2_dot(shapePtr->quadraticEdges[i].derivative0, shapePtr->quadraticEdges[i].derivative1);
            MSDFGEN_real c = (MSDFGEN_real) 2*MSDFGEN_Vector2_squaredLength(shapePtr->quadraticEdges[i].derivative0) + MSDFGEN_Vector2_dot(originP0, shapePtr->quadraticEdges[i].derivative1);
            MSDFGEN_real d = MSDFGEN_Vector2_dot(originP0, shapePtr->quadraticEdges[i].derivative0);
            double t[3];
            int solutions = MSDFGEN_solveCubic(t, a, b, c, d);
            if (originP0Dist < minDistance) {
                minDistance = originP0Dist;
                distanceSign = crossNonZeroSign(shapePtr->quadraticEdges[i].cornerVec0, MSDFGEN_Vector2_sum(shapePtr->quadraticEdges[i].direction0, originP0));
            }
            edgeCache[-1].edgeDistance = originP0Dist;
            if (originP1Dist < edgeCache[-1].edgeDistance)
                edgeCache[-1].edgeDistance = originP1Dist;
            #define MSDFGEN_SD_RESOLVE_QUADRATIC_SOLUTION(t) if (t > 0 && t < 1) { \
                MSDFGEN_Vector2 originP = MSDFGEN_Vector2_sum(MSDFGEN_Vector2_sum(originP0, MSDFGEN_Vector2_scale((MSDFGEN_real) (2*t), shapePtr->quadraticEdges[i].derivative0)), MSDFGEN_Vector2_scale((MSDFGEN_real) (t*t), shapePtr->quadraticEdges[i].derivative1)); \
                MSDFGEN_real originPDist = sqrt(MSDFGEN_Vector2_squaredLength(originP)); \
                if (originPDist < minDistance) { \
                    MSDFGEN_Vector2 direction = MSDFGEN_Vector2_sum(shapePtr->quadraticEdges[i].derivative0, MSDFGEN_Vector2_scale(t, shapePtr->quadraticEdges[i].derivative1)); \
                    minDistance = originPDist; \
                    distanceSign = crossNonZeroSign(direction, originP); \
                } \
                if (originPDist < edgeCache[-1].edgeDistance) \
                    edgeCache[-1].edgeDistance = originPDist; \
            }
            if (solutions > 0) {
                MSDFGEN_SD_RESOLVE_QUADRATIC_SOLUTION(t[0]);
                if (solutions > 1) {
                    MSDFGEN_SD_RESOLVE_QUADRATIC_SOLUTION(t[1]);
                    if (solutions > 2)
                        MSDFGEN_SD_RESOLVE_QUADRATIC_SOLUTION(t[2]);
                }
            }
            MSDFGEN_PERFSTATS_CACHE_MISS();
            #if MSDFGEN_ABTEST_ALT_CACHE
                edgeCache[-1].origin = origin;
            #endif
        }
    }

    for (i = 0; i < shapePtr->nCubicEdges; ++i) {
        MSDFGEN_IF_TRUE_DISTANCE_UNCACHED {
            int start, step;
            MSDFGEN_Vector2 originP0 = MSDFGEN_Vector2_difference(shapePtr->cubicEdges[i].endpoint0, origin);
            MSDFGEN_real originP0Dist = sqrt(MSDFGEN_Vector2_squaredLength(originP0));
            MSDFGEN_real originP1Dist = sqrt(MSDFGEN_Vector2_squaredLength(MSDFGEN_Vector2_difference(shapePtr->cubicEdges[i].endpoint1, origin)));
            if (originP0Dist < minDistance) {
                minDistance = originP0Dist;
                distanceSign = crossNonZeroSign(shapePtr->cubicEdges[i].cornerVec0, MSDFGEN_Vector2_sum(shapePtr->cubicEdges[i].direction0, originP0));
            }
            edgeCache[-1].edgeDistance = originP0Dist;
            if (originP1Dist < edgeCache[-1].edgeDistance)
                edgeCache[-1].edgeDistance = originP1Dist;
            for (start = 0; start <= MSDFGEN_CUBIC_SEARCH_STARTS; ++start) {
                int remainingSteps = MSDFGEN_CUBIC_SEARCH_STEPS;
                MSDFGEN_real t, improvedT = (MSDFGEN_real) 1/(MSDFGEN_real) MSDFGEN_CUBIC_SEARCH_STARTS*(MSDFGEN_real) start;
                MSDFGEN_Vector2 originP, derivative0, derivative1;
                do {
                    t = improvedT;
                    originP = MSDFGEN_Vector2_sum(
                        MSDFGEN_Vector2_sum(
                            MSDFGEN_Vector2_sum(originP0, MSDFGEN_Vector2_scale((MSDFGEN_real) 3*t, shapePtr->cubicEdges[i].derivative0)),
                            MSDFGEN_Vector2_scale((MSDFGEN_real) 3*(t*t), shapePtr->cubicEdges[i].derivative1)
                        ), MSDFGEN_Vector2_scale(t*t*t, shapePtr->cubicEdges[i].derivative2)
                    );
                    derivative0 = MSDFGEN_Vector2_sum(
                        MSDFGEN_Vector2_sum(
                            MSDFGEN_Vector2_scale((MSDFGEN_real) 3, shapePtr->cubicEdges[i].derivative0),
                            MSDFGEN_Vector2_scale((MSDFGEN_real) 6*t, shapePtr->cubicEdges[i].derivative1)
                        ), MSDFGEN_Vector2_scale((MSDFGEN_real) 3*(t*t), shapePtr->cubicEdges[i].derivative2)
                    );
                    if (!remainingSteps--)
                        break;
                    derivative1 = MSDFGEN_Vector2_sum(
                        MSDFGEN_Vector2_scale((MSDFGEN_real) 6, shapePtr->cubicEdges[i].derivative1),
                        MSDFGEN_Vector2_scale((MSDFGEN_real) 6*t, shapePtr->cubicEdges[i].derivative2)
                    );
                    improvedT = t-MSDFGEN_Vector2_dot(originP, derivative0)/(MSDFGEN_Vector2_squaredLength(derivative0)+MSDFGEN_Vector2_dot(originP, derivative1));
                } while (improvedT > (MSDFGEN_real) 0 && improvedT < (MSDFGEN_real) 1);
                if (t > (MSDFGEN_real) 0 && t < (MSDFGEN_real) 1) {
                    MSDFGEN_real originPDist = sqrt(MSDFGEN_Vector2_squaredLength(originP));
                    if (originPDist < minDistance) {
                        minDistance = originPDist;
                        distanceSign = crossNonZeroSign(derivative0, originP);
                    }
                    if (originPDist < edgeCache[-1].edgeDistance)
                        edgeCache[-1].edgeDistance = originPDist;
                }
            }
            MSDFGEN_PERFSTATS_CACHE_MISS();
            #if MSDFGEN_ABTEST_ALT_CACHE
                edgeCache[-1].origin = origin;
            #endif
        }
    }

    cachePtr->origin = origin;
    cachePtr->minDistance = minDistance;
    return (MSDFGEN_real) distanceSign*minDistance;
}

#endif

#ifdef __cplusplus
}
#endif
