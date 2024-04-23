
#include "resolve-shape-geometry.h"

#ifdef MSDFGEN_USE_SKIA

#include <skia/core/SkPath.h>
#include <skia/pathops/SkPathOps.h>
#include "../core/arithmetics.hpp"
#include "../core/Vector2.hpp"
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
            const EdgeSegment *edge = contour->edges.back();
            skPath.moveTo(pointToSkiaPoint(*edge->controlPoints()));
            for (std::vector<EdgeHolder>::const_iterator nextEdge = contour->edges.begin(); nextEdge != contour->edges.end(); edge = *nextEdge++) {
                const Point2 *p = edge->controlPoints();
                switch (edge->type()) {
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
                contour->addEdge(EdgeHolder(pointFromSkiaPoint(edgePoints[0]), pointFromSkiaPoint(edgePoints[1])));
                break;
            case SkPath::kQuad_Verb:
                contour->addEdge(EdgeHolder(pointFromSkiaPoint(edgePoints[0]), pointFromSkiaPoint(edgePoints[1]), pointFromSkiaPoint(edgePoints[2])));
                break;
            case SkPath::kCubic_Verb:
                contour->addEdge(EdgeHolder(pointFromSkiaPoint(edgePoints[0]), pointFromSkiaPoint(edgePoints[1]), pointFromSkiaPoint(edgePoints[2]), pointFromSkiaPoint(edgePoints[3])));
                break;
            case SkPath::kConic_Verb:
                {
                    SkPoint quadPoints[5];
                    SkPath::ConvertConicToQuads(edgePoints[0], edgePoints[1], edgePoints[2], pathIterator.conicWeight(), quadPoints, 1);
                    contour->addEdge(EdgeHolder(pointFromSkiaPoint(quadPoints[0]), pointFromSkiaPoint(quadPoints[1]), pointFromSkiaPoint(quadPoints[2])));
                    contour->addEdge(EdgeHolder(pointFromSkiaPoint(quadPoints[2]), pointFromSkiaPoint(quadPoints[3]), pointFromSkiaPoint(quadPoints[4])));
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

static void pruneCrossedQuadrilaterals(Shape &shape) {
    int n = 0;
    for (int i = 0; i < (int) shape.contours.size(); ++i) {
        Contour &contour = shape.contours[i];
        if (
            contour.edges.size() == 4 &&
            contour.edges[0]->type() == (int) LinearSegment::EDGE_TYPE &&
            contour.edges[1]->type() == (int) LinearSegment::EDGE_TYPE &&
            contour.edges[2]->type() == (int) LinearSegment::EDGE_TYPE &&
            contour.edges[3]->type() == (int) LinearSegment::EDGE_TYPE && (
                sign(crossProduct(contour.edges[0]->direction(1), contour.edges[1]->direction(0)))+
                sign(crossProduct(contour.edges[1]->direction(1), contour.edges[2]->direction(0)))+
                sign(crossProduct(contour.edges[2]->direction(1), contour.edges[3]->direction(0)))+
                sign(crossProduct(contour.edges[3]->direction(1), contour.edges[0]->direction(0)))
            ) == 0
        ) {
            contour.edges.clear();
        } else {
            if (i != n) {
                #ifdef MSDFGEN_USE_CPP11
                    shape.contours[n] = (Contour &&) contour;
                #else
                    shape.contours[n] = contour;
                #endif
            }
            ++n;
        }
    }
    shape.contours.resize(n);
}

bool resolveShapeGeometry(Shape &shape) {
    SkPath skPath;
    shape.normalize();
    shapeToSkiaPath(skPath, shape);
    if (!Simplify(skPath, &skPath))
        return false;
    // Skia's AsWinding doesn't seem to work for unknown reasons
    shapeFromSkiaPath(shape, skPath);
    // In some rare cases, Skia produces tiny residual crossed quadrilateral contours, which are not valid geometry, so they must be removed.
    pruneCrossedQuadrilaterals(shape);
    shape.orientContours();
    return true;
}

}

#endif
