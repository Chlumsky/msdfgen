
#include "msdf-artifact-patcher.h"

#include <cstring>
#include <vector>
#include <utility>
#include "arithmetics.hpp"
#include "equation-solver.h"
#include "bitmap-interpolation.hpp"
#include "edge-selectors.h"
#include "contour-combiners.h"
#include "ShapeDistanceFinder.h"

namespace msdfgen {

#define PROTECTION_RADIUS_TOLERANCE 1.001

#ifdef MSDFGEN_USE_OPENMP
// std::vector<bool> cannot be modified concurrently
#define STENCIL_TYPE std::vector<char>
#else
#define STENCIL_TYPE std::vector<bool>
#endif
#define STENCIL_INDEX(x, y) (sdf.width*(y)+(x))

template <class ArtifactClassifier, template <typename> class ContourCombiner, int N>
struct ArtifactFinderData {
    ShapeDistanceFinder<ContourCombiner<PseudoDistanceSelector> > distanceFinder;
    const ArtifactClassifier artifactClassifier;
    const BitmapConstRef<float, N> sdf;
    const double invRange;
    const Vector2 pDelta;
    Point2 p, sdfCoord;
    const float *msd;
    float psd;
    bool isProtected;

    inline ArtifactFinderData(const ArtifactClassifier &artifactClassifier, const BitmapConstRef<float, N> &sdf, const Shape &shape, double range, const Vector2 &pDelta) :
        distanceFinder(shape), artifactClassifier(artifactClassifier), sdf(sdf), invRange(1/range), pDelta(pDelta) { }
};

class IndiscriminateArtifactClassifier {
    double minImproveRatio;
public:
    inline static bool observesProtected() {
        return false;
    }
    inline static bool isEquivalent(float am, float bm) {
        return am == bm;
    }
    inline explicit IndiscriminateArtifactClassifier(double minImproveRatio) : minImproveRatio(minImproveRatio) { }
    inline bool isCandidate(float am, float bm, float xm, bool isProtected) const {
        return median(am, bm, xm) != xm;
    }
    inline bool isArtifact(float refSD, float newSD, float oldSD) const {
        return minImproveRatio*fabsf(newSD-refSD) < double(fabsf(oldSD-refSD));
    }
};

class EdgePriorityArtifactClassifier {
    double minImproveRatio;
public:
    inline static bool observesProtected() {
        return true;
    }
    inline static bool isEquivalent(float am, float bm) {
        return am == bm;
    }
    inline explicit EdgePriorityArtifactClassifier(double minImproveRatio) : minImproveRatio(minImproveRatio) { }
    inline bool isCandidate(float am, float bm, float xm, bool isProtected) const {
        return (am > .5f && bm > .5f && xm < .5f) || (am < .5f && bm < .5f && xm > .5f) || (!isProtected && median(am, bm, xm) != xm);
    }
    inline bool isArtifact(float refSD, float newSD, float oldSD) const {
        float oldDelta = fabsf(oldSD-refSD);
        float newDelta = fabsf(newSD-refSD);
        return newDelta < oldDelta && (minImproveRatio*newDelta < double(oldDelta) || (refSD > .5f && newSD > .5f && oldSD < .5f) || (refSD < .5f && newSD < .5f && oldSD > .5f));
    }
};

class EdgeOnlyArtifactClassifier {
public:
    inline static bool observesProtected() {
        return false;
    }
    inline static bool isEquivalent(float am, float bm) {
        return (am <= .5f && bm <= .5f) || (am >= .5f && bm >= .5f);
    }
    inline bool isCandidate(float am, float bm, float xm, bool isProtected) const {
        return (am > .5f && bm > .5f && xm < .5f) || (am < .5f && bm < .5f && xm > .5f);
    }
    inline bool isArtifact(float refSD, float newSD, float oldSD) const {
        return fabsf(newSD-refSD) <= fabsf(oldSD-refSD) && ((refSD > .5f && newSD > .5f && oldSD < .5f) || (refSD < .5f && newSD < .5f && oldSD > .5f));
    }
};

static float interpolatedMedian(const float *a, const float *b, double t) {
    return median(
        mix(a[0], b[0], t),
        mix(a[1], b[1], t),
        mix(a[2], b[2], t)
    );
}

static float interpolatedMedian(const float *a, const float *lin, const float *quad, double t) {
    return float(median(
        t*(t*quad[0]+lin[0])+a[0],
        t*(t*quad[1]+lin[1])+a[1],
        t*(t*quad[2]+lin[2])+a[2]
    ));
}

static bool edgeBetweenTexelsChannel(const float *a, const float *b, int channel) {
    float t = (a[channel]-.5f)/(a[channel]-b[channel]);
    if (t > 0.f && t < 1.f) {
        float c[3] = {
            mix(a[0], b[0], t),
            mix(a[1], b[1], t),
            mix(a[2], b[2], t)
        };
        return median(c[0], c[1], c[2]) == c[channel];
    }
    return false;
}

static bool edgeBetweenTexels(const float *a, const float *b) {
    return (
        edgeBetweenTexelsChannel(a, b, 0) ||
        edgeBetweenTexelsChannel(a, b, 1) ||
        edgeBetweenTexelsChannel(a, b, 2)
    );
}

template <int N>
static void flagProtected(STENCIL_TYPE &stencil, const BitmapConstRef<float, N> &sdf, const Shape &shape, const Projection &projection, double range) {
    // Protect texels that are interpolated at corners
    for (std::vector<Contour>::const_iterator contour = shape.contours.begin(); contour != shape.contours.end(); ++contour)
        if (!contour->edges.empty()) {
            const EdgeSegment *prevEdge = contour->edges.back();
            for (std::vector<EdgeHolder>::const_iterator edge = contour->edges.begin(); edge != contour->edges.end(); ++edge) {
                int commonColor = prevEdge->color&(*edge)->color;
                if (!(commonColor&(commonColor-1))) { // Color switch between prevEdge and edge => a corner
                    Point2 p = projection.project((*edge)->point(0));
                    if (shape.inverseYAxis)
                        p.y = sdf.height-p.y;
                    int l = (int) floor(p.x-.5);
                    int b = (int) floor(p.y-.5);
                    int r = l+1;
                    int t = b+1;
                    if (l < sdf.width && b < sdf.height && r >= 0 && t >= 0) {
                        if (l >= 0 && b >= 0)
                            stencil[STENCIL_INDEX(l, b)] = true;
                        if (r < sdf.width && b >= 0)
                            stencil[STENCIL_INDEX(r, b)] = true;
                        if (l >= 0 && t < sdf.height)
                            stencil[STENCIL_INDEX(l, t)] = true;
                        if (r < sdf.width && t < sdf.height)
                            stencil[STENCIL_INDEX(r, t)] = true;
                    }
                }
                prevEdge = *edge;
            }
        }
    // Protect texels that contribute to edges
    float radius;
    // Horizontal:
    radius = float(PROTECTION_RADIUS_TOLERANCE*projection.unprojectVector(Vector2(1/range, 0)).length());
    for (int y = 0; y < sdf.height; ++y) {
        const float *left = sdf(0, y);
        const float *right = sdf(1, y);
        for (int x = 0; x < sdf.width-1; ++x) {
            float lm = median(left[0], left[1], left[2]);
            float rm = median(right[0], right[1], right[2]);
            if (fabsf(lm-.5f)+fabsf(rm-.5f) < radius && edgeBetweenTexels(left, right)) {
                stencil[STENCIL_INDEX(x, y)] = true;
                stencil[STENCIL_INDEX(x+1, y)] = true;
            }
            left += N, right += N;
        }
    }
    // Vertical:
    radius = float(PROTECTION_RADIUS_TOLERANCE*projection.unprojectVector(Vector2(0, 1/range)).length());
    for (int y = 0; y < sdf.height-1; ++y) {
        const float *bottom = sdf(0, y);
        const float *top = sdf(0, y+1);
        for (int x = 0; x < sdf.width; ++x) {
            float bm = median(bottom[0], bottom[1], bottom[2]);
            float tm = median(top[0], top[1], top[2]);
            if (fabsf(bm-.5f)+fabsf(tm-.5f) < radius && edgeBetweenTexels(bottom, top)) {
                stencil[STENCIL_INDEX(x, y)] = true;
                stencil[STENCIL_INDEX(x, y+1)] = true;
            }
            bottom += N, top += N;
        }
    }
    // Diagonal:
    radius = float(PROTECTION_RADIUS_TOLERANCE*projection.unprojectVector(Vector2(1/range)).length());
    for (int y = 0; y < sdf.height-1; ++y) {
        const float *lb = sdf(0, y);
        const float *rb = sdf(1, y);
        const float *lt = sdf(0, y+1);
        const float *rt = sdf(1, y+1);
        for (int x = 0; x < sdf.width-1; ++x) {
            float mlb = median(lb[0], lb[1], lb[2]);
            float mrb = median(rb[0], rb[1], rb[2]);
            float mlt = median(lt[0], lt[1], lt[2]);
            float mrt = median(rt[0], rt[1], rt[2]);
            if (fabsf(mlb-.5f)+fabsf(mrt-.5f) < radius && edgeBetweenTexels(lb, rt)) {
                stencil[STENCIL_INDEX(x, y)] = true;
                stencil[STENCIL_INDEX(x+1, y+1)] = true;
            }
            if (fabsf(mrb-.5f)+fabsf(mlt-.5f) < radius && edgeBetweenTexels(rb, lt)) {
                stencil[STENCIL_INDEX(x+1, y)] = true;
                stencil[STENCIL_INDEX(x, y+1)] = true;
            }
            lb += N, rb += N, lt += N, rt += N;
        }
    }
}

template <class ArtifactClassifier, template <typename> class ContourCombiner, int N>
static bool isArtifact(ArtifactFinderData<ArtifactClassifier, ContourCombiner, N> &data, const Vector2 &v) {
    Point2 sdfCoord = data.sdfCoord+v;
    float oldMSD[N], newMSD[3];
    interpolate(oldMSD, data.sdf, data.sdfCoord+v);
    double aWeight = (1-fabs(v.x))*(1-fabs(v.y));
    newMSD[0] = float(oldMSD[0]+aWeight*(data.psd-data.msd[0]));
    newMSD[1] = float(oldMSD[1]+aWeight*(data.psd-data.msd[1]));
    newMSD[2] = float(oldMSD[2]+aWeight*(data.psd-data.msd[2]));
    float oldPSD = median(oldMSD[0], oldMSD[1], oldMSD[2]);
    float newPSD = median(newMSD[0], newMSD[1], newMSD[2]);
    if (ArtifactClassifier::isEquivalent(oldPSD, newPSD))
        return false;
    float refPSD = float(data.invRange*data.distanceFinder.distance(data.p+v*data.pDelta)+.5);
    return data.artifactClassifier.isArtifact(refPSD, newPSD, oldPSD);
}

template <class ArtifactClassifier, template <typename> class ContourCombiner, int N>
static bool hasLinearArtifact(ArtifactFinderData<ArtifactClassifier, ContourCombiner, N> &data, const Vector2 &v, const float *b, float bm, float dA, float dB) {
    double t = (double) dA/(dA-dB);
    if (t > 0 && t < 1) {
        float xm = interpolatedMedian(data.msd, b, t);
        return data.artifactClassifier.isCandidate(data.psd, bm, xm, data.isProtected) && isArtifact(data, t*v);
    }
    return false;
}

template <class ArtifactClassifier, template <typename> class ContourCombiner, int N>
static bool hasDiagonalArtifact(ArtifactFinderData<ArtifactClassifier, ContourCombiner, N> &data, const Vector2 &v, const float *lin, const float *quad, float dm, float dA, float dBC, float dD, double tEx0, double tEx1) {
    const float *a = data.msd;
    double t[2];
    int solutions = solveQuadratic(t, dD-dBC+dA, dBC-dA-dA, dA);
    for (int i = 0; i < solutions; ++i) {
        if (t[i] > 0 && t[i] < 1) {
            float xm = interpolatedMedian(a, lin, quad, t[i]);
            bool candidate = data.artifactClassifier.isCandidate(data.psd, dm, xm, data.isProtected);
            float m[2];
            if (!candidate && tEx0 > 0 && tEx0 < 1) {
                m[0] = data.psd, m[1] = dm;
                m[tEx0 > t[i]] = interpolatedMedian(a, lin, quad, tEx0);
                candidate = data.artifactClassifier.isCandidate(m[0], m[1], xm, data.isProtected);
            }
            if (!candidate && tEx1 > 0 && tEx1 < 1) {
                m[0] = data.psd, m[1] = dm;
                m[tEx1 > t[i]] = interpolatedMedian(a, lin, quad, tEx1);
                candidate = data.artifactClassifier.isCandidate(m[0], m[1], xm, data.isProtected);
            }
            if (candidate && isArtifact(data, t[i]*v))
                return true;
        }
    }
    return false;
}

template <class ArtifactClassifier, template <typename> class ContourCombiner, int N>
static bool hasLinearArtifact(ArtifactFinderData<ArtifactClassifier, ContourCombiner, N> &data, const Vector2 &v, const float *b) {
    const float *a = data.msd;
    float bm = median(b[0], b[1], b[2]);
    return (fabsf(data.psd-.5f) > fabsf(bm-.5f) && (
        hasLinearArtifact(data, v, b, bm, a[1]-a[0], b[1]-b[0]) ||
        hasLinearArtifact(data, v, b, bm, a[2]-a[1], b[2]-b[1]) ||
        hasLinearArtifact(data, v, b, bm, a[0]-a[2], b[0]-b[2])
    ));
}

template <class ArtifactClassifier, template <typename> class ContourCombiner, int N>
static bool hasDiagonalArtifact(ArtifactFinderData<ArtifactClassifier, ContourCombiner, N> &data, const Vector2 &v, const float *b, const float *c, const float *d) {
    const float *a = data.msd;
    float dm = median(d[0], d[1], d[2]);
    if (fabsf(data.psd-.5f) > fabsf(dm-.5f)) {
        float abc[3] = {
            a[0]-b[0]-c[0],
            a[1]-b[1]-c[1],
            a[2]-b[2]-c[2]
        };
        float lin[3] = {
            -a[0]-abc[0],
            -a[1]-abc[1],
            -a[2]-abc[2]
        };
        float quad[3] = {
            d[0]+abc[0],
            d[1]+abc[1],
            d[2]+abc[2]
        };
        double tEx[3] = {
            -.5*lin[0]/quad[0],
            -.5*lin[1]/quad[1],
            -.5*lin[2]/quad[2]
        };
        return (
            hasDiagonalArtifact(data, v, lin, quad, dm, a[1]-a[0], b[1]-b[0]+c[1]-c[0], d[1]-d[0], tEx[0], tEx[1]) ||
            hasDiagonalArtifact(data, v, lin, quad, dm, a[2]-a[1], b[2]-b[1]+c[2]-c[1], d[2]-d[1], tEx[1], tEx[2]) ||
            hasDiagonalArtifact(data, v, lin, quad, dm, a[0]-a[2], b[0]-b[2]+c[0]-c[2], d[0]-d[2], tEx[2], tEx[0])
        );
    }
    return false;
}

template <template <typename> class ContourCombiner, int N, class ArtifactClassifier>
static void msdfFindArtifacts(STENCIL_TYPE &stencil, const ArtifactClassifier &artifactClassifier, const BitmapConstRef<float, N> &sdf, const Shape &shape, const Projection &projection, double range) {
    if (ArtifactClassifier::observesProtected())
        flagProtected(stencil, sdf, shape, projection, range);
    Vector2 pDelta = projection.unprojectVector(Vector2(1));
    if (shape.inverseYAxis)
        pDelta.y = -pDelta.y;
#ifdef MSDFGEN_USE_OPENMP
    #pragma omp parallel
#endif
    {
        ArtifactFinderData<ArtifactClassifier, ContourCombiner, N> data(artifactClassifier,sdf, shape, range, pDelta);
        bool rightToLeft = false;
#ifdef MSDFGEN_USE_OPENMP
        #pragma omp for
#endif
        for (int y = 0; y < sdf.height; ++y) {
            int row = shape.inverseYAxis ? sdf.height-y-1 : y;
            for (int col = 0; col < sdf.width; ++col) {
                int x = rightToLeft ? sdf.width-col-1 : col;
                data.p = projection.unproject(Point2(x+.5, y+.5));
                data.sdfCoord = Point2(x+.5, row+.5);
                data.msd = sdf(x, row);
                data.psd = median(data.msd[0], data.msd[1], data.msd[2]);
                data.isProtected = stencil[STENCIL_INDEX(x, row)] != 0;
                const float *l = NULL, *b = NULL, *r = NULL, *t = NULL;
                stencil[STENCIL_INDEX(x, row)] = (
                    (x > 0 && ((l = sdf(x-1, row)), hasLinearArtifact(data, Vector2(-1, 0), l))) ||
                    (row > 0 && ((b = sdf(x, row-1)), hasLinearArtifact(data, Vector2(0, -1), b))) ||
                    (x < sdf.width-1 && ((r = sdf(x+1, row)), hasLinearArtifact(data, Vector2(+1, 0), r))) ||
                    (row < sdf.height-1 && ((t = sdf(x, row+1)), hasLinearArtifact(data, Vector2(0, +1), t))) ||
                    (x > 0 && row > 0 && hasDiagonalArtifact(data, Vector2(-1, -1), l, b, sdf(x-1, row-1))) ||
                    (x < sdf.width-1 && row > 0 && hasDiagonalArtifact(data, Vector2(+1, -1), r, b, sdf(x+1, row-1))) ||
                    (x > 0 && row < sdf.height-1 && hasDiagonalArtifact(data, Vector2(-1, +1), l, t, sdf(x-1, row+1))) ||
                    (x < sdf.width-1 && row < sdf.height-1 && hasDiagonalArtifact(data, Vector2(+1, +1), r, t, sdf(x+1, row+1)))
                );
            }
            rightToLeft = !rightToLeft;
        }
    }
}

template <template <typename> class ContourCombiner, int N>
static void msdfPatchArtifactsInner(const BitmapRef<float, N> &sdf, const Shape &shape, const Projection &projection, double range, const GeneratorConfig &generatorConfig, const ArtifactPatcherConfig &config) {
    if (config.mode == ArtifactPatcherConfig::DISABLED)
        return;
    STENCIL_TYPE stencil(sdf.width*sdf.height);
    switch (config.mode) {
        case ArtifactPatcherConfig::DISABLED:
            break;
        case ArtifactPatcherConfig::INDISCRIMINATE:
            msdfFindArtifacts<ContourCombiner, N>(stencil, IndiscriminateArtifactClassifier(config.minImproveRatio), sdf, shape, projection, range);
            break;
        case ArtifactPatcherConfig::EDGE_PRIORITY:
            msdfFindArtifacts<ContourCombiner, N>(stencil, EdgePriorityArtifactClassifier(config.minImproveRatio), sdf, shape, projection, range);
            break;
        case ArtifactPatcherConfig::EDGE_ONLY:
            msdfFindArtifacts<ContourCombiner, N>(stencil, EdgeOnlyArtifactClassifier(), sdf, shape, projection, range);
            break;
    }
    float *texel = sdf.pixels;
    for (int y = 0; y < sdf.height; ++y) {
        for (int x = 0; x < sdf.width; ++x) {
            if (stencil[STENCIL_INDEX(x, y)]) {
                float m = median(texel[0], texel[1], texel[2]);
                texel[0] = m, texel[1] = m, texel[2] = m;
            }
            texel += N;
        }
    }
}

void msdfPatchArtifacts(const BitmapRef<float, 3> &sdf, const Shape &shape, const Projection &projection, double range, const GeneratorConfig &generatorConfig, const ArtifactPatcherConfig &config) {
    if (generatorConfig.overlapSupport)
        msdfPatchArtifactsInner<OverlappingContourCombiner>(sdf, shape, projection, range, generatorConfig, config);
    else
        msdfPatchArtifactsInner<SimpleContourCombiner>(sdf, shape, projection, range, generatorConfig, config);
}

void msdfPatchArtifacts(const BitmapRef<float, 4> &sdf, const Shape &shape, const Projection &projection, double range, const GeneratorConfig &generatorConfig, const ArtifactPatcherConfig &config) {
    if (generatorConfig.overlapSupport)
        msdfPatchArtifactsInner<OverlappingContourCombiner>(sdf, shape, projection, range, generatorConfig, config);
    else
        msdfPatchArtifactsInner<SimpleContourCombiner>(sdf, shape, projection, range, generatorConfig, config);
}

}
