
#pragma once

#include "types.h"
#include "Vector2.hpp"
#include "SignedDistance.hpp"
#include "edge-segments.h"

namespace msdfgen {

struct MultiDistance {
    real r, g, b;
};
struct MultiAndTrueDistance : MultiDistance {
    real a;
};

/// Selects the nearest edge by its true distance.
class TrueDistanceSelector {

public:
    typedef real DistanceType;

    struct EdgeCache {
        Point2 point;
        real absDistance;

        EdgeCache();
    };

    void reset(const Point2 &p);
    void addEdge(EdgeCache &cache, const EdgeSegment *prevEdge, const EdgeSegment *edge, const EdgeSegment *nextEdge);
    void merge(const TrueDistanceSelector &other);
    DistanceType distance() const;

private:
    Point2 p;
    SignedDistance minDistance;

};

class PseudoDistanceSelectorBase {

public:
    struct EdgeCache {
        Point2 point;
        real absDistance;
        real aDomainDistance, bDomainDistance;
        real aPseudoDistance, bPseudoDistance;

        EdgeCache();
    };

    static bool getPseudoDistance(real &distance, const Vector2 &ep, const Vector2 &edgeDir);

    PseudoDistanceSelectorBase();
    void reset(real delta);
    bool isEdgeRelevant(const EdgeCache &cache, const EdgeSegment *edge, const Point2 &p) const;
    void addEdgeTrueDistance(const EdgeSegment *edge, const SignedDistance &distance, real param);
    void addEdgePseudoDistance(real distance);
    void merge(const PseudoDistanceSelectorBase &other);
    real computeDistance(const Point2 &p) const;
    SignedDistance trueDistance() const;

private:
    SignedDistance minTrueDistance;
    real minNegativePseudoDistance;
    real minPositivePseudoDistance;
    const EdgeSegment *nearEdge;
    real nearEdgeParam;

};

/// Selects the nearest edge by its pseudo-distance.
class PseudoDistanceSelector : public PseudoDistanceSelectorBase {

public:
    typedef real DistanceType;

    void reset(const Point2 &p);
    void addEdge(EdgeCache &cache, const EdgeSegment *prevEdge, const EdgeSegment *edge, const EdgeSegment *nextEdge);
    DistanceType distance() const;

private:
    Point2 p;

};

/// Selects the nearest edge for each of the three channels by its pseudo-distance.
class MultiDistanceSelector {

public:
    typedef MultiDistance DistanceType;
    typedef PseudoDistanceSelectorBase::EdgeCache EdgeCache;

    void reset(const Point2 &p);
    void addEdge(EdgeCache &cache, const EdgeSegment *prevEdge, const EdgeSegment *edge, const EdgeSegment *nextEdge);
    void merge(const MultiDistanceSelector &other);
    DistanceType distance() const;
    SignedDistance trueDistance() const;

private:
    Point2 p;
    PseudoDistanceSelectorBase r, g, b;

};

/// Selects the nearest edge for each of the three color channels by its pseudo-distance and by true distance for the alpha channel.
class MultiAndTrueDistanceSelector : public MultiDistanceSelector {

public:
    typedef MultiAndTrueDistance DistanceType;

    DistanceType distance() const;

};

}
