
#include "../msdfgen.h"

#include <vector>
#include "edge-selectors.h"
#include "contour-combiners.h"

namespace msdfgen {

template <typename DistanceType>
class DistancePixelConversion;

template <>
class DistancePixelConversion<double> {
public:
    typedef float PixelType;
    inline static PixelType convert(double distance, double range) {
        return PixelType(distance/range+.5);
    }
};

template <>
class DistancePixelConversion<MultiDistance> {
public:
    typedef FloatRGB PixelType;
    inline static PixelType convert(const MultiDistance &distance, double range) {
        PixelType pixel;
        pixel.r = float(distance.r/range+.5);
        pixel.g = float(distance.g/range+.5);
        pixel.b = float(distance.b/range+.5);
        return pixel;
    }
};

template <class ContourCombiner>
void generateDistanceField(Bitmap<typename DistancePixelConversion<typename ContourCombiner::DistanceType>::PixelType> &output, const Shape &shape, double range, const Vector2 &scale, const Vector2 &translate) {
    int w = output.width(), h = output.height();

#ifdef MSDFGEN_USE_OPENMP
    #pragma omp parallel
#endif
    {
        ContourCombiner contourCombiner(shape);
        Point2 p;
#ifdef MSDFGEN_USE_OPENMP
        #pragma omp for
#endif
        for (int y = 0; y < h; ++y) {
            int row = shape.inverseYAxis ? h-y-1 : y;
            p.y = (y+.5)/scale.y-translate.y;
            for (int x = 0; x < w; ++x) {
                p.x = (x+.5)/scale.x-translate.x;

                contourCombiner.reset(p);

                for (std::vector<Contour>::const_iterator contour = shape.contours.begin(); contour != shape.contours.end(); ++contour) {
                    if (!contour->edges.empty()) {
                        ContourCombiner::EdgeSelectorType edgeSelector(p);

                        const EdgeSegment *prevEdge = contour->edges.size() >= 2 ? *(contour->edges.end()-2) : *contour->edges.begin();
                        const EdgeSegment *curEdge = contour->edges.back();
                        for (std::vector<EdgeHolder>::const_iterator edge = contour->edges.begin(); edge != contour->edges.end(); ++edge) {
                            const EdgeSegment *nextEdge = *edge;
                            edgeSelector.addEdge(prevEdge, curEdge, nextEdge);
                            prevEdge = curEdge;
                            curEdge = nextEdge;
                        }

                        contourCombiner.setContourEdge(int(contour-shape.contours.begin()), edgeSelector);
                    }
                }

                ContourCombiner::DistanceType distance = contourCombiner.distance();
                output(x, row) = DistancePixelConversion<ContourCombiner::DistanceType>::convert(distance, range);
            }
        }
    }
}

void generateSDF(Bitmap<float> &output, const Shape &shape, double range, const Vector2 &scale, const Vector2 &translate, bool overlapSupport) {
    if (overlapSupport)
        generateDistanceField<OverlappingContourCombiner<TrueDistanceSelector> >(output, shape, range, scale, translate);
    else
        generateDistanceField<SimpleContourCombiner<TrueDistanceSelector> >(output, shape, range, scale, translate);
}

void generatePseudoSDF(Bitmap<float> &output, const Shape &shape, double range, const Vector2 &scale, const Vector2 &translate, bool overlapSupport) {
    if (overlapSupport)
        generateDistanceField<OverlappingContourCombiner<PseudoDistanceSelector> >(output, shape, range, scale, translate);
    else
        generateDistanceField<SimpleContourCombiner<PseudoDistanceSelector> >(output, shape, range, scale, translate);
}

void generateMSDF(Bitmap<FloatRGB> &output, const Shape &shape, double range, const Vector2 &scale, const Vector2 &translate, double edgeThreshold, bool overlapSupport) {
    if (overlapSupport)
        generateDistanceField<OverlappingContourCombiner<MultiDistanceSelector> >(output, shape, range, scale, translate);
    else
        generateDistanceField<SimpleContourCombiner<MultiDistanceSelector> >(output, shape, range, scale, translate);
    if (edgeThreshold > 0)
        msdfErrorCorrection(output, edgeThreshold/(scale*range));
}

inline static bool detectClash(const FloatRGB &a, const FloatRGB &b, double threshold) {
    // Sort channels so that pairs (a0, b0), (a1, b1), (a2, b2) go from biggest to smallest absolute difference
    float a0 = a.r, a1 = a.g, a2 = a.b;
    float b0 = b.r, b1 = b.g, b2 = b.b;
    float tmp;
    if (fabsf(b0-a0) < fabsf(b1-a1)) {
        tmp = a0, a0 = a1, a1 = tmp;
        tmp = b0, b0 = b1, b1 = tmp;
    }
    if (fabsf(b1-a1) < fabsf(b2-a2)) {
        tmp = a1, a1 = a2, a2 = tmp;
        tmp = b1, b1 = b2, b2 = tmp;
        if (fabsf(b0-a0) < fabsf(b1-a1)) {
            tmp = a0, a0 = a1, a1 = tmp;
            tmp = b0, b0 = b1, b1 = tmp;
        }
    }
    return (fabsf(b1-a1) >= threshold) &&
        !(b0 == b1 && b0 == b2) && // Ignore if other pixel has been equalized
        fabsf(a2-.5f) >= fabsf(b2-.5f); // Out of the pair, only flag the pixel farther from a shape edge
}

void msdfErrorCorrection(Bitmap<FloatRGB> &output, const Vector2 &threshold) {
    std::vector<std::pair<int, int> > clashes;
    int w = output.width(), h = output.height();
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            if (
                (x > 0 && detectClash(output(x, y), output(x-1, y), threshold.x)) ||
                (x < w-1 && detectClash(output(x, y), output(x+1, y), threshold.x)) ||
                (y > 0 && detectClash(output(x, y), output(x, y-1), threshold.y)) ||
                (y < h-1 && detectClash(output(x, y), output(x, y+1), threshold.y))
            )
                clashes.push_back(std::make_pair(x, y));
        }
    for (std::vector<std::pair<int, int> >::const_iterator clash = clashes.begin(); clash != clashes.end(); ++clash) {
        FloatRGB &pixel = output(clash->first, clash->second);
        float med = median(pixel.r, pixel.g, pixel.b);
        pixel.r = med, pixel.g = med, pixel.b = med;
    }
#ifndef MSDFGEN_NO_DIAGONAL_CLASH_DETECTION
    clashes.clear();
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            if (
                (x > 0 && y > 0 && detectClash(output(x, y), output(x-1, y-1), threshold.x+threshold.y)) ||
                (x < w-1 && y > 0 && detectClash(output(x, y), output(x+1, y-1), threshold.x+threshold.y)) ||
                (x > 0 && y < h-1 && detectClash(output(x, y), output(x-1, y+1), threshold.x+threshold.y)) ||
                (x < w-1 && y < h-1 && detectClash(output(x, y), output(x+1, y+1), threshold.x+threshold.y))
            )
                clashes.push_back(std::make_pair(x, y));
        }
    for (std::vector<std::pair<int, int> >::const_iterator clash = clashes.begin(); clash != clashes.end(); ++clash) {
        FloatRGB &pixel = output(clash->first, clash->second);
        float med = median(pixel.r, pixel.g, pixel.b);
        pixel.r = med, pixel.g = med, pixel.b = med;
    }
