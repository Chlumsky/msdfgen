
#pragma once

#include "Projection.h"
#include "Shape.h"
#include "BitmapRef.hpp"

namespace msdfgen {

/// Performs error correction on a computed MSDF to eliminate interpolation artifacts. This is a low-level class, you may want to use the API in msdf-error-correction.h instead.
class MSDFErrorCorrection {

public:
    /// Stencil flags.
    enum Flags {
        /// Texel marked as potentially causing interpolation errors.
        ERROR = 1,
        /// Texel marked as protected. Protected texels are only given the error flag if they cause inversion artifacts.
        PROTECTED = 2
    };

    MSDFErrorCorrection();
    explicit MSDFErrorCorrection(const BitmapRef<byte, 1> &stencil);
    /// Flags all texels that are interpolated at corners as protected.
    void protectCorners(const Shape &shape, const Projection &projection);
    /// Flags all texels that contribute to edges as protected.
    template <int N>
    void protectEdges(const BitmapConstRef<float, N> &sdf, const Projection &projection, double range);
    /// Flags all texels as protected.
    void protectAll();
    /// Flags texels that are expected to cause interpolation artifacts.
    template <int N>
    void findErrors(const BitmapConstRef<float, N> &sdf, const Projection &projection, double range, double threshold);
    /// Modifies the MSDF so that all texels with the error flag are converted to single-channel.
    template <int N>
    void apply(const BitmapRef<float, N> &sdf) const;
    /// Returns the stencil in its current state (see Flags).
    BitmapConstRef<byte, 1> getStencil() const;

private:
    BitmapRef<byte, 1> stencil;

};

}
