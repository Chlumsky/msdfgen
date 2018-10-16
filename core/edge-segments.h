
#pragma once

#include "Vector2.h"
#include "SignedDistance.h"
#include "EdgeColor.h"

namespace msdfgen {

// Parameters for iterative search of closest point on a cubic Bezier curve. Increase for higher precision.
#define MSDFGEN_CUBIC_SEARCH_STARTS 4
#define MSDFGEN_CUBIC_SEARCH_STEPS 4

/// An abstract edge segment.
class EdgeSegment {

public:
    EdgeColor color;

    EdgeSegment(EdgeColor edgeColor = WHITE) : color(edgeColor) { }
    virtual ~EdgeSegment() { }
    /// Creates a copy of the edge segment.
    virtual EdgeSegment * clone() const = 0;
    /// Returns the point on the edge specified by the parameter (between 0 and 1).
    virtual Point2 point(double param) const = 0;
    /// Returns the direction the edge has at the point specified by the parameter.
    virtual Vector2 direction(double param) const = 0;
    /// Returns the minimum signed distance between origin and the edge.
    virtual SignedDistance signedDistance(Point2 origin, double &param) const = 0;
    /// Converts a previously retrieved signed distance from origin to pseudo-distance.
    virtual void distanceToPseudoDistance(SignedDistance &distance, Point2 origin, double param) const;
    /// Adjusts the bounding box to fit the edge segment.
    virtual void bounds(double &l, double &b, double &r, double &t) const = 0;

    /// Moves the start point of the edge segment.
    virtual void moveStartPoint(Point2 to) = 0;
    /// Moves the end point of the edge segment.
    virtual void moveEndPoint(Point2 to) = 0;
    /// Splits the edge segments into thirds which together represent the original edge.
    virtual void splitInThirds(EdgeSegment *&part1, EdgeSegment *&part2, EdgeSegment *&part3) const = 0;
    
    virtual bool isDegenerate() const = 0;

    
    
    class CrossingCallback {
    public:
        virtual ~CrossingCallback() {}
        /// Callback for receiving intersection points. Winding is either:
        /// +1 if the segment is increasing in the Y axis
        /// -1 if the segment is decreasing in the Y axis
        virtual void intersection(const Point2& p, int winding) = 0;
    };
    
    /// Calculate how many times the segment intersects the infinite ray that extends
    /// to the right (+X) from the given point. Returns:
    ///  0 for no intersection (or co-linear)
    /// +1 for each intersection where Y is increasing
    /// -1 for each intersection where Y is decreasing.
    virtual int crossings(const Point2 &r, CrossingCallback *cb = NULL) const = 0;
    
    
};

/// A line segment.
class LinearSegment : public EdgeSegment {

public:
    Point2 p[2];

    LinearSegment(Point2 p0, Point2 p1, EdgeColor edgeColor = WHITE);
    LinearSegment * clone() const;
    Point2 point(double param) const;
    Vector2 direction(double param) const;
    SignedDistance signedDistance(Point2 origin, double &param) const;
    void bounds(double &l, double &b, double &r, double &t) const;

    void moveStartPoint(Point2 to);
    void moveEndPoint(Point2 to);
    void splitInThirds(EdgeSegment *&part1, EdgeSegment *&part2, EdgeSegment *&part3) const;
    
    bool isDegenerate() const;

    int crossings(const Point2 &r, CrossingCallback *cb = NULL) const;
};

/// A quadratic Bezier curve.
class QuadraticSegment : public EdgeSegment {

public:
    Point2 p[3];

    QuadraticSegment(Point2 p0, Point2 p1, Point2 p2, EdgeColor edgeColor = WHITE);
    QuadraticSegment * clone() const;
    Point2 point(double param) const;
    Vector2 direction(double param) const;
    SignedDistance signedDistance(Point2 origin, double &param) const;
    void bounds(double &l, double &b, double &r, double &t) const;

    void moveStartPoint(Point2 to);
    void moveEndPoint(Point2 to);
    void splitInThirds(EdgeSegment *&part1, EdgeSegment *&part2, EdgeSegment *&part3) const;

    bool isDegenerate() const;

    int crossings(const Point2 &r, CrossingCallback *cb = NULL) const;
};

/// A cubic Bezier curve.
class CubicSegment : public EdgeSegment {

public:
    Point2 p[4];

    CubicSegment(Point2 p0, Point2 p1, Point2 p2, Point2 p3, EdgeColor edgeColor = WHITE);
    CubicSegment * clone() const;
    Point2 point(double param) const;
    Vector2 direction(double param) const;
    SignedDistance signedDistance(Point2 origin, double &param) const;
    void bounds(double &l, double &b, double &r, double &t) const;

    void moveStartPoint(Point2 to);
    void moveEndPoint(Point2 to);
    void splitInThirds(EdgeSegment *&part1, EdgeSegment *&part2, EdgeSegment *&part3) const;

    bool isDegenerate() const;

    int crossings(const Point2 &r, CrossingCallback *cb = NULL) const;
};

}
