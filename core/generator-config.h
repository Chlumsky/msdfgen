
#pragma once

#include <cstdlib>

#define MSDFGEN_DEFAULT_ERROR_CORRECTION_THRESHOLD 1.001

namespace msdfgen {

/// The configuration of the distance field generator algorithm.
struct GeneratorConfig {
    /// Specifies whether to use the version of the algorithm that supports overlapping contours with the same winding. May be set to false to improve performance when no such contours are present.
    bool overlapSupport;

    inline explicit GeneratorConfig(bool overlapSupport = true) : overlapSupport(overlapSupport) { }
};

/// The configuration of the MSDF error correction pass.
struct ErrorCorrectionConfig {
    /// Mode of operation.
    enum Mode {
        /// Skips error correction pass.
        DISABLED,
        /// Corrects all discontinuities of the distance field regardless if edges are adversely affected.
        INDISCRIMINATE,
        /// Corrects artifacts at edges and other discontinuous distances only if it does not affect edges or corners.
        EDGE_PRIORITY,
        /// Only corrects artifacts at edges.
        EDGE_ONLY
    } mode;
    /// The minimum ratio between the actual and maximum expected distance delta to be considered an error.
    double threshold;
    /// An optional buffer to avoid dynamic allocation. Must have at least as many bytes as the MSDF has pixels.
    byte *buffer;

    inline explicit ErrorCorrectionConfig(Mode mode = EDGE_PRIORITY, double threshold = MSDFGEN_DEFAULT_ERROR_CORRECTION_THRESHOLD, byte *buffer = NULL) : mode(mode), threshold(threshold), buffer(buffer) { }
};

}
