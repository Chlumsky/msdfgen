
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
    typedef BitmapRef<float, 1> BitmapRefType;
    inline static void convert(float *pixels, double distance, double range) {
        *pixels = float(distance/range+.5);
    }
};

template <>
class DistancePixelConversion<MultiDistance> {
public:
    typedef BitmapRef<float, 3> BitmapRefType;
    inline static void convert(float *pixels, const MultiDistance &distance, double range) {
        pixels[0] = float(distance.r/range+.5);
        pixels[1] = float(distance.g/range+.5);
        pixels[2] = float(distance.b/range+.5);
    }
};

template <>
class DistancePixelConversion<MultiAndTrueDistance> {
public:
    typedef BitmapRef<float, 4> BitmapRefType;
    inline static void convert(float *pixels, const MultiAndTrueDistance &distance, double range) {
        pixels[0] = float(distance.r/range+.5);
        pixels[1] = float(distance.g/range+.5);
        pixels[2] = float(distance.b/range+.5);
        pixels[3] = float(distance.a/range+.5);
    }
};

template <class ContourCombiner>
void generateDistanceField(const typename DistancePixelConversion<typename ContourCombiner::DistanceType>::BitmapRefType &output, const Shape &shape, double range, const Vector2 &scale, const Vector2 &translate) {
    int edgeCount = shape.edgeCount();
#ifdef MSDFGEN_USE_OPENMP
    #pragma omp parallel
#endif
    {
        ContourCombiner contourCombiner(shape);
        std::vector<typename ContourCombiner::EdgeSelectorType::EdgeCache> shapeEdgeCache(edgeCount);
        bool rightToLeft = false;
        Point2 p;
#ifdef MSDFGEN_USE_OPENMP
        #pragma omp for
#endif
        for (int y = 0; y < output.height; ++y) {
            int row = shape.inverseYAxis ? output.height-y-1 : y;
            p.y = (y+.5)/scale.y-translate.y;
            for (int col = 0; col < output.width; ++col) {
                int x = rightToLeft ? output.width-col-1 : col;
                p.x = (x+.5)/scale.x-translate.x;

                contourCombiner.reset(p);
                typename ContourCombiner::EdgeSelectorType::EdgeCache *edgeCache = &shapeEdgeCache[0];

                for (std::vector<Contour>::const_iterator contour = shape.contours.begin(); contour != shape.contours.end(); ++contour) {
                    if (!contour->edges.empty()) {
                        typename ContourCombiner::EdgeSelectorType &edgeSelector = contourCombiner.edgeSelector(int(contour-shape.contours.begin()));

                        const EdgeSegment *prevEdge = contour->edges.size() >= 2 ? *(contour->edges.end()-2) : *contour->edges.begin();
                        const EdgeSegment *curEdge = contour->edges.back();
                        for (std::vector<EdgeHolder>::const_iterator edge = contour->edges.begin(); edge != contour->edges.end(); ++edge) {
                            const EdgeSegment *nextEdge = *edge;
                            edgeSelector.addEdge(*edgeCache++, prevEdge, curEdge, nextEdge);
                            prevEdge = curEdge;
                            curEdge = nextEdge;
                        }
                    }
                }

                typename ContourCombiner::DistanceType distance = contourCombiner.distance();
                DistancePixelConversion<typename ContourCombiner::DistanceType>::convert(output(x, row), distance, range);
            }
            rightToLeft = !rightToLeft;
        }
    }
}

void generateSDF(const BitmapRef<float, 1> &output, const Shape &shape, double range, const Vector2 &scale, const Vector2 &translate, bool overlapSupport) {
    if (overlapSupport)
        generateDistanceField<OverlappingContourCombiner<TrueDistanceSelector> >(output, shape, range, scale, translate);
    else
        generateDistanceField<SimpleContourCombiner<TrueDistanceSelector> >(output, shape, range, scale, translate);
}

void generatePseudoSDF(const BitmapRef<float, 1> &output, const Shape &shape, double range, const Vector2 &scale, const Vector2 &translate, bool overlapSupport) {
    if (overlapSupport)
        generateDistanceField<OverlappingContourCombiner<PseudoDistanceSelector> >(output, shape, range, scale, translate);
    else
        generateDistanceField<SimpleContourCombiner<PseudoDistanceSelector> >(output, shape, range, scale, translate);
}

void generateMSDF(const BitmapRef<float, 3> &output, const Shape &shape, double range, const Vector2 &scale, const Vector2 &translate, double edgeThreshold, bool overlapSupport) {
    if (overlapSupport)
        generateDistanceField<OverlappingContourCombiner<MultiDistanceSelector> >(output, shape, range, scale, translate);
    else
        generateDistanceField<SimpleContourCombiner<MultiDistanceSelector> >(output, shape, range, scale, translate);
    if (edgeThreshold > 0)
        msdfErrorCorrection(output, edgeThreshold/(scale*range));
}

