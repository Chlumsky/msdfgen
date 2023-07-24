
#include "resolve-shape-geometry.h"

#ifdef MSDFGEN_USE_SKIA

#include <skia/core/SkPath.h>
#include <skia/pathops/SkPathOps.h>
#include "../core/Vector2.h"
#include "../core/edge-segments.h"
#include "../core/Contour.h"

namespace msdfgen {

SkPoint pointToSkiaPoint(Point2 p) {
    return SkPoint::Make((SkScalar) p.x, (SkScalar) p.y);
}

Point2 pointFromSkiaPoint(const SkPoint p) {
    return Point2((double) p.x(), (double) p.y());
}

void shapeToSkiaPath(SkPath &skPath, const Shape &shape) {
    for (std::vector<Contour>::const_iterator contour = shape.contours.begin(); contour != shape.contours.end(); ++contour) {
        if (!contour->edges.empty()) {
            skPath.moveTo(pointToSkiaPoint(contour->edges.front()->point(0)));
            for (std::vector<EdgeHolder>::const_iterator edge = contour->edges.begin(); edge != contour->edges.end(); ++edge) {
                const Point2 *p = (*edge)->controlPoints();
                switch ((*edge)->type()) {
                    case (int) LinearSegment::EDGE_TYPE:
                        skPath.lineTo(pointToSkiaPoint(p[1]));
                        break;
                    case (int) QuadraticSegment::EDGE_TYPE:
                        skPath.quadTo(pointToSkiaPoint(p[1]), pointToSkiaPoint(p[2]));
                        break;
                    case (int) CubicSegment::EDGE_TYPE:
                        skPath.cubicTo(pointToSkiaPoint(p[1]), pointToSkiaPoint(p[2]), pointToSkiaPoint(p[3]));
                        break;
                }
            }
        }
    }
}

void shapeFromSkiaPath(Shape &shape, const SkPath &skPath) {
    shape.contours.clear();
    Contour *contour = &shape.addContour();
    SkPath::Iter pathIterator(skPath, true);
    SkPoint edgePoints[4];
    for (SkPath::Verb op; (op = pathIterator.next(edgePoints)) != SkPath::kDone_Verb;) {
        switch (op) {
            case SkPath::kMove_Verb:
                if (!contour->edges.empty())
                    contour = &shape.addContour();
                break;
            case SkPath::kLine_Verb:
                contour->addEdge(new LinearSegment(pointFromSkiaPoint(edgePoints[0]), pointFromSkiaPoint(edgePoints[1])));
                break;
            case SkPath::kQuad_Verb:
                contour->addEdge(new QuadraticSegment(pointFromSkiaPoint(edgePoints[0]), pointFromSkiaPoint(edgePoints[1]), pointFromSkiaPoint(edgePoints[2])));
                break;
            case SkPath::kCubic_Verb:
                contour->addEdge(new CubicSegment(pointFromSkiaPoint(edgePoints[0]), pointFromSkiaPoint(edgePoints[1]), pointFromSkiaPoint(edgePoints[2]), pointFromSkiaPoint(edgePoints[3])));
                break;
            case SkPath::kConic_Verb:
                {
                    SkPoint quadPoints[5];
                    SkPath::ConvertConicToQuads(edgePoints[0], edgePoints[1], edgePoints[2], pathIterator.conicWeight(), quadPoints, 1);
                    contour->addEdge(new QuadraticSegment(pointFromSkiaPoint(quadPoints[0]), pointFromSkiaPoint(quadPoints[1]), pointFromSkiaPoint(quadPoints[2])));
                    contour->addEdge(new QuadraticSegment(pointFromSkiaPoint(quadPoints[2]), pointFromSkiaPoint(quadPoints[3]), pointFromSkiaPoint(quadPoints[4])));
                }
                break;
            case SkPath::kClose_Verb:
            case SkPath::kDone_Verb:
                break;
        }
    }
    if (contour->edges.empty())
        shape.contours.pop_back();
}

bool resolveShapeGeometry(Shape &shape) {
    SkPath skPath;
    shapeToSkiaPath(skPath, shape);
    if (!Simplify(skPath, &skPath))
        return false;
    // Skia's AsWinding doesn't seem to work for unknown reasons
    shapeFromSkiaPath(shape, skPath);
    shape.orientContours();
    return true;
}

}

#endif
