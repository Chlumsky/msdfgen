
#include "contour-combiners.h"

#include "arithmetics.hpp"

namespace msdfgen {

static void initDistance(double &sqDistance) {
    sqDistance = SignedDistance::INFINITE.sqDistance;
}

static void initDistance(MultiDistance &sqDistance) {
    sqDistance.r = SignedDistance::INFINITE.sqDistance;
    sqDistance.g = SignedDistance::INFINITE.sqDistance;
    sqDistance.b = SignedDistance::INFINITE.sqDistance;
}

static double resolveDistance(double sqDistance) {
    return sqDistance;
}

static double resolveDistance(const MultiDistance &sqDistance) {
    return median(sqDistance.r, sqDistance.g, sqDistance.b);
}

template <class EdgeSelector>
SimpleContourCombiner<EdgeSelector>::SimpleContourCombiner(const Shape &shape) { }

template <class EdgeSelector>
void SimpleContourCombiner<EdgeSelector>::reset(const Point2 &p) {
    shapeEdgeSelector = EdgeSelector(p);
}

template <class EdgeSelector>
void SimpleContourCombiner<EdgeSelector>::setContourEdgeSelection(int i, const EdgeSelector &edgeSelector) {
    shapeEdgeSelector.merge(edgeSelector);
}

template <class EdgeSelector>
typename SimpleContourCombiner<EdgeSelector>::DistanceType SimpleContourCombiner<EdgeSelector>::squaredDistance() const {
    return shapeEdgeSelector.squaredDistance();
}

template class SimpleContourCombiner<TrueDistanceSelector>;
template class SimpleContourCombiner<PseudoDistanceSelector>;
template class SimpleContourCombiner<MultiDistanceSelector>;

template <class EdgeSelector>
OverlappingContourCombiner<EdgeSelector>::OverlappingContourCombiner(const Shape &shape) {
    windings.reserve(shape.contours.size());
    for (std::vector<Contour>::const_iterator contour = shape.contours.begin(); contour != shape.contours.end(); ++contour)
        windings.push_back(contour->winding());
    edgeSelectors.resize(shape.contours.size());
}

template <class EdgeSelector>
void OverlappingContourCombiner<EdgeSelector>::reset(const Point2 &p) {
    shapeEdgeSelector = EdgeSelector(p);
    innerEdgeSelector = EdgeSelector(p);
    outerEdgeSelector = EdgeSelector(p);
}

template <class EdgeSelector>
void OverlappingContourCombiner<EdgeSelector>::setContourEdgeSelection(int i, const EdgeSelector &edgeSelector) {
    DistanceType edgeSqDistance = edgeSelector.squaredDistance();
    edgeSelectors[i] = edgeSelector;
    shapeEdgeSelector.merge(edgeSelector);
    if (windings[i] > 0 && resolveDistance(edgeSqDistance) >= 0)
        innerEdgeSelector.merge(edgeSelector);
    if (windings[i] < 0 && resolveDistance(edgeSqDistance) <= 0)
        outerEdgeSelector.merge(edgeSelector);
}

template <class EdgeSelector>
typename OverlappingContourCombiner<EdgeSelector>::DistanceType OverlappingContourCombiner<EdgeSelector>::squaredDistance() const {
    DistanceType shapeSqDistance = shapeEdgeSelector.squaredDistance();
    DistanceType innerSqDistance = innerEdgeSelector.squaredDistance();
    DistanceType outerSqDistance = outerEdgeSelector.squaredDistance();
    double innerScalarSqDistance = resolveDistance(innerSqDistance);
    double outerScalarSqDistance = resolveDistance(outerSqDistance);
    DistanceType sqDistance;
    initDistance(sqDistance);
    int contourCount = (int) windings.size();

    int winding = 0;
    if (innerScalarSqDistance >= 0 && fabs(innerScalarSqDistance) <= fabs(outerScalarSqDistance)) {
        sqDistance = innerSqDistance;
        winding = 1;
        for (int i = 0; i < contourCount; ++i)
            if (windings[i] > 0) {
                DistanceType contourSqDistance = edgeSelectors[i].squaredDistance();
                if (fabs(resolveDistance(contourSqDistance)) < fabs(outerScalarSqDistance) && resolveDistance(contourSqDistance) > resolveDistance(sqDistance))
                    sqDistance = contourSqDistance;
            }
    } else if (outerScalarSqDistance <= 0 && fabs(outerScalarSqDistance) < fabs(innerScalarSqDistance)) {
        sqDistance = outerSqDistance;
        winding = -1;
        for (int i = 0; i < contourCount; ++i)
            if (windings[i] < 0) {
                DistanceType contourSqDistance = edgeSelectors[i].squaredDistance();
                if (fabs(resolveDistance(contourSqDistance)) < fabs(innerScalarSqDistance) && resolveDistance(contourSqDistance) < resolveDistance(sqDistance))
                    sqDistance = contourSqDistance;
            }
    } else
        return shapeSqDistance;

    for (int i = 0; i < contourCount; ++i)
        if (windings[i] != winding) {
            DistanceType contourSqDistance = edgeSelectors[i].squaredDistance();
            if (resolveDistance(contourSqDistance)*resolveDistance(sqDistance) >= 0 && fabs(resolveDistance(contourSqDistance)) < fabs(resolveDistance(sqDistance)))
                sqDistance = contourSqDistance;
        }
    if (resolveDistance(sqDistance) == resolveDistance(shapeSqDistance))
        sqDistance = shapeSqDistance;
    return sqDistance;
}

template class OverlappingContourCombiner<TrueDistanceSelector>;
template class OverlappingContourCombiner<PseudoDistanceSelector>;
template class OverlappingContourCombiner<MultiDistanceSelector>;

}
