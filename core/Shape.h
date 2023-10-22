
#pragma once

#include <vector>
#include "types.h"
#include "Contour.h"
#include "Scanline.h"

namespace msdfgen {

// Threshold of the dot product of adjacent edge directions to be considered convergent.
#define MSDFGEN_CORNER_DOT_EPSILON ::msdfgen::real(.000001)
// The proportional amount by which a curve's control point will be adjusted to eliminate convergent corners.
#define MSDFGEN_DECONVERGENCE_FACTOR ::msdfgen::real(.000001)

/// Vector shape representation.
class Shape {

public:
    struct Bounds {
        real l, b, r, t;
    };

    /// The list of contours the shape consists of.
    std::vector<Contour> contours;
    /// Specifies whether the shape uses bottom-to-top (false) or top-to-bottom (true) Y coordinates.
    bool inverseYAxis;

    Shape();
    /// Adds a contour.
    void addContour(const Contour &contour);
#ifdef MSDFGEN_USE_CPP11
    void addContour(Contour &&contour);
#endif
    /// Adds a blank contour and returns its reference.
    Contour &addContour();
    /// Normalizes the shape geometry for distance field generation.
    void normalize();
    /// Performs basic checks to determine if the object represents a valid shape.
    bool validate() const;
    /// Adjusts the bounding box to fit the shape.
    void bound(real &l, real &b, real &r, real &t) const;
    /// Adjusts the bounding box to fit the shape border's mitered corners.
    void boundMiters(real &l, real &b, real &r, real &t, real border, real miterLimit, int polarity) const;
    /// Computes the minimum bounding box that fits the shape, optionally with a (mitered) border.
    Bounds getBounds(real border = 0, real miterLimit = 0, int polarity = 0) const;
    /// Outputs the scanline that intersects the shape at y.
    void scanline(Scanline &line, real y) const;
    /// Returns the total number of edge segments
    int edgeCount() const;
    /// Assumes its contours are unoriented (even-odd fill rule). Attempts to orient them to conform to the non-zero winding rule.
    void orientContours();

};

}
