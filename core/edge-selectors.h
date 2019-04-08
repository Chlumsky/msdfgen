
#pragma once

#include "Vector2.h"
#include "SignedDistance.h"
#include "edge-segments.h"

namespace msdfgen {

struct MultiDistance {
    double r, g, b;
};

/// Selects the nearest edge by its true distance.
class TrueDistanceSelector {

public:
    typedef double DistanceType;

    explicit TrueDistanceSelector(const Point2 &p = Point2());
    void addEdge(const EdgeSegment *prevEdge, const EdgeSegment *edge, const EdgeSegment *nextEdge);
    void merge(const TrueDistanceSelector &other);
    DistanceType distance() const;

private:
    Point2 p;
    SignedDistance minDistance;

};

class PseudoDistanceSelectorBase {

public:
    static bool pointFacingEdge(const EdgeSegment *prevEdge, const EdgeSegment *edge, const EdgeSegment *nextEdge, const Point2 &p, double param);

    PseudoDistanceSelectorBase();
    void addEdgeTrueDistance(const EdgeSegment *edge, const SignedDistance &distance, double param);
    void addEdgePseudoDistance(const SignedDistance &distance);
    void merge(const PseudoDistanceSelectorBase &other);
    double computeDistance(const Point2 &p) const;

private:
    SignedDistance minTrueDistance;
    SignedDistance minNegativePseudoDistance;
    SignedDistance minPositivePseudoDistance;
    const EdgeSegment *nearEdge;
    double nearEdgeParam;

};

/// Selects the nearest edge by its pseudo-distance.
class PseudoDistanceSelector : public PseudoDistanceSelectorBase {

public:
    typedef double DistanceType;

    explicit PseudoDistanceSelector(const Point2 &p = Point2());
    void addEdge(const EdgeSegment *prevEdge, const EdgeSegment *edge, const EdgeSegment *nextEdge);
    DistanceType distance() const;

private:
    Point2 p;

};

/// Selects the nearest edge for each of the three channels by its pseudo-distance.
class MultiDistanceSelector {

public:
    typedef MultiDistance DistanceType;

    explicit MultiDistanceSelector(const Point2 &p = Point2());
    void addEdge(const EdgeSegment *prevEdge, const EdgeSegment *edge, const EdgeSegment *nextEdge);
    void merge(const MultiDistanceSelector &other);
    DistanceType distance() const;

private:
    Point2 p;
    PseudoDistanceSelectorBase r, g, b;

};

}