void generateMTSDF(const BitmapRef<float, 4> &output, const Shape &shape, double range, const Vector2 &scale, const Vector2 &translate, double edgeThreshold, bool overlapSupport) {
    if (overlapSupport)
        generateDistanceField<OverlappingContourCombiner<MultiAndTrueDistanceSelector> >(output, shape, range, scale, translate);
    else
        generateDistanceField<SimpleContourCombiner<MultiAndTrueDistanceSelector> >(output, shape, range, scale, translate);
    if (edgeThreshold > 0)
        msdfErrorCorrection(output, edgeThreshold/(scale*range));
}

inline static bool detectClash(const float *a, const float *b, double threshold) {
    // Sort channels so that pairs (a0, b0), (a1, b1), (a2, b2) go from biggest to smallest absolute difference
    float a0 = a[0], a1 = a[1], a2 = a[2];
    float b0 = b[0], b1 = b[1], b2 = b[2];
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

template <int N>
void msdfErrorCorrectionInner(const BitmapRef<float, N> &output, const Vector2 &threshold) {
    std::vector<std::pair<int, int> > clashes;
    int w = output.width, h = output.height;
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
        float *pixel = output(clash->first, clash->second);
        float med = median(pixel[0], pixel[1], pixel[2]);
        pixel[0] = med, pixel[1] = med, pixel[2] = med;
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
        float *pixel = output(clash->first, clash->second);
        float med = median(pixel[0], pixel[1], pixel[2]);
        pixel[0] = med, pixel[1] = med, pixel[2] = med;
    }
#endif
}

void msdfErrorCorrection(const BitmapRef<float, 3> &output, const Vector2 &threshold) {
    msdfErrorCorrectionInner(output, threshold);
}
void msdfErrorCorrection(const BitmapRef<float, 4> &output, const Vector2 &threshold) {
    msdfErrorCorrectionInner(output, threshold);
}

// Legacy version

void generateSDF_legacy(const BitmapRef<float, 1> &output, const Shape &shape, double range, const Vector2 &scale, const Vector2 &translate) {
#ifdef MSDFGEN_USE_OPENMP
    #pragma omp parallel for
#endif
    for (int y = 0; y < output.height; ++y) {
        int row = shape.inverseYAxis ? output.height-y-1 : y;
        for (int x = 0; x < output.width; ++x) {
            double dummy;
            Point2 p = Vector2(x+.5, y+.5)/scale-translate;
            SignedDistance minDistance;
            for (std::vector<Contour>::const_iterator contour = shape.contours.begin(); contour != shape.contours.end(); ++contour)
                for (std::vector<EdgeHolder>::const_iterator edge = contour->edges.begin(); edge != contour->edges.end(); ++edge) {
                    SignedDistance distance = (*edge)->signedDistance(p, dummy);
                    if (distance < minDistance)
                        minDistance = distance;
                }
            *output(x, row) = float(minDistance.distance/range+.5);
        }
    }
}

void generatePseudoSDF_legacy(const BitmapRef<float, 1> &output, const Shape &shape, double range, const Vector2 &scale, const Vector2 &translate) {
#ifdef MSDFGEN_USE_OPENMP
    #pragma omp parallel for
#endif
    for (int y = 0; y < output.height; ++y) {
        int row = shape.inverseYAxis ? output.height-y-1 : y;
        for (int x = 0; x < output.width; ++x) {
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
            *output(x, row) = float(minDistance.distance/range+.5);
        }
    }
}

void generateMSDF_legacy(const BitmapRef<float, 3> &output, const Shape &shape, double range, const Vector2 &scale, const Vector2 &translate, double edgeThreshold) {
#ifdef MSDFGEN_USE_OPENMP
    #pragma omp parallel for
#endif
    for (int y = 0; y < output.height; ++y) {
        int row = shape.inverseYAxis ? output.height-y-1 : y;
        for (int x = 0; x < output.width; ++x) {
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
            output(x, row)[0] = float(r.minDistance.distance/range+.5);
            output(x, row)[1] = float(g.minDistance.distance/range+.5);
            output(x, row)[2] = float(b.minDistance.distance/range+.5);
        }
    }

    if (edgeThreshold > 0)
        msdfErrorCorrection(output, edgeThreshold/(scale*range));
}

void generateMTSDF_legacy(const BitmapRef<float, 4> &output, const Shape &shape, double range, const Vector2 &scale, const Vector2 &translate, double edgeThreshold) {
#ifdef MSDFGEN_USE_OPENMP
    #pragma omp parallel for
#endif
    for (int y = 0; y < output.height; ++y) {
        int row = shape.inverseYAxis ? output.height-y-1 : y;
        for (int x = 0; x < output.width; ++x) {
            Point2 p = Vector2(x+.5, y+.5)/scale-translate;

            SignedDistance minDistance;
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
                    if (distance < minDistance)
                        minDistance = distance;
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
            output(x, row)[0] = float(r.minDistance.distance/range+.5);
            output(x, row)[1] = float(g.minDistance.distance/range+.5);
            output(x, row)[2] = float(b.minDistance.distance/range+.5);
            output(x, row)[3] = float(minDistance.distance/range+.5);
        }
    }

    if (edgeThreshold > 0)
        msdfErrorCorrection(output, edgeThreshold/(scale*range));
}

}
