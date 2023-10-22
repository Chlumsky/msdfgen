
#include "sdf-error-estimation.h"

#include <cmath>
#include <cfloat>
#include "arithmetics.hpp"

namespace msdfgen {

void scanlineSDF(Scanline &line, const BitmapConstRef<float, 1> &sdf, const Projection &projection, real y, bool inverseYAxis) {
    if (!(sdf.width > 0 && sdf.height > 0))
        return line.setIntersections(std::vector<Scanline::Intersection>());
    real pixelY = clamp(projection.projectY(y)-real(.5), real(sdf.height-1));
    if (inverseYAxis)
        pixelY = sdf.height-1-pixelY;
    int b = (int) floor(pixelY);
    int t = b+1;
    real bt = pixelY-real(b);
    if (t >= sdf.height) {
        b = sdf.height-1;
        t = sdf.height-1;
        bt = 1;
    }
    bool inside = false;
    std::vector<Scanline::Intersection> intersections;
    float lv, rv = mix(*sdf(0, b), *sdf(0, t), bt);
    if ((inside = rv > .5f)) {
        Scanline::Intersection intersection = { -FLT_MAX, 1 };
        intersections.push_back(intersection);
    }
    for (int l = 0, r = 1; r < sdf.width; ++l, ++r) {
        lv = rv;
        rv = mix(*sdf(r, b), *sdf(r, t), bt);
        if (lv != rv) {
            real lr = real(.5f-lv)/real(rv-lv);
            if (lr >= real(0) && lr <= real(1)) {
                Scanline::Intersection intersection = { projection.unprojectX(real(l)+lr+real(.5)), sign(rv-lv) };
                intersections.push_back(intersection);
            }
        }
    }
#ifdef MSDFGEN_USE_CPP11
    line.setIntersections((std::vector<Scanline::Intersection> &&) intersections);
#else
    line.setIntersections(intersections);
#endif
}

template <int N>
void scanlineMSDF(Scanline &line, const BitmapConstRef<float, N> &sdf, const Projection &projection, real y, bool inverseYAxis) {
    if (!(sdf.width > 0 && sdf.height > 0))
        return line.setIntersections(std::vector<Scanline::Intersection>());
    real pixelY = clamp(projection.projectY(y)-real(.5), real(sdf.height-1));
    if (inverseYAxis)
        pixelY = sdf.height-1-pixelY;
    int b = (int) floor(pixelY);
    int t = b+1;
    real bt = pixelY-real(b);
    if (t >= sdf.height) {
        b = sdf.height-1;
        t = sdf.height-1;
        bt = 1;
    }
    bool inside = false;
    std::vector<Scanline::Intersection> intersections;
    float lv[3], rv[3];
    rv[0] = mix(sdf(0, b)[0], sdf(0, t)[0], bt);
    rv[1] = mix(sdf(0, b)[1], sdf(0, t)[1], bt);
    rv[2] = mix(sdf(0, b)[2], sdf(0, t)[2], bt);
    if ((inside = median(rv[0], rv[1], rv[2]) > .5f)) {
        Scanline::Intersection intersection = { -FLT_MAX, 1 };
        intersections.push_back(intersection);
    }
    for (int l = 0, r = 1; r < sdf.width; ++l, ++r) {
        lv[0] = rv[0], lv[1] = rv[1], lv[2] = rv[2];
        rv[0] = mix(sdf(r, b)[0], sdf(r, t)[0], bt);
        rv[1] = mix(sdf(r, b)[1], sdf(r, t)[1], bt);
        rv[2] = mix(sdf(r, b)[2], sdf(r, t)[2], bt);
        Scanline::Intersection newIntersections[4];
        int newIntersectionCount = 0;
        for (int i = 0; i < 3; ++i) {
            if (lv[i] != rv[i]) {
                real lr = real(.5f-lv[i])/real(rv[i]-lv[i]);
                if (lr >= real(0) && lr <= real(1)) {
                    float v[3] = {
                        mix(lv[0], rv[0], lr),
                        mix(lv[1], rv[1], lr),
                        mix(lv[2], rv[2], lr)
                    };
                    if (median(v[0], v[1], v[2]) == v[i]) {
                        newIntersections[newIntersectionCount].x = projection.unprojectX(real(l)+lr+real(.5));
                        newIntersections[newIntersectionCount].direction = sign(rv[i]-lv[i]);
                        ++newIntersectionCount;
                    }
                }
            }
        }
        // Sort new intersections
        if (newIntersectionCount >= 2) {
            if (newIntersections[0].x > newIntersections[1].x)
                newIntersections[3] = newIntersections[0], newIntersections[0] = newIntersections[1], newIntersections[1] = newIntersections[3];
            if (newIntersectionCount >= 3 && newIntersections[1].x > newIntersections[2].x) {
                newIntersections[3] = newIntersections[1], newIntersections[1] = newIntersections[2], newIntersections[2] = newIntersections[3];
                if (newIntersections[0].x > newIntersections[1].x)
                    newIntersections[3] = newIntersections[0], newIntersections[0] = newIntersections[1], newIntersections[1] = newIntersections[3];
            }
        }
        for (int i = 0; i < newIntersectionCount; ++i) {
            if ((newIntersections[i].direction > 0) == !inside) {
                intersections.push_back(newIntersections[i]);
                inside = !inside;
            }
        }
        // Consistency check
        float rvScalar = median(rv[0], rv[1], rv[2]);
        if ((rvScalar > .5f) != inside && rvScalar != .5f && !intersections.empty()) {
            intersections.pop_back();
            inside = !inside;
        }
    }
#ifdef MSDFGEN_USE_CPP11
    line.setIntersections((std::vector<Scanline::Intersection> &&) intersections);
#else
    line.setIntersections(intersections);
#endif
}

void scanlineSDF(Scanline &line, const BitmapConstRef<float, 3> &sdf, const Projection &projection, real y, bool inverseYAxis) {
    scanlineMSDF(line, sdf, projection, y, inverseYAxis);
}
void scanlineSDF(Scanline &line, const BitmapConstRef<float, 4> &sdf, const Projection &projection, real y, bool inverseYAxis) {
    scanlineMSDF(line, sdf, projection, y, inverseYAxis);
}

template <int N>
real estimateSDFErrorInner(const BitmapConstRef<float, N> &sdf, const Shape &shape, const Projection &projection, int scanlinesPerRow, FillRule fillRule) {
    if (sdf.width <= 1 || sdf.height <= 1 || scanlinesPerRow < 1)
        return 0;
    real subRowSize = real(1)/real(scanlinesPerRow);
    real xFrom = projection.unprojectX(real(.5));
    real xTo = projection.unprojectX(real(sdf.width)-real(.5));
    real overlapFactor = real(1)/(xTo-xFrom);
    real error = 0;
    Scanline refScanline, sdfScanline;
    for (int row = 0; row < sdf.height-1; ++row) {
        for (int subRow = 0; subRow < scanlinesPerRow; ++subRow) {
            real bt = (real(subRow)+real(.5))*subRowSize;
            real y = projection.unprojectY(real(row)+bt+real(.5));
            shape.scanline(refScanline, y);
            scanlineSDF(sdfScanline, sdf, projection, y, shape.inverseYAxis);
            error += real(1)-overlapFactor*Scanline::overlap(refScanline, sdfScanline, xFrom, xTo, fillRule);
        }
    }
    return error/real((sdf.height-1)*scanlinesPerRow);
}

real estimateSDFError(const BitmapConstRef<float, 1> &sdf, const Shape &shape, const Projection &projection, int scanlinesPerRow, FillRule fillRule) {
    return estimateSDFErrorInner(sdf, shape, projection, scanlinesPerRow, fillRule);
}
real estimateSDFError(const BitmapConstRef<float, 3> &sdf, const Shape &shape, const Projection &projection, int scanlinesPerRow, FillRule fillRule) {
    return estimateSDFErrorInner(sdf, shape, projection, scanlinesPerRow, fillRule);
}
real estimateSDFError(const BitmapConstRef<float, 4> &sdf, const Shape &shape, const Projection &projection, int scanlinesPerRow, FillRule fillRule) {
    return estimateSDFErrorInner(sdf, shape, projection, scanlinesPerRow, fillRule);
}

// Legacy API

void scanlineSDF(Scanline &line, const BitmapConstRef<float, 1> &sdf, const Vector2 &scale, const Vector2 &translate, bool inverseYAxis, real y) {
    scanlineSDF(line, sdf, Projection(scale, translate), y, inverseYAxis);
}

void scanlineSDF(Scanline &line, const BitmapConstRef<float, 3> &sdf, const Vector2 &scale, const Vector2 &translate, bool inverseYAxis, real y) {
    scanlineSDF(line, sdf, Projection(scale, translate), y, inverseYAxis);
}

void scanlineSDF(Scanline &line, const BitmapConstRef<float, 4> &sdf, const Vector2 &scale, const Vector2 &translate, bool inverseYAxis, real y) {
    scanlineSDF(line, sdf, Projection(scale, translate), y, inverseYAxis);
}

real estimateSDFError(const BitmapConstRef<float, 1> &sdf, const Shape &shape, const Vector2 &scale, const Vector2 &translate, int scanlinesPerRow, FillRule fillRule) {
    return estimateSDFError(sdf, shape, Projection(scale, translate), scanlinesPerRow, fillRule);
}

real estimateSDFError(const BitmapConstRef<float, 3> &sdf, const Shape &shape, const Vector2 &scale, const Vector2 &translate, int scanlinesPerRow, FillRule fillRule) {
    return estimateSDFError(sdf, shape, Projection(scale, translate), scanlinesPerRow, fillRule);
}

real estimateSDFError(const BitmapConstRef<float, 4> &sdf, const Shape &shape, const Vector2 &scale, const Vector2 &translate, int scanlinesPerRow, FillRule fillRule) {
    return estimateSDFError(sdf, shape, Projection(scale, translate), scanlinesPerRow, fillRule);
}

}
