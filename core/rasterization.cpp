
#include "rasterization.h"

#include <vector>
#include "arithmetics.hpp"
#include "Scanline.h"

namespace msdfgen {

static bool interpretFillRule(int intersections, FillRule fillRule) {
    switch (fillRule) {
        case FILL_NONZERO:
            return intersections != 0;
        case FILL_ODD:
            return intersections&1;
        case FILL_POSITIVE:
            return intersections > 0;
        case FILL_NEGATIVE:
            return intersections < 0;
    }
    return false;
}

void rasterize(Bitmap<float> &output, const Shape &shape, const Vector2 &scale, const Vector2 &translate, FillRule fillRule) {
    int w = output.width(), h = output.height();
    Point2 p;
    Scanline scanline;
    for (int y = 0; y < h; ++y) {
        int row = shape.inverseYAxis ? h-y-1 : y;
        p.y = (y+.5)/scale.y-translate.y;
        shape.scanline(scanline, p.y);
        for (int x = 0; x < w; ++x) {
            p.x = (x+.5)/scale.x-translate.x;
            int intersections = scanline.sumIntersections(p.x);
            bool fill = interpretFillRule(intersections, fillRule);
            output(x, row) = (float) fill;
        }
    }
}

void distanceSignCorrection(Bitmap<float> &sdf, const Shape &shape, const Vector2 &scale, const Vector2 &translate, FillRule fillRule) {
    int w = sdf.width(), h = sdf.height();
    Point2 p;
    Scanline scanline;
    for (int y = 0; y < h; ++y) {
        int row = shape.inverseYAxis ? h-y-1 : y;
        p.y = (y+.5)/scale.y-translate.y;
        shape.scanline(scanline, p.y);
        for (int x = 0; x < w; ++x) {
            p.x = (x+.5)/scale.x-translate.x;
            int intersections = scanline.sumIntersections(p.x);
            bool fill = interpretFillRule(intersections, fillRule);
            float &sd = sdf(x, row);
            if ((sd > .5f) != fill)
                sd = 1.f-sd;
        }
    }
}

void distanceSignCorrection(Bitmap<FloatRGB> &sdf, const Shape &shape, const Vector2 &scale, const Vector2 &translate, FillRule fillRule) {
    int w = sdf.width(), h = sdf.height();
    if (!(w*h))
        return;
    Point2 p;
    Scanline scanline;
    bool ambiguous = false;
    std::vector<char> matchMap;
    matchMap.resize(w*h);
    char *match = &matchMap[0];
    for (int y = 0; y < h; ++y) {
        int row = shape.inverseYAxis ? h-y-1 : y;
        p.y = (y+.5)/scale.y-translate.y;
        shape.scanline(scanline, p.y);
        for (int x = 0; x < w; ++x) {
            p.x = (x+.5)/scale.x-translate.x;
            int intersections = scanline.sumIntersections(p.x);
            bool fill = interpretFillRule(intersections, fillRule);
            FloatRGB &msd = sdf(x, row);
            float sd = median(msd.r, msd.g, msd.b);
            if (sd == .5f)
                ambiguous = true;
            else if ((sd > .5f) != fill) {
                msd.r = 1.f-msd.r;
                msd.g = 1.f-msd.g;
                msd.b = 1.f-msd.b;
                *match = -1;
            } else
                *match = 1;
            ++match;
        }
    }
    // This step is necessary to avoid artifacts when whole shape is inverted
    if (ambiguous) {
        match = &matchMap[0];
        for (int y = 0; y < h; ++y) {
            int row = shape.inverseYAxis ? h-y-1 : y;
            for (int x = 0; x < w; ++x) {
                if (!*match) {
                    int neighborMatch = 0;
                    if (x > 0) neighborMatch += *(match-1);
                    if (x < w-1) neighborMatch += *(match+1);
                    if (y > 0) neighborMatch += *(match-w);
                    if (y < h-1) neighborMatch += *(match+w);
                    if (neighborMatch < 0) {
                        FloatRGB &msd = sdf(x, row);
                        msd.r = 1.f-msd.r;
                        msd.g = 1.f-msd.g;
                        msd.b = 1.f-msd.b;
                    }
                }
                ++match;
            }
        }
    }
}

}
