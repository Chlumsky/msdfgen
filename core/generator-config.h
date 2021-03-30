
#pragma once

#define MSDFGEN_DEFAULT_ARTIFACT_PATCHER_MIN_IMPROVE_RATIO 1.001

namespace msdfgen {

/// The configuration of the distance field generator algorithm.
struct GeneratorConfig {
    /// Specifies whether to use the version of the algorithm that supports overlapping contours with the same winding. May be set to false to improve performance when no such contours are present.
    bool overlapSupport;

    inline explicit GeneratorConfig(bool overlapSupport = true) : overlapSupport(overlapSupport) { }
};

/// The configuration of the artifact patcher that processes the generated MSDF and fixes potential interpolation errors.
struct ArtifactPatcherConfig {
    /// The mode of operation.
    enum Mode {
        DISABLED,
        INDISCRIMINATE,
        EDGE_PRIORITY,
        EDGE_ONLY
    } mode;
    /// The minimum ratio of improvement required to patch a pixel. Must be greater than or equal to 1.
    double minImproveRatio;

    inline explicit ArtifactPatcherConfig(Mode mode = EDGE_PRIORITY, double minImproveRatio = MSDFGEN_DEFAULT_ARTIFACT_PATCHER_MIN_IMPROVE_RATIO) : mode(mode), minImproveRatio(minImproveRatio) { }
};

}
