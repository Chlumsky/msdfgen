
#pragma once

#include <vector>

namespace msdfgen {

/// Represents a horizontal scanline intersecting a shape.
class Scanline {

public:
    /// An intersection with the scanline.
    struct Intersection {
        /// X coordinate
        double x;
        /// Normalized Y direction of the oriented edge at the point of intersection
        int direction;
    };

    Scanline();
    /// Populates the intersection list.
    void setIntersections(const std::vector<Intersection> &intersections);
#ifdef MSDFGEN_USE_CPP11
    void setIntersections(std::vector<Intersection> &&intersections);
#endif
    /// Returns the number of intersections left of x.
    int countIntersections(double x) const;
    /// Returns the total sign of intersections left of x.
    int sumIntersections(double x) const;

private:
    std::vector<Intersection> intersections;
    mutable int lastIndex;

    void preprocess();
    int moveTo(double x) const;

};

}
