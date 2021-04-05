
#include "msdf-error-correction.h"

#include <vector>
#include "arithmetics.hpp"
#include "Bitmap.h"
#include "MSDFErrorCorrection.h"

namespace msdfgen {

template <int N>
static void msdfErrorCorrectionInner(const BitmapRef<float, N> &sdf, const Shape &shape, const Projection &projection, double range, const ErrorCorrectionConfig &config) {
    if (config.mode == ErrorCorrectionConfig::DISABLED)
        return;
    Bitmap<byte, 1> stencilBuffer;
    if (!config.buffer)
        stencilBuffer = Bitmap<byte, 1>(sdf.width, sdf.height);
    BitmapRef<byte, 1> stencil;
    stencil.pixels = config.buffer ? config.buffer : (byte *) stencilBuffer;
    stencil.width = sdf.width, stencil.height = sdf.height;
    MSDFErrorCorrection ec(stencil);
    switch (config.mode) {
        case ErrorCorrectionConfig::DISABLED:
        case ErrorCorrectionConfig::INDISCRIMINATE:
            break;
        case ErrorCorrectionConfig::EDGE_PRIORITY:
            ec.protectCorners(shape, projection);
            ec.protectEdges<N>(sdf, projection, range);
            break;
        case ErrorCorrectionConfig::EDGE_ONLY:
            ec.protectAll();
            break;
    }
    ec.findErrors<N>(sdf, projection, range, config.threshold);
    ec.apply(sdf);
}

template <int N>
static void msdfErrorCorrectionShapeless(const BitmapRef<float, N> &sdf, const Projection &projection, double range, double threshold, bool protectAll) {
    Bitmap<byte, 1> stencilBuffer(sdf.width, sdf.height);
    MSDFErrorCorrection ec(stencilBuffer);
    if (protectAll)
        ec.protectAll();
    ec.findErrors<N>(sdf, projection, range, threshold);
    ec.apply(sdf);
}

void msdfErrorCorrection(const BitmapRef<float, 3> &sdf, const Shape &shape, const Projection &projection, double range, const ErrorCorrectionConfig &config) {
    msdfErrorCorrectionInner(sdf, shape, projection, range, config);
}
void msdfErrorCorrection(const BitmapRef<float, 4> &sdf, const Shape &shape, const Projection &projection, double range, const ErrorCorrectionConfig &config) {
    msdfErrorCorrectionInner(sdf, shape, projection, range, config);
}

void msdfDistanceErrorCorrection(const BitmapRef<float, 3> &sdf, const Projection &projection, double range, double threshold) {
    msdfErrorCorrectionShapeless(sdf, projection, range, threshold, false);
}
void msdfDistanceErrorCorrection(const BitmapRef<float, 4> &sdf, const Projection &projection, double range, double threshold) {
    msdfErrorCorrectionShapeless(sdf, projection, range, threshold, false);
}

void msdfEdgeErrorCorrection(const BitmapRef<float, 3> &sdf, const Projection &projection, double range, double threshold) {
    msdfErrorCorrectionShapeless(sdf, projection, range, threshold, true);
}
void msdfEdgeErrorCorrection(const BitmapRef<float, 4> &sdf, const Projection &projection, double range, double threshold) {
    msdfErrorCorrectionShapeless(sdf, projection, range, threshold, true);
}


// Legacy version

inline static bool detectClash(const float *a, const float *b, double threshold) {
    // Sort channels so that pairs (a0, b0), (a1, b1), (a2, b2) go from biggest to smallest absolute difference
    float a0 = a[0], a1 = a[1], a2 = a[2];
    float b0 = b[0], b1 = b[1], b2 = b[2];
    float tmp;
    if (fabsf(b0-a0) < fabsf(b1-a1)) {
        tmp = a0, a0 = a1, a1 = tmp;
        tmp = b0, b0 = b1, b1 = tmp;
    }
    if (fabsf(b1-a1) < fabsf(b2-a2)) {
        tmp = a1, a1 = a2, a2 = tmp;
        tmp = b1, b1 = b2, b2 = tmp;
        if (fabsf(b0-a0) < fabsf(b1-a1)) {
            tmp = a0, a0 = a1, a1 = tmp;
            tmp = b0, b0 = b1, b1 = tmp;
        }
    }
    return (fabsf(b1-a1) >= threshold) &&
        !(b0 == b1 && b0 == b2) && // Ignore if other pixel has been equalized
        fabsf(a2-.5f) >= fabsf(b2-.5f); // Out of the pair, only flag the pixel farther from a shape edge
}

template <int N>
static void msdfErrorCorrectionInner_legacy(const BitmapRef<float, N> &output, const Vector2 &threshold) {
    std::vector<std::pair<int, int> > clashes;
    int w = output.width, h = output.height;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            if (
                (x > 0 && detectClash(output(x, y), output(x-1, y), threshold.x)) ||
                (x < w-1 && detectClash(output(x, y), output(x+1, y), threshold.x)) ||
                (y > 0 && detectClash(output(x, y), output(x, y-1), threshold.y)) ||
                (y < h-1 && detectClash(output(x, y), output(x, y+1), threshold.y))
            )
                clashes.push_back(std::make_pair(x, y));
        }
    for (std::vector<std::pair<int, int> >::const_iterator clash = clashes.begin(); clash != clashes.end(); ++clash) {
        float *pixel = output(clash->first, clash->second);
        float med = median(pixel[0], pixel[1], pixel[2]);
        pixel[0] = med, pixel[1] = med, pixel[2] = med;
    }
#ifndef MSDFGEN_NO_DIAGONAL_CLASH_DETECTION
    clashes.clear();
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            if (
                (x > 0 && y > 0 && detectClash(output(x, y), output(x-1, y-1), threshold.x+threshold.y)) ||
                (x < w-1 && y > 0 && detectClash(output(x, y), output(x+1, y-1), threshold.x+threshold.y)) ||
                (x > 0 && y < h-1 && detectClash(output(x, y), output(x-1, y+1), threshold.x+threshold.y)) ||
                (x < w-1 && y < h-1 && detectClash(output(x, y), output(x+1, y+1), threshold.x+threshold.y))
            )
                clashes.push_back(std::make_pair(x, y));
        }
    for (std::vector<std::pair<int, int> >::const_iterator clash = clashes.begin(); clash != clashes.end(); ++clash) {
        float *pixel = output(clash->first, clash->second);
        float med = median(pixel[0], pixel[1], pixel[2]);
        pixel[0] = med, pixel[1] = med, pixel[2] = med;
    }
#endif
}

void msdfErrorCorrection_legacy(const BitmapRef<float, 3> &output, const Vector2 &threshold) {
    msdfErrorCorrectionInner_legacy(output, threshold);
}
void msdfErrorCorrection_legacy(const BitmapRef<float, 4> &output, const Vector2 &threshold) {
    msdfErrorCorrectionInner_legacy(output, threshold);
}

}
