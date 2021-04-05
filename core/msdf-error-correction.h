
#pragma once

#include "Vector2.h"
#include "Projection.h"
#include "Shape.h"
#include "BitmapRef.hpp"
#include "generator-config.h"

namespace msdfgen {

/// Predicts potential artifacts caused by the interpolation of the MSDF and corrects them by converting nearby texels to single-channel.
void msdfErrorCorrection(const BitmapRef<float, 3> &sdf, const Shape &shape, const Projection &projection, double range, const ErrorCorrectionConfig &config = ErrorCorrectionConfig());
void msdfErrorCorrection(const BitmapRef<float, 4> &sdf, const Shape &shape, const Projection &projection, double range, const ErrorCorrectionConfig &config = ErrorCorrectionConfig());

/// Applies the error correction to all discontiunous distances (INDISCRIMINATE mode). Does not need shape or translation.
void msdfDistanceErrorCorrection(const BitmapRef<float, 3> &sdf, const Projection &projection, double range, double threshold = MSDFGEN_DEFAULT_ERROR_CORRECTION_THRESHOLD);
void msdfDistanceErrorCorrection(const BitmapRef<float, 4> &sdf, const Projection &projection, double range, double threshold = MSDFGEN_DEFAULT_ERROR_CORRECTION_THRESHOLD);

/// Applies the error correction to edges only (EDGE_ONLY mode). Does not need shape or translation.
void msdfEdgeErrorCorrection(const BitmapRef<float, 3> &sdf, const Projection &projection, double range, double threshold = MSDFGEN_DEFAULT_ERROR_CORRECTION_THRESHOLD);
void msdfEdgeErrorCorrection(const BitmapRef<float, 4> &sdf, const Projection &projection, double range, double threshold = MSDFGEN_DEFAULT_ERROR_CORRECTION_THRESHOLD);

/// The original version of the error correction algorithm.
void msdfErrorCorrection_legacy(const BitmapRef<float, 3> &output, const Vector2 &threshold);
void msdfErrorCorrection_legacy(const BitmapRef<float, 4> &output, const Vector2 &threshold);

}
