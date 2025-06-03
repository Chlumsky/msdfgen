
#include "CompiledShape.h"

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

static MSDFGEN_Vector2 computeCornerVec(MSDFGEN_Vector2 normalizedDirection, MSDFGEN_Vector2 adjacentEdgeDirection1) {
    return MSDFGEN_Vector2_normalizeOrZero(MSDFGEN_Vector2_sum(normalizedDirection, MSDFGEN_Vector2_normalizeOrZero(adjacentEdgeDirection1)));
}

void MSDFGEN_compileLinearEdge(MSDFGEN_CompiledLinearEdge *dstEdgePtr, int colorMask, MSDFGEN_Vector2 point0, MSDFGEN_Vector2 point1, MSDFGEN_Vector2 prevEdgeDirection1, MSDFGEN_Vector2 nextEdgeDirection0) {
    MSDFGEN_Vector2 p0p1 = MSDFGEN_Vector2_difference(point1, point0);
    MSDFGEN_real p0p1SqLen = MSDFGEN_Vector2_squaredLength(p0p1);
    dstEdgePtr->colorMask = colorMask;
    dstEdgePtr->endpoint0 = point0;
    dstEdgePtr->endpoint1 = point1;
    dstEdgePtr->direction = MSDFGEN_Vector2_normalizeOrZero(p0p1);
    dstEdgePtr->cornerVec0 = computeCornerVec(dstEdgePtr->direction, prevEdgeDirection1);
    dstEdgePtr->cornerVec1 = computeCornerVec(dstEdgePtr->direction, nextEdgeDirection0);
    dstEdgePtr->invDerivative.x = 0;
    dstEdgePtr->invDerivative.y = 0;
    if (p0p1SqLen != (MSDFGEN_real) 0) {
        dstEdgePtr->invDerivative.x = -p0p1.x/p0p1SqLen;
        dstEdgePtr->invDerivative.y = -p0p1.y/p0p1SqLen;
    }
}

void MSDFGEN_compileQuadraticEdge(MSDFGEN_CompiledQuadraticEdge *dstEdgePtr, int colorMask, MSDFGEN_Vector2 point0, MSDFGEN_Vector2 point1, MSDFGEN_Vector2 point2, MSDFGEN_Vector2 prevEdgeDirection1, MSDFGEN_Vector2 nextEdgeDirection0) {
    MSDFGEN_Vector2 p1p2 = MSDFGEN_Vector2_difference(point2, point1);
    dstEdgePtr->colorMask = colorMask;
    dstEdgePtr->endpoint0 = point0;
    dstEdgePtr->endpoint1 = point2;
    dstEdgePtr->derivative0 = MSDFGEN_Vector2_difference(point1, point0);
    dstEdgePtr->derivative1 = MSDFGEN_Vector2_difference(p1p2, dstEdgePtr->derivative0);
    dstEdgePtr->direction0 = dstEdgePtr->derivative0;
    dstEdgePtr->direction1 = p1p2;
    if (dstEdgePtr->direction0.x == (MSDFGEN_real) 0 && dstEdgePtr->direction0.y == (MSDFGEN_real) 0)
        dstEdgePtr->direction0 = MSDFGEN_Vector2_difference(point2, point0);
    if (dstEdgePtr->direction1.x == (MSDFGEN_real) 0 && dstEdgePtr->direction1.y == (MSDFGEN_real) 0)
        dstEdgePtr->direction1 = MSDFGEN_Vector2_difference(point2, point0);
    dstEdgePtr->direction0 = MSDFGEN_Vector2_normalizeOrZero(dstEdgePtr->direction0);
    dstEdgePtr->direction1 = MSDFGEN_Vector2_normalizeOrZero(dstEdgePtr->direction1);
    dstEdgePtr->cornerVec0 = computeCornerVec(dstEdgePtr->direction0, prevEdgeDirection1);
    dstEdgePtr->cornerVec1 = computeCornerVec(dstEdgePtr->direction1, nextEdgeDirection0);
}

void MSDFGEN_compileCubicEdge(MSDFGEN_CompiledCubicEdge *dstEdgePtr, int colorMask, MSDFGEN_Vector2 point0, MSDFGEN_Vector2 point1, MSDFGEN_Vector2 point2, MSDFGEN_Vector2 point3, MSDFGEN_Vector2 prevEdgeDirection1, MSDFGEN_Vector2 nextEdgeDirection0) {
    MSDFGEN_Vector2 p1p2 = MSDFGEN_Vector2_difference(point2, point1);
    MSDFGEN_Vector2 p2p3 = MSDFGEN_Vector2_difference(point3, point2);
    dstEdgePtr->colorMask = colorMask;
    dstEdgePtr->endpoint0 = point0;
    dstEdgePtr->endpoint1 = point3;
    dstEdgePtr->derivative0 = MSDFGEN_Vector2_difference(point1, point0);
    dstEdgePtr->derivative1 = MSDFGEN_Vector2_difference(p1p2, dstEdgePtr->derivative0);
    dstEdgePtr->derivative2 = MSDFGEN_Vector2_difference(MSDFGEN_Vector2_difference(p2p3, p1p2), dstEdgePtr->derivative1);
    dstEdgePtr->direction0 = dstEdgePtr->derivative0;
    if (dstEdgePtr->direction0.x == (MSDFGEN_real) 0 && dstEdgePtr->direction0.y == (MSDFGEN_real) 0) {
        dstEdgePtr->direction0 = MSDFGEN_Vector2_difference(point2, point0);
        if (dstEdgePtr->direction0.x == (MSDFGEN_real) 0 && dstEdgePtr->direction0.y == (MSDFGEN_real) 0)
            dstEdgePtr->direction0 = MSDFGEN_Vector2_difference(point3, point0);
    }
    dstEdgePtr->direction1 = p2p3;
    if (dstEdgePtr->direction1.x == (MSDFGEN_real) 0 && dstEdgePtr->direction1.y == (MSDFGEN_real) 0) {
        dstEdgePtr->direction1 = MSDFGEN_Vector2_difference(point3, point1);
        if (dstEdgePtr->direction1.x == (MSDFGEN_real) 0 && dstEdgePtr->direction1.y == (MSDFGEN_real) 0)
            dstEdgePtr->direction1 = MSDFGEN_Vector2_difference(point3, point0);
    }
    dstEdgePtr->direction0 = MSDFGEN_Vector2_normalizeOrZero(dstEdgePtr->direction0);
    dstEdgePtr->direction1 = MSDFGEN_Vector2_normalizeOrZero(dstEdgePtr->direction1);
    dstEdgePtr->cornerVec0 = computeCornerVec(dstEdgePtr->direction0, prevEdgeDirection1);
    dstEdgePtr->cornerVec1 = computeCornerVec(dstEdgePtr->direction1, nextEdgeDirection0);
}

#ifdef __cplusplus
}
#endif
