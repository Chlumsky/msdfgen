
#pragma once

#include <stddef.h>
#include "Vector2.h"
#include "CompiledShape.h"

#define MSDFGEN_CUBIC_SEARCH_STARTS 4
#define MSDFGEN_CUBIC_SEARCH_STEPS 4

#define MSDFGEN_ABTEST_ALT_CACHE 0

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
#if MSDFGEN_ABTEST_ALT_CACHE
    MSDFGEN_Vector2 origin;
#endif
    MSDFGEN_real edgeDistance;
} MSDFGEN_TrueDistanceEdgeCache;

typedef struct {
    MSDFGEN_Vector2 origin;
    MSDFGEN_real minDistance;
    MSDFGEN_TrueDistanceEdgeCache edgeData[1];
} MSDFGEN_TrueDistanceCache;

typedef struct {
    MSDFGEN_Vector2 origin;
    struct EdgeData {
        MSDFGEN_real absDistance;
        MSDFGEN_real domainDistance0, domainDistance1;
        MSDFGEN_real perpendicularDistance0, perpendicularDistance1;
    } edgeData[1];
} MSDFGEN_PerpendicularDistanceCache;

typedef struct {
    struct {
        MSDFGEN_real scale, translate;
    } distanceMapping;
    struct {
        MSDFGEN_Vector2 scale, translate;
    } projection;
} MSDFGEN_SDFTransformation;

typedef struct {
    enum Mode {
        DISABLED,
        INDISCRIMINATE,
        EDGE_PRIORITY,
        EDGE_ONLY
    } mode;
    enum DistanceCheckMode {
        DO_NOT_CHECK_DISTANCE,
        CHECK_DISTANCE_AT_EDGE,
        ALWAYS_CHECK_DISTANCE
    } distanceCheckMode;
    MSDFGEN_real minDeviationRatio;
    MSDFGEN_real minImproveRatio;
    void *buffer;
} MSDFGEN_ErrorCorrectionSettings;

size_t MSDFGEN_TrueDistanceCache_size(const MSDFGEN_CompiledShape *shapePtr);
size_t MSDFGEN_PerpendicularDistanceCache_size(const MSDFGEN_CompiledShape *shapePtr);

void MSDFGEN_TrueDistanceCache_initialize(MSDFGEN_TrueDistanceCache *cachePtr, const MSDFGEN_CompiledShape *shapePtr);
void MSDFGEN_PerpendicularDistanceCache_initialize(MSDFGEN_PerpendicularDistanceCache *cachePtr, const MSDFGEN_CompiledShape *shapePtr);

MSDFGEN_real MSDFGEN_signedDistance(const MSDFGEN_CompiledShape *shapePtr, MSDFGEN_Vector2 origin, MSDFGEN_TrueDistanceCache *cachePtr);
MSDFGEN_real MSDFGEN_perpendicularSignedDistance(const MSDFGEN_CompiledShape *shapePtr, MSDFGEN_Vector2 origin, MSDFGEN_PerpendicularDistanceCache *cachePtr);
MSDFGEN_real MSDFGEN_multiSignedDistance(MSDFGEN_real dstMultiDistance[3], const MSDFGEN_CompiledShape *shapePtr, MSDFGEN_Vector2 origin, MSDFGEN_PerpendicularDistanceCache *cachePtr);

void MSDFGEN_generateSDF(float *dstPixels, int width, int height, size_t rowStride, const MSDFGEN_CompiledShape *shapePtr, MSDFGEN_SDFTransformation transformation, MSDFGEN_TrueDistanceCache *cachePtr);
void MSDFGEN_generatePSDF(float *dstPixels, int width, int height, size_t rowStride, const MSDFGEN_CompiledShape *shapePtr, MSDFGEN_SDFTransformation transformation, MSDFGEN_PerpendicularDistanceCache *cachePtr);
void MSDFGEN_generateMSDF(float *dstPixels, int width, int height, size_t rowStride, const MSDFGEN_CompiledShape *shapePtr, MSDFGEN_SDFTransformation transformation, MSDFGEN_PerpendicularDistanceCache *cachePtr);
void MSDFGEN_generateMTSDF(float *dstPixels, int width, int height, size_t rowStride, const MSDFGEN_CompiledShape *shapePtr, MSDFGEN_SDFTransformation transformation, MSDFGEN_PerpendicularDistanceCache *cachePtr);

void MSDFGEN_errorCorrectionMSDF(MSDFGEN_ErrorCorrectionSettings settings, float *pixels, int width, int height, size_t rowStride, const MSDFGEN_CompiledShape *shapePtr, MSDFGEN_SDFTransformation transformation, MSDFGEN_PerpendicularDistanceCache *cachePtr);
void MSDFGEN_errorCorrectionMTSDF(MSDFGEN_ErrorCorrectionSettings settings, float *pixels, int width, int height, size_t rowStride, const MSDFGEN_CompiledShape *shapePtr, MSDFGEN_SDFTransformation transformation, MSDFGEN_PerpendicularDistanceCache *cachePtr);

#ifdef __cplusplus
}
#endif