#endif
}

// Legacy version

void generateSDF_legacy(Bitmap<float> &output, const Shape &shape, double range, const Vector2 &scale, const Vector2 &translate) {
    int w = output.width(), h = output.height();
#ifdef MSDFGEN_USE_OPENMP
    #pragma omp parallel for
#endif
    for (int y = 0; y < h; ++y) {
        int row = shape.inverseYAxis ? h-y-1 : y;
        for (int x = 0; x < w; ++x) {
            double dummy;
            Point2 p = Vector2(x+.5, y+.5)/scale-translate;
            SignedDistance minDistance;
            for (std::vector<Contour>::const_iterator contour = shape.contours.begin(); contour != shape.contours.end(); ++contour)
                for (std::vector<EdgeHolder>::const_iterator edge = contour->edges.begin(); edge != contour->edges.end(); ++edge) {
                    SignedDistance distance = (*edge)->signedDistance(p, dummy);
                    if (distance < minDistance)
                        minDistance = distance;
                }
            output(x, row) = float(minDistance.distance/range+.5);
        }
    }
}

void generatePseudoSDF_legacy(Bitmap<float> &output, const Shape &shape, double range, const Vector2 &scale, const Vector2 &translate) {
    int w = output.width(), h = output.height();
#ifdef MSDFGEN_USE_OPENMP
    #pragma omp parallel for
#endif
    for (int y = 0; y < h; ++y) {
        int row = shape.inverseYAxis ? h-y-1 : y;
        for (int x = 0; x < w; ++x) {
            Point2 p = Vector2(x+.5, y+.5)/scale-translate;
            SignedDistance minDistance;
            const EdgeHolder *nearEdge = NULL;
            double nearParam = 0;
            for (std::vector<Contour>::const_iterator contour = shape.contours.begin(); contour != shape.contours.end(); ++contour)
                for (std::vector<EdgeHolder>::const_iterator edge = contour->edges.begin(); edge != contour->edges.end(); ++edge) {
                    double param;
                    SignedDistance distance = (*edge)->signedDistance(p, param);
                    if (distance < minDistance) {
                        minDistance = distance;
                        nearEdge = &*edge;
                        nearParam = param;
                    }
                }
            if (nearEdge)
                (*nearEdge)->distanceToPseudoDistance(minDistance, p, nearParam);
            output(x, row) = float(minDistance.distance/range+.5);
        }
    }
}

