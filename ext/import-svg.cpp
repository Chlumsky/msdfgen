
#include "import-svg.h"

#include <cstdio>
#include <tinyxml2.h>

#ifdef _WIN32
    #pragma warning(disable:4996)
#endif

namespace msdfgen {

#define REQUIRE(cond) { if (!(cond)) return false; }

static bool readNodeType(char &output, const char *&pathDef) {
    int shift;
    char nodeType;
    if (sscanf(pathDef, " %c%n", &nodeType, &shift) == 1 && nodeType != '+' && nodeType != '-' && nodeType != '.' && nodeType != ',' && (nodeType < '0' || nodeType > '9')) {
        pathDef += shift;
        output = nodeType;
        return true;
    }
    return false;
}

static bool readCoord(Point2 &output, const char *&pathDef) {
    int shift;
    double x, y;
    if (sscanf(pathDef, "%lf%lf%n", &x, &y, &shift) == 2) {
        output.x = x;
        output.y = y;
        pathDef += shift;
        return true;
    }
    if (sscanf(pathDef, "%lf,%lf%n", &x, &y, &shift) == 2) {
        output.x = x;
        output.y = y;
        pathDef += shift;
        return true;
    }
    return false;
}

static bool readDouble(double &output, const char *&pathDef) {
    int shift;
    double v;
    if (sscanf(pathDef, "%lf%n", &v, &shift) == 1) {
        pathDef += shift;
        output = v;
        return true;
    }
    return false;
}

static bool readBool(bool &output, const char *&pathDef) {
    int shift;
    int v;
    if (sscanf(pathDef, "%d%n", &v, &shift) == 1) {
        pathDef += shift;
        output = static_cast<bool>(v);
        return true;
    }
    return false;
}

static void consumeOptionalComma(const char *&pathDef) {
    while (*pathDef == ' ')
        ++pathDef;
    if (*pathDef == ',')
        ++pathDef;
}

static double toDegrees(double rad) {
    return rad * 180.0 / M_PI;
}

static double toRad(double degrees) {
    return degrees * M_PI / 180.0;
}

static Point2 handleArc(const char *&pathDef, char nodeType, Point2 prevNode, Contour &contour) {
    Point2 arcEnd;
    double radiusX = 0;
    double radiusY = 0;
    double xAxisRotation = 0;
    bool largeArc = false;
    bool sweep = false;

    // arc syntax
    // A rx ry x-axis-rotation large-arc-flag sweep-flag x y
    // a rx ry x-axis-rotation large-arc-flag sweep-flag dx dy
    REQUIRE(readDouble(radiusX, pathDef));
    radiusX = std::abs(radiusX);
    consumeOptionalComma(pathDef);
    REQUIRE(readDouble(radiusY, pathDef));
    radiusY = std::abs(radiusY);
    consumeOptionalComma(pathDef);
    REQUIRE(readDouble(xAxisRotation, pathDef));
    consumeOptionalComma(pathDef);
    REQUIRE(readBool(largeArc, pathDef));
    consumeOptionalComma(pathDef);
    REQUIRE(readBool(sweep, pathDef));
    consumeOptionalComma(pathDef);
    REQUIRE(readCoord(arcEnd, pathDef));
    if (nodeType == 'a') {
        arcEnd += prevNode;
    }

    // Zero arc radius results in a straight line
    if (radiusX == 0 || radiusY == 0) {
        contour.addEdge(new LinearSegment(prevNode, arcEnd));
        return arcEnd;
    }

    if (prevNode.x == arcEnd.x && prevNode.y == arcEnd.y) {
        // End point matches previous node,
        // which means it has a length of zero and can be ignored
        return prevNode;
    }

    double xAxisRotationRad = toRad(std::fmod(xAxisRotation, 360.0));
    double cosRotation = std::cos(xAxisRotationRad);
    double sinRotation = std::sin(xAxisRotationRad);
    Point2 mid = (prevNode - arcEnd) / 2.0;

    // Compute transformed start point
    double x1 = (cosRotation * mid.x + sinRotation * mid.y);
    double y1 = (-sinRotation * mid.x + cosRotation * mid.y);
    double radXsq = std::pow(radiusX, 2);
    double radYsq = std::pow(radiusY, 2);
    double x1sq = std::pow(x1, 2);
    double y1sq = std::pow(y1, 2);

    // Scale the radius if it is not big enough
    double radiiCheck = x1sq / radXsq + y1sq / radYsq;
    if (radiiCheck > 1) {
        radiusX = std::sqrt(radiiCheck) * radiusX;
        radiusY = std::sqrt(radiiCheck) * radiusY;
        radXsq = std::pow(radiusX, 2);
        radYsq = std::pow(radiusY, 2);
    }

    // Compute centre point
    double sign = (largeArc == sweep) ? -1 : 1;
    double sq = ((radXsq * radYsq) - (radXsq * y1sq) - (radYsq * x1sq)) / ((radXsq * y1sq) + (radYsq * x1sq));
    sq = (sq < 0) ? 0 : sq;
    double coefficient = sign * std::sqrt(sq);
    double cx1 = coefficient * ((radiusX * y1) / radiusY);
    double cy1 = coefficient * -((radiusY * x1) / radiusX);
    Point2 sx = (prevNode + arcEnd) / 2.0;
    double cx = sx.x + (cosRotation * cx1 - sinRotation * cy1);
    double cy = sx.y + (sinRotation * cx1 + cosRotation * cy1);
    Point2 center(cx, cy);

    // Compute the angle start
    double ux = (x1 - cx1) / radiusX;
    double uy = (y1 - cy1) / radiusY;
    double n = std::sqrt((ux * ux) + (uy * uy));
    double p = ux;
    sign = (uy < 0) ? -1.0 : 1.0;
    double angleStart = toDegrees(sign * std::acos(p / n));

    // Compute the angle extent
    double vx = (-x1 - cx1) / radiusX;
    double vy = (-y1 - cy1) / radiusY;
    n = std::sqrt((ux * ux + uy * uy) * (vx * vx + vy * vy));
    p = ux * vx + uy * vy;
    sign = (ux * vy - uy * vx < 0) ? -1.0 : 1.0;
    double angleExtent = toDegrees(sign * std::acos(p / n));
    if (!sweep && angleExtent > 0) {
        angleExtent -= 360;
    } else if (sweep && angleExtent < 0) {
        angleExtent += 360;
    }

    // generate bezier curves for a unit circle that covers the given arc angles
    int segmentCount = static_cast<int>(std::ceil(std::abs(angleExtent) / 90.0));
    angleStart = toRad(std::fmod(angleStart, 360.0));
    angleExtent = toRad(std::fmod(angleExtent, 360.0));
    double angleIncrement = angleExtent / segmentCount;
    double controlLength = 4.0 / 3.0 * std::sin(angleIncrement / 2.0) / (1.0 + std::cos(angleIncrement / 2.0));

    Point2 startPoint = prevNode;
    Point2 bezEndpoint;
    Point2 controlPoint[2];
    for (int i = 0; i < segmentCount; i++) {
        double angle = angleStart + i * angleIncrement;
        double dx = std::cos(angle);
        double dy = std::sin(angle);
        controlPoint[0].x = (dx - controlLength * dy) * radiusX;
        controlPoint[0].y = (dy + controlLength * dx) * radiusY;
        controlPoint[0] = controlPoint[0].rotate(xAxisRotation) + center;
        angle += angleIncrement;
        dx = std::cos(angle);
        dy = std::sin(angle);
        controlPoint[1].x = (dx + controlLength * dy) * radiusX;
        controlPoint[1].y = (dy - controlLength * dx) * radiusY;
        controlPoint[1] = controlPoint[1].rotate(xAxisRotation) + center;
        bezEndpoint.x = dx * radiusX;
        bezEndpoint.y = dy * radiusY;
        bezEndpoint = bezEndpoint.rotate(xAxisRotation);
        bezEndpoint = bezEndpoint + center;
        if (i == segmentCount - 1) {
            // to prevent rounding errors
            bezEndpoint = arcEnd;
        }

        contour.addEdge(new CubicSegment(startPoint, controlPoint[0], controlPoint[1], bezEndpoint));
        startPoint = bezEndpoint;
    }

    return arcEnd;
}

static bool buildFromPath(Shape &shape, const char *pathDef) {
    char nodeType;
    Point2 prevNode(0, 0);
    while (readNodeType(nodeType, pathDef)) {
        Contour &contour = shape.addContour();
        bool contourStart = true;

        Point2 startPoint;
        Point2 controlPoint[2];
        Point2 node;

        while (true) {
            switch (nodeType) {
                case 'M': case 'm':
                    REQUIRE(contourStart);
                    REQUIRE(readCoord(node, pathDef));
                    if (nodeType == 'm')
                        node += prevNode;
                    startPoint = node;
                    --nodeType; // to 'L' or 'l'
                    break;
                case 'Z': case 'z':
                    if (prevNode != startPoint)
                        contour.addEdge(new LinearSegment(prevNode, startPoint));
                    prevNode = startPoint;
                    goto NEXT_CONTOUR;
                case 'L': case 'l':
                    REQUIRE(readCoord(node, pathDef));
                    if (nodeType == 'l')
                        node += prevNode;
                    contour.addEdge(new LinearSegment(prevNode, node));
                    break;
                case 'H': case 'h':
                    REQUIRE(readDouble(node.x, pathDef));
                    if (nodeType == 'h')
                        node.x += prevNode.x;
                    contour.addEdge(new LinearSegment(prevNode, node));
                    break;
                case 'V': case 'v':
                    REQUIRE(readDouble(node.y, pathDef));
                    if (nodeType == 'v')
                        node.y += prevNode.y;
                    contour.addEdge(new LinearSegment(prevNode, node));
                    break;
                case 'Q': case 'q':
                    REQUIRE(readCoord(controlPoint[0], pathDef));
                    consumeOptionalComma(pathDef);
                    REQUIRE(readCoord(node, pathDef));
                    if (nodeType == 'q') {
                        controlPoint[0] += prevNode;
                        node += prevNode;
                    }
                    contour.addEdge(new QuadraticSegment(prevNode, controlPoint[0], node));
                    break;
                // TODO T, t
                case 'C': case 'c':
                    REQUIRE(readCoord(controlPoint[0], pathDef));
                    consumeOptionalComma(pathDef);
                    REQUIRE(readCoord(controlPoint[1], pathDef));
                    consumeOptionalComma(pathDef);
                    REQUIRE(readCoord(node, pathDef));
                    if (nodeType == 'c') {
                        controlPoint[0] += prevNode;
                        controlPoint[1] += prevNode;
                        node += prevNode;
                    }
                    contour.addEdge(new CubicSegment(prevNode, controlPoint[0], controlPoint[1], node));
                    break;
                case 'S': case 's':
                    controlPoint[0] = node+node-controlPoint[1];
                    REQUIRE(readCoord(controlPoint[1], pathDef));
                    consumeOptionalComma(pathDef);
                    REQUIRE(readCoord(node, pathDef));
                    if (nodeType == 's') {
                        controlPoint[1] += prevNode;
                        node += prevNode;
                    }
                    contour.addEdge(new CubicSegment(prevNode, controlPoint[0], controlPoint[1], node));
                    break;
                case 'A': case 'a':
                    node = handleArc(pathDef, nodeType, prevNode, contour);
                    break;
                default:
                    REQUIRE(false);
            }
            contourStart &= nodeType == 'M' || nodeType == 'm';
            prevNode = node;
            readNodeType(nodeType, pathDef);
        }
    NEXT_CONTOUR:;
    }
    return true;
}

bool loadSvgShape(Shape &output, const char *filename, int pathIndex, Vector2 *dimensions) {
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(filename))
        return false;
    tinyxml2::XMLElement *root = doc.FirstChildElement("svg");
    if (!root)
        return false;

    tinyxml2::XMLElement *path = NULL;
    if( pathIndex > 0 ) {
        path = root->FirstChildElement("path");
        if (!path) {
            tinyxml2::XMLElement *g = root->FirstChildElement("g");
            if (g)
                path = g->FirstChildElement("path");
        }
        while (path && --pathIndex > 0)
            path = path->NextSiblingElement("path");
    }
    else {
        // A pathIndex of 0 means "default", which is the same as specifying a -1 (i.e. last).

        path = root->LastChildElement("path");
        if (!path) {
            tinyxml2::XMLElement *g = root->LastChildElement("g");
            if (g)
                path = g->LastChildElement("path");
        }
        while (path && ++pathIndex < 0) {
            path = path->PreviousSiblingElement("path");
        }
    }
    if (!path)
        return false;
    const char *pd = path->Attribute("d");
    if (!pd)
        return false;

    output.contours.clear();
    output.inverseYAxis = true;
    if (dimensions) {
        dimensions->x = root->DoubleAttribute("width");
        dimensions->y = root->DoubleAttribute("height");
    }
    return buildFromPath(output, pd);
}

}
