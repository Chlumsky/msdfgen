
#include "edge-selectors.h"

#include "arithmetics.hpp"

namespace msdfgen {

#define DISTANCE_DELTA_FACTOR 1.001

TrueDistanceSelector::EdgeCache::EdgeCache() : absDistance(0) { }

void TrueDistanceSelector::reset(const Point2 &p) {
    double delta = DISTANCE_DELTA_FACTOR*(p-this->p).length();
    minDistance.distance += nonZeroSign(minDistance.distance)*delta;
    this->p = p;
}

void TrueDistanceSelector::addEdge(EdgeCache &cache, const EdgeSegment *prevEdge, const EdgeSegment *edge, const EdgeSegment *nextEdge) {
    double delta = DISTANCE_DELTA_FACTOR*(p-cache.point).length();
    if (cache.absDistance-delta <= fabs(minDistance.distance)) {
        double dummy;
        SignedDistance distance = edge->signedDistance(p, dummy);
        if (distance < minDistance)
            minDistance = distance;
        cache.point = p;
        cache.absDistance = fabs(distance.distance);
    }
}

void TrueDistanceSelector::merge(const TrueDistanceSelector &other) {
    if (other.minDistance < minDistance)
        minDistance = other.minDistance;
}

TrueDistanceSelector::DistanceType TrueDistanceSelector::distance() const {
    return minDistance.distance;
}

PseudoDistanceSelectorBase::EdgeCache::EdgeCache() : absDistance(0), edgeDomainDistance(0), pseudoDistance(0) { }

double PseudoDistanceSelectorBase::edgeDomainDistance(const EdgeSegment *prevEdge, const EdgeSegment *edge, const EdgeSegment *nextEdge, const Point2 &p, double param) {
    if (param < 0) {
        Vector2 prevEdgeDir = -prevEdge->direction(1).normalize(true);
        Vector2 edgeDir = edge->direction(0).normalize(true);
        Vector2 pointDir = p-edge->point(0);
        return dotProduct(pointDir, (prevEdgeDir-edgeDir).normalize(true));
    }
    if (param > 1) {
        Vector2 edgeDir = -edge->direction(1).normalize(true);
        Vector2 nextEdgeDir = nextEdge->direction(0).normalize(true);
        Vector2 pointDir = p-edge->point(1);
        return dotProduct(pointDir, (nextEdgeDir-edgeDir).normalize(true));
    }
    return 0;
}

PseudoDistanceSelectorBase::PseudoDistanceSelectorBase() : nearEdge(NULL), nearEdgeParam(0) { }

void PseudoDistanceSelectorBase::reset(double delta) {
    minTrueDistance.distance += nonZeroSign(minTrueDistance.distance)*delta;
    minNegativePseudoDistance.distance = -fabs(minTrueDistance.distance);
    minPositivePseudoDistance.distance = fabs(minTrueDistance.distance);
    nearEdge = NULL;
    nearEdgeParam = 0;
}

bool PseudoDistanceSelectorBase::isEdgeRelevant(const EdgeCache &cache, const EdgeSegment *edge, const Point2 &p) const {
    double delta = DISTANCE_DELTA_FACTOR*(p-cache.point).length();
    return (
        cache.absDistance-delta <= fabs(minTrueDistance.distance) ||
        (cache.edgeDomainDistance > 0 ?
            cache.edgeDomainDistance-delta <= 0 :
            (cache.pseudoDistance < 0 ?
                cache.pseudoDistance+delta >= minNegativePseudoDistance.distance :
                cache.pseudoDistance-delta <= minPositivePseudoDistance.distance
            )
        )
    );
}

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

SignedDistance PseudoDistanceSelectorBase::trueDistance() const {
    return minTrueDistance;
}

void PseudoDistanceSelector::reset(const Point2 &p) {
    double delta = DISTANCE_DELTA_FACTOR*(p-this->p).length();
    PseudoDistanceSelectorBase::reset(delta);
    this->p = p;
}

void PseudoDistanceSelector::addEdge(EdgeCache &cache, const EdgeSegment *prevEdge, const EdgeSegment *edge, const EdgeSegment *nextEdge) {
    if (isEdgeRelevant(cache, edge, p)) {
        double param;
        SignedDistance distance = edge->signedDistance(p, param);
        double edd = edgeDomainDistance(prevEdge, edge, nextEdge, p, param);
        addEdgeTrueDistance(edge, distance, param);
        cache.point = p;
        cache.absDistance = fabs(distance.distance);
        cache.edgeDomainDistance = edd;
        if (edd <= 0) {
            edge->distanceToPseudoDistance(distance, p, param);
            addEdgePseudoDistance(distance);
            cache.pseudoDistance = distance.distance;
        }
    }
}

PseudoDistanceSelector::DistanceType PseudoDistanceSelector::distance() const {
    return computeDistance(p);
}

void MultiDistanceSelector::reset(const Point2 &p) {
    double delta = DISTANCE_DELTA_FACTOR*(p-this->p).length();
    r.reset(delta);
    g.reset(delta);
    b.reset(delta);
    this->p = p;
}

void MultiDistanceSelector::addEdge(EdgeCache &cache, const EdgeSegment *prevEdge, const EdgeSegment *edge, const EdgeSegment *nextEdge) {
    if (
        (edge->color&RED && r.isEdgeRelevant(cache, edge, p)) ||
        (edge->color&GREEN && g.isEdgeRelevant(cache, edge, p)) ||
        (edge->color&BLUE && b.isEdgeRelevant(cache, edge, p))
    ) {
        double param;
        SignedDistance distance = edge->signedDistance(p, param);
        double edd = PseudoDistanceSelectorBase::edgeDomainDistance(prevEdge, edge, nextEdge, p, param);
        if (edge->color&RED)
            r.addEdgeTrueDistance(edge, distance, param);
        if (edge->color&GREEN)
            g.addEdgeTrueDistance(edge, distance, param);
        if (edge->color&BLUE)
            b.addEdgeTrueDistance(edge, distance, param);
        cache.point = p;
        cache.absDistance = fabs(distance.distance);
        cache.edgeDomainDistance = edd;
        if (edd <= 0) {
            edge->distanceToPseudoDistance(distance, p, param);
            if (edge->color&RED)
                r.addEdgePseudoDistance(distance);
            if (edge->color&GREEN)
                g.addEdgePseudoDistance(distance);
            if (edge->color&BLUE)
                b.addEdgePseudoDistance(distance);
            cache.pseudoDistance = distance.distance;
        }
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

SignedDistance MultiDistanceSelector::trueDistance() const {
    SignedDistance distance = r.trueDistance();
    if (g.trueDistance() < distance)
        distance = g.trueDistance();
    if (b.trueDistance() < distance)
        distance = b.trueDistance();
    return distance;
}

MultiAndTrueDistanceSelector::DistanceType MultiAndTrueDistanceSelector::distance() const {
    MultiDistance multiDistance = MultiDistanceSelector::distance();
    MultiAndTrueDistance mtd;
    mtd.r = multiDistance.r;
    mtd.g = multiDistance.g;
    mtd.b = multiDistance.b;
    mtd.a = trueDistance().distance;
    return mtd;
}

}