void generateMSDF_legacy(Bitmap<FloatRGB> &output, const Shape &shape, double range, const Vector2 &scale, const Vector2 &translate, double edgeThreshold) {
    int w = output.width(), h = output.height();
#ifdef MSDFGEN_USE_OPENMP
    #pragma omp parallel for
#endif
    for (int y = 0; y < h; ++y) {
        int row = shape.inverseYAxis ? h-y-1 : y;
        for (int x = 0; x < w; ++x) {
            Point2 p = Vector2(x+.5, y+.5)/scale-translate;

            struct {
                SignedDistance minDistance;
                const EdgeHolder *nearEdge;
                double nearParam;
            } r, g, b;
            r.nearEdge = g.nearEdge = b.nearEdge = NULL;
            r.nearParam = g.nearParam = b.nearParam = 0;

            for (std::vector<Contour>::const_iterator contour = shape.contours.begin(); contour != shape.contours.end(); ++contour)
                for (std::vector<EdgeHolder>::const_iterator edge = contour->edges.begin(); edge != contour->edges.end(); ++edge) {
                    double param;
                    SignedDistance distance = (*edge)->signedDistance(p, param);
                    if ((*edge)->color&RED && distance < r.minDistance) {
                        r.minDistance = distance;
                        r.nearEdge = &*edge;
                        r.nearParam = param;
                    }
                    if ((*edge)->color&GREEN && distance < g.minDistance) {
                        g.minDistance = distance;
                        g.nearEdge = &*edge;
                        g.nearParam = param;
                    }
                    if ((*edge)->color&BLUE && distance < b.minDistance) {
                        b.minDistance = distance;
                        b.nearEdge = &*edge;
                        b.nearParam = param;
                    }
                }

            if (r.nearEdge)
                (*r.nearEdge)->distanceToPseudoDistance(r.minDistance, p, r.nearParam);
            if (g.nearEdge)
                (*g.nearEdge)->distanceToPseudoDistance(g.minDistance, p, g.nearParam);
            if (b.nearEdge)
                (*b.nearEdge)->distanceToPseudoDistance(b.minDistance, p, b.nearParam);
            output(x, row).r = float(r.minDistance.distance/range+.5);
            output(x, row).g = float(g.minDistance.distance/range+.5);
            output(x, row).b = float(b.minDistance.distance/range+.5);
        }
    }

    if (edgeThreshold > 0)
        msdfErrorCorrection(output, edgeThreshold/(scale*range));
}

}
