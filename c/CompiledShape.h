
#pragma once

#include <stddef.h>
#include "Vector2.h"

#ifdef __cplusplus
extern "C" {
#endif

/*typedef struct {
    void *userPtr;
    void *(*reallocPtr)(void *userPtr, void *memoryPtr, size_t memorySize);
    void *(*freePtr)(void *userPtr, void *memoryPtr);
} MSDFGEN_Allocator;*/

typedef struct {
    int colorMask;
    MSDFGEN_Vector2 endpoint0, endpoint1;
    MSDFGEN_Vector2 cornerVec0, cornerVec1;
    MSDFGEN_Vector2 direction;
    MSDFGEN_Vector2 invDerivative;
} MSDFGEN_CompiledLinearEdge;

typedef struct {
    int colorMask;
    MSDFGEN_Vector2 endpoint0, endpoint1;
    MSDFGEN_Vector2 cornerVec0, cornerVec1;
    MSDFGEN_Vector2 direction0, direction1;
    MSDFGEN_Vector2 derivative0, derivative1;
} MSDFGEN_CompiledQuadraticEdge;

typedef struct {
    int colorMask;
    MSDFGEN_Vector2 endpoint0, endpoint1;
    MSDFGEN_Vector2 cornerVec0, cornerVec1;
    MSDFGEN_Vector2 direction0, direction1;
    MSDFGEN_Vector2 derivative0, derivative1, derivative2;
} MSDFGEN_CompiledCubicEdge;

typedef struct {
    MSDFGEN_CompiledLinearEdge *linearEdges;
    MSDFGEN_CompiledQuadraticEdge *quadraticEdges;
    MSDFGEN_CompiledCubicEdge *cubicEdges;
    size_t nLinearEdges, nQuadraticEdges, nCubicEdges;
} MSDFGEN_CompiledShape;

/*void MSDFGEN_Shape_initialize(MSDFGEN_Shape *shapePtr);
void MSDFGEN_Shape_clear(MSDFGEN_Shape *shapePtr, MSDFGEN_Allocator allocator);*/

void MSDFGEN_compileLinearEdge(MSDFGEN_CompiledLinearEdge *dstEdgePtr, int colorMask, MSDFGEN_Vector2 point0, MSDFGEN_Vector2 point1, MSDFGEN_Vector2 prevEdgeDirection1, MSDFGEN_Vector2 nextEdgeDirection0);
void MSDFGEN_compileQuadraticEdge(MSDFGEN_CompiledQuadraticEdge *dstEdgePtr, int colorMask, MSDFGEN_Vector2 point0, MSDFGEN_Vector2 point1, MSDFGEN_Vector2 point2, MSDFGEN_Vector2 prevEdgeDirection1, MSDFGEN_Vector2 nextEdgeDirection0);
void MSDFGEN_compileCubicEdge(MSDFGEN_CompiledCubicEdge *dstEdgePtr, int colorMask, MSDFGEN_Vector2 point0, MSDFGEN_Vector2 point1, MSDFGEN_Vector2 point2, MSDFGEN_Vector2 point3, MSDFGEN_Vector2 prevEdgeDirection1, MSDFGEN_Vector2 nextEdgeDirection0);

/*void MSDFGEN_Shape_addLinearEdge(MSDFGEN_Shape *shapePtr, const MSDFGEN_Shape::LinearEdge *edgePtr, MSDFGEN_Allocator allocator);
void MSDFGEN_Shape_addQuadraticEdge(MSDFGEN_Shape *shapePtr, const MSDFGEN_Shape::QuadraticEdge *edgePtr, MSDFGEN_Allocator allocator);
void MSDFGEN_Shape_addCubicEdge(MSDFGEN_Shape *shapePtr, const MSDFGEN_Shape::CubicEdge *edgePtr, MSDFGEN_Allocator allocator);*/

#ifdef __cplusplus
}
#endif
