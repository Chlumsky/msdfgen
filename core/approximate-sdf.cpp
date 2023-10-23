
#include "approximate-sdf.h"

#include <cmath>
#include <queue>
#include "arithmetics.hpp"

#define ESTSDF_MAX_DIST 1e24f // Cannot be FLT_MAX because it might be divided by range, which could be < 1

namespace msdfgen {

void approximateSDF(const BitmapRef<float, 1> &output, const Shape &shape, const Projection &projection, double outerRange, double innerRange) {
    struct Entry {
        float absDist;
        int bitmapX, bitmapY;
        Point2 nearPoint;

        bool operator<(const Entry &other) const {
            return absDist > other.absDist;
        }
    } entry;

    float *firstRow = output.pixels;
    ptrdiff_t stride = output.width;
    if (shape.inverseYAxis) {
        firstRow += (output.height-1)*stride;
        stride = -stride;
    }
    #define ESTSDF_PIXEL_AT(x, y) ((firstRow+(y)*stride)[x])

    for (float *p = output.pixels, *end = output.pixels+output.width*output.height; p < end; ++p)
        *p = -ESTSDF_MAX_DIST;

    Vector2 invScale = projection.unprojectVector(Vector2(1));
    float dLimit = float(max(outerRange, innerRange));
    std::priority_queue<Entry> queue;
    double x[3], y[3];
    int dx[3], dy[3];

    // Horizontal scanlines
    for (int bitmapY = 0; bitmapY < output.height; ++bitmapY) {
        float *row = firstRow+bitmapY*stride;
        double y = projection.unprojectY(bitmapY+.5);
        entry.bitmapY = bitmapY;
        for (std::vector<Contour>::const_iterator contour = shape.contours.begin(); contour != shape.contours.end(); ++contour) {
            for (std::vector<EdgeHolder>::const_iterator edge = contour->edges.begin(); edge != contour->edges.end(); ++edge) {
                int n = (*edge)->horizontalScanlineIntersections(x, dy, y);
                for (int i = 0; i < n; ++i) {
                    double bitmapX = projection.projectX(x[i]);
                    double bitmapX0 = floor(bitmapX-.5)+.5;
                    double bitmapX1 = bitmapX0+1;
                    if (bitmapX1 > 0 && bitmapX0 < output.width) {
                        float sd0 = float(dy[i]*invScale.x*(bitmapX0-bitmapX));
                        float sd1 = float(dy[i]*invScale.x*(bitmapX1-bitmapX));
                        if (sd0 == 0.f) {
                            if (sd1 == 0.f)
                                continue;
                            sd0 = -.000001f*float(sign(sd1));
                        }
                        if (sd1 == 0.f)
                            sd1 = -.000001f*float(sign(sd0));
                        if (bitmapX0 > 0) {
                            entry.absDist = fabsf(sd0);
                            entry.bitmapX = int(bitmapX0);
                            float &sd = row[entry.bitmapX];
                            if (entry.absDist < fabsf(sd)) {
                                sd = sd0;
                                entry.nearPoint = Point2(x[i], y);
                                queue.push(entry);
                            } else if (sd == -sd0)
                                sd = -ESTSDF_MAX_DIST;
                        }
                        if (bitmapX1 < output.width) {
                            entry.absDist = fabsf(sd1);
                            entry.bitmapX = int(bitmapX1);
                            float &sd = row[entry.bitmapX];
                            if (entry.absDist < fabsf(sd)) {
                                sd = sd1;
                                entry.nearPoint = Point2(x[i], y);
                                queue.push(entry);
                            } else if (sd == -sd1)
                                sd = -ESTSDF_MAX_DIST;
                        }
                    }
                }
            }
        }
    }

    // Bake in distance signs
    for (int y = 0; y < output.height; ++y) {
        float *row = firstRow+y*stride;
        int x = 0;
        for (; x < output.width && row[x] == -ESTSDF_MAX_DIST; ++x);
        if (x < output.width) {
            bool flip = row[x] > 0;
            if (flip) {
                for (int i = 0; i < x; ++i)
                    row[i] = ESTSDF_MAX_DIST;
            }
            for (; x < output.width; ++x) {
                if (row[x] != -ESTSDF_MAX_DIST)
                    flip = row[x] > 0;
                else if (flip)
                    row[x] = ESTSDF_MAX_DIST;
            }
        }
    }

    // Vertical scanlines
    for (int bitmapX = 0; bitmapX < output.width; ++bitmapX) {
        double x = projection.unprojectX(bitmapX+.5);
        entry.bitmapX = bitmapX;
        for (std::vector<Contour>::const_iterator contour = shape.contours.begin(); contour != shape.contours.end(); ++contour) {
            for (std::vector<EdgeHolder>::const_iterator edge = contour->edges.begin(); edge != contour->edges.end(); ++edge) {
                int n = (*edge)->verticalScanlineIntersections(y, dx, x);
                for (int i = 0; i < n; ++i) {
                    double bitmapY = projection.projectY(y[i]);
                    double bitmapY0 = floor(bitmapY-.5)+.5;
                    double bitmapY1 = bitmapY0+1;
                    if (bitmapY0 > 0 && bitmapY1 < output.height) {
                        float sd0 = float(dx[i]*invScale.y*(bitmapY-bitmapY0));
                        float sd1 = float(dx[i]*invScale.y*(bitmapY-bitmapY1));
                        if (sd0 == 0.f) {
                            if (sd1 == 0.f)
                                continue;
                            sd0 = -.000001f*float(sign(sd1));
                        }
                        if (sd1 == 0.f)
                            sd1 = -.000001f*float(sign(sd0));
                        if (bitmapY0 > 0) {
                            entry.absDist = fabsf(sd0);
                            entry.bitmapY = int(bitmapY0);
                            float &sd = ESTSDF_PIXEL_AT(bitmapX, entry.bitmapY);
                            if (entry.absDist < fabsf(sd)) {
                                sd = sd0;
                                entry.nearPoint = Point2(x, y[i]);
                                queue.push(entry);
                            }
                        }
                        if (bitmapY1 < output.height) {
                            entry.absDist = fabsf(sd1);
                            entry.bitmapY = int(bitmapY1);
                            float &sd = ESTSDF_PIXEL_AT(bitmapX, entry.bitmapY);
                            if (entry.absDist < fabsf(sd)) {
                                sd = sd1;
                                entry.nearPoint = Point2(x, y[i]);
                                queue.push(entry);
                            }
                        }
                    }
                }
            }
        }
    }

    if (queue.empty())
        return;

    while (!queue.empty()) {
        Entry entry = queue.top();
        queue.pop();
        Entry newEntry = entry;
        newEntry.bitmapX = entry.bitmapX-1;
        if (newEntry.bitmapX >= 0) {
            float &sd = ESTSDF_PIXEL_AT(newEntry.bitmapX, newEntry.bitmapY);
            if (fabsf(sd) == ESTSDF_MAX_DIST) {
                Point2 shapeCoord = projection.unproject(Point2(newEntry.bitmapX+.5, newEntry.bitmapY+.5));
                newEntry.absDist = float((shapeCoord-entry.nearPoint).length());
                sd = float(sign(sd))*newEntry.absDist;
                if (newEntry.absDist < dLimit)
                    queue.push(newEntry);
            }
        }
        newEntry.bitmapX = entry.bitmapX+1;
        if (newEntry.bitmapX < output.width) {
            float &sd = ESTSDF_PIXEL_AT(newEntry.bitmapX, newEntry.bitmapY);
            if (fabsf(sd) == ESTSDF_MAX_DIST) {
                Point2 shapeCoord = projection.unproject(Point2(newEntry.bitmapX+.5, newEntry.bitmapY+.5));
                newEntry.absDist = float((shapeCoord-entry.nearPoint).length());
                sd = float(sign(sd))*newEntry.absDist;
                if (newEntry.absDist < dLimit)
                    queue.push(newEntry);
            }
        }
        newEntry.bitmapX = entry.bitmapX;
        newEntry.bitmapY = entry.bitmapY-1;
        if (newEntry.bitmapY >= 0) {
            float &sd = ESTSDF_PIXEL_AT(newEntry.bitmapX, newEntry.bitmapY);
            if (fabsf(sd) == ESTSDF_MAX_DIST) {
                Point2 shapeCoord = projection.unproject(Point2(newEntry.bitmapX+.5, newEntry.bitmapY+.5));
                newEntry.absDist = float((shapeCoord-entry.nearPoint).length());
                sd = float(sign(sd))*newEntry.absDist;
                if (newEntry.absDist < dLimit)
                    queue.push(newEntry);
            }
        }
        newEntry.bitmapY = entry.bitmapY+1;
        if (newEntry.bitmapY < output.height) {
            float &sd = ESTSDF_PIXEL_AT(newEntry.bitmapX, newEntry.bitmapY);
            if (fabsf(sd) == ESTSDF_MAX_DIST) {
                Point2 shapeCoord = projection.unproject(Point2(newEntry.bitmapX+.5, newEntry.bitmapY+.5));
                newEntry.absDist = float((shapeCoord-entry.nearPoint).length());
                sd = float(sign(sd))*newEntry.absDist;
                if (newEntry.absDist < dLimit)
                    queue.push(newEntry);
            }
        }
    }

    float rangeFactor = 1.f/float(outerRange+innerRange);
    float zeroBias = rangeFactor*float(outerRange);
    for (float *p = output.pixels, *end = output.pixels+output.width*output.height; p < end; ++p)
        *p = rangeFactor**p+zeroBias;
}

void approximateSDF(const BitmapRef<float, 1> &output, const Shape &shape, const Projection &projection, double range) {
    approximateSDF(output, shape, projection, .5*range, .5*range);
}

}
