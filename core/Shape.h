
#pragma once

#include <vector>
#include "Contour.h"
#include "Scanline.h"

namespace msdfgen {

/// Vector shape representation.
class Shape {

public:
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
    Contour & addContour();
    /// Normalizes the shape geometry for distance field generation.
    void normalize();
    /// Performs basic checks to determine if the object represents a valid shape.
    bool validate() const;
    /// Adjusts the bounding box to fit the shape.
    void bounds(double &l, double &b, double &r, double &t) const;
    /// Adjusts the bounding box to fit the shape border's mitered corners.
    void miterBounds(double &l, double &b, double &r, double &t, double border, double miterLimit) const;
    /// Outputs the scanline that intersects the shape at y.
    void scanline(Scanline &line, double y) const;

};

}
