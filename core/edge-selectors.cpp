
#include "edge-selectors.h"

namespace msdfgen {

TrueDistanceSelector::TrueDistanceSelector(const Point2 &p) : p(p) { }

void TrueDistanceSelector::addEdge(const EdgeSegment *prevEdge, const EdgeSegment *edge, const EdgeSegment *nextEdge) {
    double dummy;
    SignedDistance distance = edge->signedDistance(p, dummy);
    if (distance < minDistance)
        minDistance = distance;
}

void TrueDistanceSelector::merge(const TrueDistanceSelector &other) {
    if (other.minDistance < minDistance)
        minDistance = other.minDistance;
}

TrueDistanceSelector::DistanceType TrueDistanceSelector::distance() const {
    return minDistance.distance;
}

bool PseudoDistanceSelectorBase::pointFacingEdge(const EdgeSegment *prevEdge, const EdgeSegment *edge, const EdgeSegment *nextEdge, const Point2 &p, double param) {
    if (param < 0) {
        Vector2 prevEdgeDir = -prevEdge->direction(1).normalize(true);
        Vector2 edgeDir = edge->direction(0).normalize(true);
        Vector2 pointDir = p-edge->point(0);
        return dotProduct(pointDir, edgeDir) >= dotProduct(pointDir, prevEdgeDir);
    }
    if (param > 1) {
        Vector2 edgeDir = -edge->direction(1).normalize(true);
        Vector2 nextEdgeDir = nextEdge->direction(0).normalize(true);
        Vector2 pointDir = p-edge->point(1);
        return dotProduct(pointDir, edgeDir) >= dotProduct(pointDir, nextEdgeDir);
    }
    return true;
}

PseudoDistanceSelectorBase::PseudoDistanceSelectorBase() : nearEdge(NULL), nearEdgeParam(0) { }

void PseudoDistanceSelectorBase::addEdgeTrueDistance(const EdgeSegment *edge, const SignedDistance &distance, double param) {
    if (distance < minTrueDistance) {
        minTrueDistance = distance;
        nearEdge = edge;
        nearEdgeParam = param;
    }
}

void PseudoDistanceSelectorBase::addEdgePseudoDistance(const SignedDistance &distance) {
    SignedDistance &minPseudoDistance = distance.distance < 0 ? minNegativePseudoDistance : minPositivePseudoDistance;
    if (distance < minPseudoDistance)
        minPseudoDistance = distance;
}

void PseudoDistanceSelectorBase::merge(const PseudoDistanceSelectorBase &other) {
    if (other.minTrueDistance < minTrueDistance) {
        minTrueDistance = other.minTrueDistance;
        nearEdge = other.nearEdge;
        nearEdgeParam = other.nearEdgeParam;
    }
    if (other.minNegativePseudoDistance < minNegativePseudoDistance)
        minNegativePseudoDistance = other.minNegativePseudoDistance;
    if (other.minPositivePseudoDistance < minPositivePseudoDistance)
        minPositivePseudoDistance = other.minPositivePseudoDistance;
}

double PseudoDistanceSelectorBase::computeDistance(const Point2 &p) const {
    double minDistance = minTrueDistance.distance < 0 ? minNegativePseudoDistance.distance : minPositivePseudoDistance.distance;
    if (nearEdge) {
        SignedDistance distance = minTrueDistance;
        nearEdge->distanceToPseudoDistance(distance, p, nearEdgeParam);
        if (fabs(distance.distance) < fabs(minDistance))
            minDistance = distance.distance;
    }
    return minDistance;
}

PseudoDistanceSelector::PseudoDistanceSelector(const Point2 &p) : p(p) { }

void PseudoDistanceSelector::addEdge(const EdgeSegment *prevEdge, const EdgeSegment *edge, const EdgeSegment *nextEdge) {
    double param;
    SignedDistance distance = edge->signedDistance(p, param);
    addEdgeTrueDistance(edge, distance, param);
    if (pointFacingEdge(prevEdge, edge, nextEdge, p, param)) {
        edge->distanceToPseudoDistance(distance, p, param);
        addEdgePseudoDistance(distance);
    }
}

PseudoDistanceSelector::DistanceType PseudoDistanceSelector::distance() const {
    return computeDistance(p);
}

MultiDistanceSelector::MultiDistanceSelector(const Point2 &p) : p(p) { }

void MultiDistanceSelector::addEdge(const EdgeSegment *prevEdge, const EdgeSegment *edge, const EdgeSegment *nextEdge) {
    double param;
    SignedDistance distance = edge->signedDistance(p, param);
    if (edge->color&RED)
        r.addEdgeTrueDistance(edge, distance, param);
    if (edge->color&GREEN)
        g.addEdgeTrueDistance(edge, distance, param);
    if (edge->color&BLUE)
        b.addEdgeTrueDistance(edge, distance, param);
    if (PseudoDistanceSelectorBase::pointFacingEdge(prevEdge, edge, nextEdge, p, param)) {
        edge->distanceToPseudoDistance(distance, p, param);
        if (edge->color&RED)
            r.addEdgePseudoDistance(distance);
        if (edge->color&GREEN)
            g.addEdgePseudoDistance(distance);
        if (edge->color&BLUE)
            b.addEdgePseudoDistance(distance);
    }
}

void MultiDistanceSelector::merge(const MultiDistanceSelector &other) {
    r.merge(other.r);
    g.merge(other.g);
    b.merge(other.b);
}

MultiDistanceSelector::DistanceType MultiDistanceSelector::distance() const {
    MultiDistance multiDistance;
    multiDistance.r = r.computeDistance(p);
    multiDistance.g = g.computeDistance(p);
    multiDistance.b = b.computeDistance(p);
    return multiDistance;
}

}
