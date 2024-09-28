
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "import-svg.h"

#ifndef MSDFGEN_DISABLE_SVG

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <stack>

#ifdef MSDFGEN_USE_TINYXML2
#include <tinyxml2.h>
#endif
#ifdef MSDFGEN_USE_DROPXML
#include <dropXML.hpp>
#endif

#ifdef MSDFGEN_USE_SKIA
#include <skia/core/SkPath.h>
#include <skia/utils/SkParsePath.h>
#include <skia/pathops/SkPathOps.h>
#endif

#include "../core/arithmetics.hpp"

#define ARC_SEGMENTS_PER_PI 2
#define ENDPOINT_SNAP_RANGE_PROPORTION (1/16384.)

namespace msdfgen {

#if defined(_DEBUG) || !NDEBUG
#define REQUIRE(cond) { if (!(cond)) { fprintf(stderr, "SVG Parse Error (%s:%d): " #cond "\n", __FILE__, __LINE__); return false; } }
#else
#define REQUIRE(cond) { if (!(cond)) return false; }
#endif

MSDFGEN_EXT_PUBLIC const int SVG_IMPORT_FAILURE = 0x00;
MSDFGEN_EXT_PUBLIC const int SVG_IMPORT_SUCCESS_FLAG = 0x01;
MSDFGEN_EXT_PUBLIC const int SVG_IMPORT_PARTIAL_FAILURE_FLAG = 0x02;
MSDFGEN_EXT_PUBLIC const int SVG_IMPORT_INCOMPLETE_FLAG = 0x04;
MSDFGEN_EXT_PUBLIC const int SVG_IMPORT_UNSUPPORTED_FEATURE_FLAG = 0x08;
MSDFGEN_EXT_PUBLIC const int SVG_IMPORT_TRANSFORMATION_IGNORED_FLAG = 0x10;

#define FLAGS_FINAL(flags) (((flags)&(SVG_IMPORT_SUCCESS_FLAG|SVG_IMPORT_INCOMPLETE_FLAG|SVG_IMPORT_UNSUPPORTED_FEATURE_FLAG)) == (SVG_IMPORT_SUCCESS_FLAG|SVG_IMPORT_INCOMPLETE_FLAG|SVG_IMPORT_UNSUPPORTED_FEATURE_FLAG))

static void skipExtraChars(const char *&pathDef) {
    while (*pathDef == ',' || *pathDef == ' ' || *pathDef == '\t' || *pathDef == '\r' || *pathDef == '\n')
        ++pathDef;
}

static bool readNodeType(char &output, const char *&pathDef) {
    skipExtraChars(pathDef);
    char nodeType = *pathDef;
    if (nodeType && nodeType != '+' && nodeType != '-' && nodeType != '.' && nodeType != ',' && (nodeType < '0' || nodeType > '9')) {
        ++pathDef;
        output = nodeType;
        return true;
    }
    return false;
}

static bool readDouble(double &output, const char *&pathDef) {
    skipExtraChars(pathDef);
    char *end = NULL;
    output = strtod(pathDef, &end);
    if (end > pathDef) {
        pathDef = end;
        return true;
    }
    return false;
}

static bool readCoord(Point2 &output, const char *&pathDef) {
    return readDouble(output.x, pathDef) && readDouble(output.y, pathDef);
}

static bool readBool(bool &output, const char *&pathDef) {
    skipExtraChars(pathDef);
    char *end = NULL;
    long v = strtol(pathDef, &end, 10);
    if (end > pathDef) {
        pathDef = end;
        output = v != 0;
        return true;
    }
    return false;
}

static double arcAngle(Vector2 u, Vector2 v) {
    return nonZeroSign(crossProduct(u, v))*acos(clamp(dotProduct(u, v)/(u.length()*v.length()), -1., +1.));
}

static Vector2 rotateVector(Vector2 v, Vector2 direction) {
    return Vector2(direction.x*v.x-direction.y*v.y, direction.y*v.x+direction.x*v.y);
}

static void addArcApproximate(Contour &contour, Point2 startPoint, Point2 endPoint, Vector2 radius, double rotation, bool largeArc, bool sweep) {
    if (endPoint == startPoint)
        return;
    if (radius.x == 0 || radius.y == 0)
        return contour.addEdge(EdgeHolder(startPoint, endPoint));

    radius.x = fabs(radius.x);
    radius.y = fabs(radius.y);
    Vector2 axis(cos(rotation), sin(rotation));

    Vector2 rm = rotateVector(.5*(startPoint-endPoint), Vector2(axis.x, -axis.y));
    Vector2 rm2 = rm*rm;
    Vector2 radius2 = radius*radius;
    double radiusGap = rm2.x/radius2.x+rm2.y/radius2.y;
    if (radiusGap > 1) {
        radius *= sqrt(radiusGap);
        radius2 = radius*radius;
    }
    double dq = (radius2.x*rm2.y+radius2.y*rm2.x);
    double pq = radius2.x*radius2.y/dq-1;
    double q = (largeArc == sweep ? -1 : +1)*sqrt(max(pq, 0.));
    Vector2 rc(q*radius.x*rm.y/radius.y, -q*radius.y*rm.x/radius.x);
    Point2 center = .5*(startPoint+endPoint)+rotateVector(rc, axis);

    double angleStart = arcAngle(Vector2(1, 0), (rm-rc)/radius);
    double angleExtent = arcAngle((rm-rc)/radius, (-rm-rc)/radius);
    if (!sweep && angleExtent > 0)
        angleExtent -= 2*M_PI;
    else if (sweep && angleExtent < 0)
        angleExtent += 2*M_PI;

    int segments = (int) ceil(ARC_SEGMENTS_PER_PI/M_PI*fabs(angleExtent));
    double angleIncrement = angleExtent/segments;
    double cl = 4/3.*sin(.5*angleIncrement)/(1+cos(.5*angleIncrement));

    Point2 prevNode = startPoint;
    double angle = angleStart;
    for (int i = 0; i < segments; ++i) {
        Point2 controlPoint[2];
        Vector2 d(cos(angle), sin(angle));
        controlPoint[0] = center+rotateVector(Vector2(d.x-cl*d.y, d.y+cl*d.x)*radius, axis);
        angle += angleIncrement;
        d.set(cos(angle), sin(angle));
        controlPoint[1] = center+rotateVector(Vector2(d.x+cl*d.y, d.y-cl*d.x)*radius, axis);
        Point2 node = i == segments-1 ? endPoint : center+rotateVector(d*radius, axis);
        contour.addEdge(EdgeHolder(prevNode, controlPoint[0], controlPoint[1], node));
        prevNode = node;
    }
}

bool buildShapeFromSvgPath(Shape &shape, const char *pathDef, double endpointSnapRange) {
    char nodeType = '\0';
    char prevNodeType = '\0';
    Point2 prevNode(0, 0);
    bool nodeTypePreread = false;
    while (nodeTypePreread || readNodeType(nodeType, pathDef)) {
        nodeTypePreread = false;
        Contour &contour = shape.addContour();
        bool contourStart = true;

        Point2 startPoint;
        Point2 controlPoint[2];
        Point2 node;

        while (*pathDef) {
            switch (nodeType) {
                case 'M': case 'm':
                    if (!contourStart) {
                        nodeTypePreread = true;
                        goto NEXT_CONTOUR;
                    }
                    REQUIRE(readCoord(node, pathDef));
                    if (nodeType == 'm')
                        node += prevNode;
                    startPoint = node;
                    --nodeType; // to 'L' or 'l'
                    break;
                case 'Z': case 'z':
                    REQUIRE(!contourStart);
                    goto NEXT_CONTOUR;
                case 'L': case 'l':
                    REQUIRE(readCoord(node, pathDef));
                    if (nodeType == 'l')
                        node += prevNode;
                    contour.addEdge(EdgeHolder(prevNode, node));
                    break;
                case 'H': case 'h':
                    REQUIRE(readDouble(node.x, pathDef));
                    if (nodeType == 'h')
                        node.x += prevNode.x;
                    contour.addEdge(EdgeHolder(prevNode, node));
                    break;
                case 'V': case 'v':
                    REQUIRE(readDouble(node.y, pathDef));
                    if (nodeType == 'v')
                        node.y += prevNode.y;
                    contour.addEdge(EdgeHolder(prevNode, node));
                    break;
                case 'Q': case 'q':
                    REQUIRE(readCoord(controlPoint[0], pathDef));
                    REQUIRE(readCoord(node, pathDef));
                    if (nodeType == 'q') {
                        controlPoint[0] += prevNode;
                        node += prevNode;
                    }
                    contour.addEdge(EdgeHolder(prevNode, controlPoint[0], node));
                    break;
                case 'T': case 't':
                    if (prevNodeType == 'Q' || prevNodeType == 'q' || prevNodeType == 'T' || prevNodeType == 't')
                        controlPoint[0] = node+node-controlPoint[0];
                    else
                        controlPoint[0] = node;
                    REQUIRE(readCoord(node, pathDef));
                    if (nodeType == 't')
                        node += prevNode;
                    contour.addEdge(EdgeHolder(prevNode, controlPoint[0], node));
                    break;
                case 'C': case 'c':
                    REQUIRE(readCoord(controlPoint[0], pathDef));
                    REQUIRE(readCoord(controlPoint[1], pathDef));
                    REQUIRE(readCoord(node, pathDef));
                    if (nodeType == 'c') {
                        controlPoint[0] += prevNode;
                        controlPoint[1] += prevNode;
                        node += prevNode;
                    }
                    contour.addEdge(EdgeHolder(prevNode, controlPoint[0], controlPoint[1], node));
                    break;
                case 'S': case 's':
                    if (prevNodeType == 'C' || prevNodeType == 'c' || prevNodeType == 'S' || prevNodeType == 's')
                        controlPoint[0] = node+node-controlPoint[1];
                    else
                        controlPoint[0] = node;
                    REQUIRE(readCoord(controlPoint[1], pathDef));
                    REQUIRE(readCoord(node, pathDef));
                    if (nodeType == 's') {
                        controlPoint[1] += prevNode;
                        node += prevNode;
                    }
                    contour.addEdge(EdgeHolder(prevNode, controlPoint[0], controlPoint[1], node));
                    break;
                case 'A': case 'a':
                    {
                        Vector2 radius;
                        double angle;
                        bool largeArg;
                        bool sweep;
                        REQUIRE(readCoord(radius, pathDef));
                        REQUIRE(readDouble(angle, pathDef));
                        REQUIRE(readBool(largeArg, pathDef));
                        REQUIRE(readBool(sweep, pathDef));
                        REQUIRE(readCoord(node, pathDef));
                        if (nodeType == 'a')
                            node += prevNode;
                        angle *= M_PI/180.0;
                        addArcApproximate(contour, prevNode, node, radius, angle, largeArg, sweep);
                    }
                    break;
                default:
                    REQUIRE(!"Unknown node type");
            }
            contourStart &= nodeType == 'M' || nodeType == 'm';
            prevNode = node;
            prevNodeType = nodeType;
            readNodeType(nodeType, pathDef);
        }
    NEXT_CONTOUR:
        // Fix contour if it isn't properly closed
        if (!contour.edges.empty() && prevNode != startPoint) {
            if ((contour.edges.back()->point(1)-contour.edges[0]->point(0)).length() < endpointSnapRange)
                contour.edges.back()->moveEndPoint(contour.edges[0]->point(0));
            else
                contour.addEdge(EdgeHolder(prevNode, startPoint));
        }
        prevNode = startPoint;
        prevNodeType = '\0';
    }
    return true;
}

#ifdef MSDFGEN_USE_TINYXML2

static void findPathByForwardIndex(tinyxml2::XMLElement *&path, int &flags, int &skips, tinyxml2::XMLElement *parent, bool hasTransformation) {
    for (tinyxml2::XMLElement *cur = parent->FirstChildElement(); cur && !FLAGS_FINAL(flags); cur = cur->NextSiblingElement()) {
        if (!strcmp(cur->Name(), "path")) {
            if (!skips--) {
                path = cur;
                flags |= SVG_IMPORT_SUCCESS_FLAG;
                if (hasTransformation || cur->Attribute("transform"))
                    flags |= SVG_IMPORT_TRANSFORMATION_IGNORED_FLAG;
            } else if (flags&SVG_IMPORT_SUCCESS_FLAG)
                flags |= SVG_IMPORT_INCOMPLETE_FLAG;
        } else if (!strcmp(cur->Name(), "g"))
            findPathByForwardIndex(path, flags, skips, cur, hasTransformation || cur->Attribute("transform"));
        else if (!strcmp(cur->Name(), "rect") || !strcmp(cur->Name(), "circle") || !strcmp(cur->Name(), "ellipse") || !strcmp(cur->Name(), "polygon"))
            flags |= SVG_IMPORT_INCOMPLETE_FLAG;
        else if (!strcmp(cur->Name(), "mask") || !strcmp(cur->Name(), "use"))
            flags |= SVG_IMPORT_UNSUPPORTED_FEATURE_FLAG;
    }
}

static void findPathByBackwardIndex(tinyxml2::XMLElement *&path, int &flags, int &skips, tinyxml2::XMLElement *parent, bool hasTransformation) {
    for (tinyxml2::XMLElement *cur = parent->LastChildElement(); cur && !FLAGS_FINAL(flags); cur = cur->PreviousSiblingElement()) {
        if (!strcmp(cur->Name(), "path")) {
            if (!skips--) {
                path = cur;
                flags |= SVG_IMPORT_SUCCESS_FLAG;
                if (hasTransformation || cur->Attribute("transform"))
                    flags |= SVG_IMPORT_TRANSFORMATION_IGNORED_FLAG;
            } else if (flags&SVG_IMPORT_SUCCESS_FLAG)
                flags |= SVG_IMPORT_INCOMPLETE_FLAG;
        } else if (!strcmp(cur->Name(), "g"))
            findPathByBackwardIndex(path, flags, skips, cur, hasTransformation || cur->Attribute("transform"));
        else if (!strcmp(cur->Name(), "rect") || !strcmp(cur->Name(), "circle") || !strcmp(cur->Name(), "ellipse") || !strcmp(cur->Name(), "polygon"))
            flags |= SVG_IMPORT_INCOMPLETE_FLAG;
        else if (!strcmp(cur->Name(), "mask") || !strcmp(cur->Name(), "use"))
            flags |= SVG_IMPORT_UNSUPPORTED_FEATURE_FLAG;
    }
}

bool loadSvgShape(Shape &output, const char *filename, int pathIndex, Vector2 *dimensions) {
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(filename))
        return false;
    tinyxml2::XMLElement *root = doc.FirstChildElement("svg");
    if (!root)
        return false;

    tinyxml2::XMLElement *path = NULL;
    int flags = 0;
    int skippedPaths = abs(pathIndex)-(pathIndex != 0);
    if (pathIndex > 0)
        findPathByForwardIndex(path, flags, skippedPaths, root, false);
    else
        findPathByBackwardIndex(path, flags, skippedPaths, root, false);
    if (!path)
        return false;
    const char *pd = path->Attribute("d");
    if (!pd)
        return false;

    Vector2 dims(root->DoubleAttribute("width"), root->DoubleAttribute("height"));
    if (const char *viewBox = root->Attribute("viewBox")) {
        double left = 0, top = 0;
        readDouble(left, viewBox) && readDouble(top, viewBox) && readDouble(dims.x, viewBox) && readDouble(dims.y, viewBox);
    }
    if (dimensions)
        *dimensions = dims;
    output.contours.clear();
    output.inverseYAxis = true;
    return buildShapeFromSvgPath(output, pd, ENDPOINT_SNAP_RANGE_PROPORTION*dims.length());
}

#endif

#ifdef MSDFGEN_USE_DROPXML

struct StrRange {
    const char *start, *end;
    inline StrRange() : start(), end() { }
    inline StrRange(const char *start, const char *end) : start(start), end(end) { }
    inline std::string str() const { return std::string(start, end); }
};

static bool matchName(const char *start, const char *end, const char *value) {
    for (const char *c = start; c < end; ++c, ++value) {
        if (*c != *value)
            return false;
    }
    return !*value;
}

static std::string xmlDecode(const char *start, const char *end) {
    if (!dropXML::decode(start, end, nullptr, nullptr)) {
        std::string buffer(end-start+1, '\0');
        if (!dropXML::decode(start, end, &buffer[0], &buffer[buffer.size()-1]))
            return std::string();
        if (start == buffer.data()) {
            buffer.resize(end-start, '\0');
            return (std::string &&) buffer;
        }
    }
    return std::string(start, end);
}

static double xmlGetDouble(const char *start, const char *end) {
    double x = 0;
    std::string decodedStr(xmlDecode(start, end));
    const char *strPtr = decodedStr.c_str();
    readDouble(x, strPtr);
    return x;
}

#define SVG_NAME_IS(x) matchName(nameStart, nameEnd, x)
#define SVG_DEC_VAL() xmlDecode(valueStart, valueEnd)
#define SVG_DOUBLEVAL() xmlGetDouble(valueStart, valueEnd)

static bool readFile(std::vector<char> &output, const char *filename) {
    if (FILE *f = fopen(filename, "rb")) {
        struct FileGuard {
            FILE *f;
            ~FileGuard() {
                fclose(f);
            }
        } fileGuard = { f };
        if (fseek(f, 0, SEEK_END))
            return false;
        long size = ftell(f);
        if (size < 0)
            return false;
        output.resize(size);
        if (!size)
            return true;
        if (fseek(f, 0, SEEK_SET))
            return false;
        return fread(&output[0], 1, size, f) == (size_t) size;
    }
    return false;
}

class BaseSvgConsumer {
public:
    inline bool processingInstruction(const char *, const char *) { return true; }
    inline bool doctype(const char *, const char *) { return true; }
    inline bool text(const char *, const char *) { return true; }
    inline bool cdata(const char *, const char *) { return true; }
};

class SvgPathAggregator : public BaseSvgConsumer {
    enum {
        IGNORED,
        SVG,
        G,
        PATH
    } curElement;
    int ignoredDepth;

public:
    int flags;
    Vector2 dimensions;
    StrRange viewBox;
    std::vector<StrRange> pathDefs;

    inline SvgPathAggregator() : curElement(IGNORED), ignoredDepth(0), flags(0) { }

    inline bool enterElement(const char *nameStart, const char *nameEnd) {
        curElement = IGNORED;
        if (ignoredDepth)
            ++ignoredDepth;
        else if (SVG_NAME_IS("svg"))
            curElement = SVG;
        else if (SVG_NAME_IS("g"))
            curElement = G;
        else if (SVG_NAME_IS("path"))
            curElement = PATH;
        else {
            if (SVG_NAME_IS("rect") || SVG_NAME_IS("circle") || SVG_NAME_IS("ellipse") || SVG_NAME_IS("polygon"))
                flags |= SVG_IMPORT_INCOMPLETE_FLAG;
            else if (SVG_NAME_IS("mask") || SVG_NAME_IS("use"))
                flags |= SVG_IMPORT_UNSUPPORTED_FEATURE_FLAG;
            ++ignoredDepth;
        }
        return true;
    }

    inline bool leaveElement(const char *, const char *) {
        if (ignoredDepth)
            --ignoredDepth;
        return true;
    }

    inline bool elementAttribute(const char *nameStart, const char *nameEnd, const char *valueStart, const char *valueEnd) {
        switch (curElement) {
            case IGNORED:
                break;
            case SVG:
                if (SVG_NAME_IS("width"))
                    dimensions.x = xmlGetDouble(valueStart, valueEnd);
                else if (SVG_NAME_IS("height"))
                    dimensions.y = xmlGetDouble(valueStart, valueEnd);
                else if (SVG_NAME_IS("viewBox"))
                    viewBox = StrRange(valueStart, valueEnd);
                break;
            case PATH:
                if (SVG_NAME_IS("d"))
                    pathDefs.push_back(StrRange(valueStart, valueEnd));
                // fallthrough
            case G:
                if (SVG_NAME_IS("transform"))
                    flags |= SVG_IMPORT_TRANSFORMATION_IGNORED_FLAG;
                break;
        }
        return true;
    }

    inline bool finishAttributes() { return true; }
    inline bool finish() { return !ignoredDepth; }

};

bool loadSvgShape(Shape &output, const char *filename, int pathIndex, Vector2 *dimensions) {
    std::vector<char> svgData;
    if (!(readFile(svgData, filename) && !svgData.empty()))
        return false;

    SvgPathAggregator pathAggregator;
    if (!dropXML::parse(pathAggregator, &svgData[0], &svgData[0]+svgData.size()))
        return false;

    if (pathIndex <= 0) {
        if (pathIndex == 0)
            pathIndex = -1;
        pathIndex = pathAggregator.pathDefs.size()+pathIndex;
    } else
        --pathIndex;
    if (!(pathIndex > 0 && pathIndex < (int) pathAggregator.pathDefs.size()))
        return false;

    Vector2 dims(pathAggregator.dimensions);
    if (pathAggregator.viewBox.start < pathAggregator.viewBox.end) {
        std::string viewBoxStr = xmlDecode(pathAggregator.viewBox.start, pathAggregator.viewBox.end);
        const char *viewBoxPtr = viewBoxStr.c_str();
        double left = 0, top = 0;
        readDouble(left, viewBoxPtr) && readDouble(top, viewBoxPtr) && readDouble(dims.x, viewBoxPtr) && readDouble(dims.y, viewBoxPtr);
    }
    if (dimensions)
        *dimensions = dims;
    output.contours.clear();
    output.inverseYAxis = true;
    return buildShapeFromSvgPath(output, xmlDecode(pathAggregator.pathDefs[pathIndex].start, pathAggregator.pathDefs[pathIndex].end).c_str(), ENDPOINT_SNAP_RANGE_PROPORTION*dims.length());
}

#endif

#ifndef MSDFGEN_USE_SKIA

#ifdef MSDFGEN_USE_TINYXML2
int loadSvgShape(Shape &output, Shape::Bounds &viewBox, const char *filename) {
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(filename))
        return SVG_IMPORT_FAILURE;
    tinyxml2::XMLElement *root = doc.FirstChildElement("svg");
    if (!root)
        return SVG_IMPORT_FAILURE;

    tinyxml2::XMLElement *path = NULL;
    int flags = 0;
    int skippedPaths = 0;
    findPathByBackwardIndex(path, flags, skippedPaths, root, false);
    if (!(path && (flags&SVG_IMPORT_SUCCESS_FLAG)))
        return SVG_IMPORT_FAILURE;
    const char *pd = path->Attribute("d");
    if (!pd)
        return SVG_IMPORT_FAILURE;

    viewBox.l = 0, viewBox.b = 0;
    Vector2 dims(root->DoubleAttribute("width"), root->DoubleAttribute("height"));
    if (const char *viewBoxStr = root->Attribute("viewBox"))
        readDouble(viewBox.l, viewBoxStr) && readDouble(viewBox.b, viewBoxStr) && readDouble(dims.x, viewBoxStr) && readDouble(dims.y, viewBoxStr);
    viewBox.r = viewBox.l+dims.x;
    viewBox.t = viewBox.b+dims.y;
    output.contours.clear();
    output.inverseYAxis = true;
    if (!buildShapeFromSvgPath(output, pd, ENDPOINT_SNAP_RANGE_PROPORTION*dims.length()))
        return SVG_IMPORT_FAILURE;
    return flags;
}
#endif

#ifdef MSDFGEN_USE_DROPXML
int loadSvgShape(Shape &output, Shape::Bounds &viewBox, const char *filename) {
    std::vector<char> svgData;
    if (!(readFile(svgData, filename) && !svgData.empty()))
        return SVG_IMPORT_FAILURE;

    SvgPathAggregator pathAggregator;
    if (!dropXML::parse(pathAggregator, &svgData[0], &svgData[0]+svgData.size()) || pathAggregator.pathDefs.empty())
        return SVG_IMPORT_FAILURE;

    viewBox.l = 0, viewBox.b = 0;
    Vector2 dims(pathAggregator.dimensions);
    if (pathAggregator.viewBox.start < pathAggregator.viewBox.end) {
        std::string viewBoxStr = xmlDecode(pathAggregator.viewBox.start, pathAggregator.viewBox.end);
        const char *viewBoxPtr = viewBoxStr.c_str();
        readDouble(viewBox.l, viewBoxPtr) && readDouble(viewBox.b, viewBoxPtr) && readDouble(dims.x, viewBoxPtr) && readDouble(dims.y, viewBoxPtr);
    }
    viewBox.r = viewBox.l+dims.x;
    viewBox.t = viewBox.b+dims.y;
    output.contours.clear();
    output.inverseYAxis = true;
    if (!buildShapeFromSvgPath(output, xmlDecode(pathAggregator.pathDefs.back().start, pathAggregator.pathDefs.back().end).c_str(), ENDPOINT_SNAP_RANGE_PROPORTION*dims.length()))
        return SVG_IMPORT_FAILURE;
    return SVG_IMPORT_SUCCESS_FLAG|pathAggregator.flags;
}
#endif

#else

void shapeFromSkiaPath(Shape &shape, const SkPath &skPath); // defined in resolve-shape-geometry.cpp

static bool readTransformationOp(SkScalar dst[6], int &count, const char *&str, const char *name) {
    int nameLen = int(strlen(name));
    if (!memcmp(str, name, nameLen)) {
        const char *curStr = str+nameLen;
        skipExtraChars(curStr);
        if (*curStr == '(') {
            skipExtraChars(++curStr);
            count = 0;
            while (*curStr && *curStr != ')') {
                double x;
                if (!(count < 6 && readDouble(x, curStr)))
                    return false;
                dst[count++] = SkScalar(x);
                skipExtraChars(curStr);
            }
            if (*curStr == ')') {
                str = curStr+1;
                return true;
            }
        }
    }
    return false;
}

static SkMatrix parseTransformation(int &flags, const char *str) {
    SkMatrix transformation;
    skipExtraChars(str);
    while (*str) {
        SkScalar values[6];
        int count;
        SkMatrix partial;
        if (readTransformationOp(values, count, str, "matrix") && count == 6) {
            partial.setAll(values[0], values[2], values[4], values[1], values[3], values[5], SkScalar(0), SkScalar(0), SkScalar(1));
        } else if (readTransformationOp(values, count, str, "translate") && (count == 1 || count == 2)) {
            if (count == 1)
                values[1] = SkScalar(0);
            partial.setTranslate(values[0], values[1]);
        } else if (readTransformationOp(values, count, str, "scale") && (count == 1 || count == 2)) {
            if (count == 1)
                values[1] = values[0];
            partial.setScale(values[0], values[1]);
        } else if (readTransformationOp(values, count, str, "rotate") && (count == 1 || count == 3)) {
            if (count == 3)
                partial.setRotate(values[0], values[1], values[2]);
            else
                partial.setRotate(values[0]);
        } else if (readTransformationOp(values, count, str, "skewX") && count == 1) {
            partial.setSkewX(SkScalar(tan(M_PI/180*values[0])));
        } else if (readTransformationOp(values, count, str, "skewY") && count == 1) {
            partial.setSkewY(SkScalar(tan(M_PI/180*values[0])));
        } else {
            flags |= SVG_IMPORT_PARTIAL_FAILURE_FLAG;
            break;
        }
        transformation = transformation*partial;
        skipExtraChars(str);
    }
    return transformation;
}

static SkMatrix combineTransformation(int &flags, const SkMatrix &parentTransformation, const char *transformationString, const char *transformationOriginString) {
    if (transformationString && *transformationString) {
        SkMatrix transformation = parseTransformation(flags, transformationString);
        if (transformationOriginString && *transformationOriginString) {
            Point2 origin;
            if (readCoord(origin, transformationOriginString))
                transformation = SkMatrix::Translate(SkScalar(origin.x), SkScalar(origin.y))*transformation*SkMatrix::Translate(SkScalar(-origin.x), SkScalar(-origin.y));
            else
                flags |= SVG_IMPORT_PARTIAL_FAILURE_FLAG;
        }
        return parentTransformation*transformation;
    }
    return parentTransformation;
}

#ifdef MSDFGEN_USE_TINYXML2

static void gatherPaths(SkPath &fullPath, int &flags, tinyxml2::XMLElement *parent, const SkMatrix &transformation) {
    for (tinyxml2::XMLElement *cur = parent->FirstChildElement(); cur && !FLAGS_FINAL(flags); cur = cur->NextSiblingElement()) {
        if (!strcmp(cur->Name(), "g"))
            gatherPaths(fullPath, flags, cur, combineTransformation(flags, transformation, cur->Attribute("transform"), cur->Attribute("transform-origin")));
        else if (!strcmp(cur->Name(), "mask") || !strcmp(cur->Name(), "use"))
            flags |= SVG_IMPORT_UNSUPPORTED_FEATURE_FLAG;
        else {
            SkPath curPath;
            if (!strcmp(cur->Name(), "path")) {
                const char *pd = cur->Attribute("d");
                if (!(pd && SkParsePath::FromSVGString(pd, &curPath))) {
                    flags |= SVG_IMPORT_PARTIAL_FAILURE_FLAG;
                    continue;
                }
            } else if (!strcmp(cur->Name(), "rect")) {
                SkScalar x = SkScalar(cur->DoubleAttribute("x")), y = SkScalar(cur->DoubleAttribute("y"));
                SkScalar width = SkScalar(cur->DoubleAttribute("width")), height = SkScalar(cur->DoubleAttribute("height"));
                SkScalar rx = SkScalar(cur->DoubleAttribute("rx")), ry = SkScalar(cur->DoubleAttribute("ry"));
                if (!(width && height))
                    continue;
                SkRect rect = SkRect::MakeLTRB(x, y, x+width, y+height);
                if (rx || ry) {
                    SkScalar radii[] = { rx, ry, rx, ry, rx, ry, rx, ry };
                    curPath.addRoundRect(rect, radii);
                } else
                    curPath.addRect(rect);
            } else if (!strcmp(cur->Name(), "circle")) {
                SkScalar cx = SkScalar(cur->DoubleAttribute("cx")), cy = SkScalar(cur->DoubleAttribute("cy"));
                SkScalar r = SkScalar(cur->DoubleAttribute("r"));
                if (!r)
                    continue;
                curPath.addCircle(cx, cy, r);
            } else if (!strcmp(cur->Name(), "ellipse")) {
                SkScalar cx = SkScalar(cur->DoubleAttribute("cx")), cy = SkScalar(cur->DoubleAttribute("cy"));
                SkScalar rx = SkScalar(cur->DoubleAttribute("rx")), ry = SkScalar(cur->DoubleAttribute("ry"));
                if (!(rx && ry))
                    continue;
                curPath.addOval(SkRect::MakeLTRB(cx-rx, cy-ry, cx+rx, cy+ry));
            } else if (!strcmp(cur->Name(), "polygon")) {
                const char *pd = cur->Attribute("points");
                if (!pd) {
                    flags |= SVG_IMPORT_PARTIAL_FAILURE_FLAG;
                    continue;
                }
                Point2 point;
                if (!readCoord(point, pd))
                    continue;
                curPath.moveTo(SkScalar(point.x), SkScalar(point.y));
                if (!readCoord(point, pd))
                    continue;
                do {
                    curPath.lineTo(SkScalar(point.x), SkScalar(point.y));
                } while (readCoord(point, pd));
                curPath.close();
            } else
                continue;
            const char *fillRule = cur->Attribute("fill-rule");
            if (fillRule && !strcmp(fillRule, "evenodd"))
                curPath.setFillType(SkPathFillType::kEvenOdd);
            curPath.transform(combineTransformation(flags, transformation, cur->Attribute("transform"), cur->Attribute("transform-origin")));
            if (Op(fullPath, curPath, kUnion_SkPathOp, &fullPath))
                flags |= SVG_IMPORT_SUCCESS_FLAG;
            else
                flags |= SVG_IMPORT_PARTIAL_FAILURE_FLAG;
        }
    }
}

int loadSvgShape(Shape &output, Shape::Bounds &viewBox, const char *filename) {
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(filename))
        return SVG_IMPORT_FAILURE;
    tinyxml2::XMLElement *root = doc.FirstChildElement("svg");
    if (!root)
        return SVG_IMPORT_FAILURE;

    SkPath fullPath;
    int flags = 0;
    gatherPaths(fullPath, flags, root, SkMatrix());
    if (!((flags&SVG_IMPORT_SUCCESS_FLAG) && Simplify(fullPath, &fullPath)))
        return SVG_IMPORT_FAILURE;
    shapeFromSkiaPath(output, fullPath);
    output.inverseYAxis = true;
    output.orientContours();

    viewBox.l = 0, viewBox.b = 0;
    Vector2 dims(root->DoubleAttribute("width"), root->DoubleAttribute("height"));
    if (const char *viewBoxStr = root->Attribute("viewBox"))
        readDouble(viewBox.l, viewBoxStr) && readDouble(viewBox.b, viewBoxStr) && readDouble(dims.x, viewBoxStr) && readDouble(dims.y, viewBoxStr);
    viewBox.r = viewBox.l+dims.x;
    viewBox.t = viewBox.b+dims.y;
    return flags;
}

#endif

#ifdef MSDFGEN_USE_DROPXML

int parseSvgShape(Shape &output, Shape::Bounds &viewBox, const char *svgData, size_t svgLength) {

    class SvgConsumer : public BaseSvgConsumer {
        enum Element {
            BEGINNING,
            IGNORED,
            SVG,
            G,
            PATH,
            RECT,
            CIRCLE,
            ELLIPSE,
            POLYGON
        } curElement;

        // Current element attributes
        struct ElementData {
            StrRange transform, transformOrigin;
            Vector2 pos, dims, radius;
            StrRange pathDef;
            bool fillRuleEvenOdd;
            ElementData() : fillRuleEvenOdd(false) { }
        } elem;

        int ignoredDepth;
        SkMatrix transformation;
        std::stack<SkMatrix> transformationStack;

    public:
        int flags;
        Vector2 dimensions;
        Shape::Bounds viewBox;
        SkPath fullPath;

        SvgConsumer() : curElement(BEGINNING), ignoredDepth(0), flags(0), viewBox() { }

        bool enterElement(const char *nameStart, const char *nameEnd) {
            if (ignoredDepth) {
                ++ignoredDepth;
                return true;
            }
            if (curElement == BEGINNING && SVG_NAME_IS("svg"))
                curElement = SVG;
            else if (SVG_NAME_IS("g"))
                curElement = G;
            else if (SVG_NAME_IS("path"))
                curElement = PATH;
            else if (SVG_NAME_IS("rect"))
                curElement = RECT;
            else if (SVG_NAME_IS("circle"))
                curElement = CIRCLE;
            else if (SVG_NAME_IS("ellipse"))
                curElement = ELLIPSE;
            else if (SVG_NAME_IS("polygon"))
                curElement = POLYGON;
            else {
                curElement = IGNORED;
                ++ignoredDepth;
                if (SVG_NAME_IS("mask") || SVG_NAME_IS("use"))
                    flags |= SVG_IMPORT_UNSUPPORTED_FEATURE_FLAG;
            }
            if (curElement != IGNORED)
                elem = ElementData();
            return true;
        }

        bool leaveElement(const char *nameStart, const char *nameEnd) {
            if (ignoredDepth) {
                --ignoredDepth;
                return true;
            }
            if (SVG_NAME_IS("g")) {
                if (transformationStack.empty())
                    return false;
                transformation = transformationStack.top();
                transformationStack.pop();
            }
            return true;
        }

        bool elementAttribute(const char *nameStart, const char *nameEnd, const char *valueStart, const char *valueEnd) {
            switch (curElement) {
                case BEGINNING:
                case IGNORED:
                    break;
                case SVG:
                    if (SVG_NAME_IS("width"))
                        dimensions.x = SVG_DOUBLEVAL();
                    else if (SVG_NAME_IS("height"))
                        dimensions.y = SVG_DOUBLEVAL();
                    else if (SVG_NAME_IS("viewBox")) {
                        std::string viewBoxStr(SVG_DEC_VAL());
                        const char *strPtr = viewBoxStr.c_str();
                        double w = 0, h = 0;
                        readDouble(viewBox.l, strPtr) && readDouble(viewBox.b, strPtr) && readDouble(w, strPtr) && readDouble(h, strPtr);
                        viewBox.r = viewBox.l+w;
                        viewBox.t = viewBox.b+h;
                    }
                    break;
                case G:
                    break;
                case PATH:
                    if (SVG_NAME_IS("d"))
                        elem.pathDef = StrRange(valueStart, valueEnd);
                    break;
                case RECT:
                    if (SVG_NAME_IS("x"))
                        elem.pos.x = SVG_DOUBLEVAL();
                    else if (SVG_NAME_IS("y"))
                        elem.pos.y = SVG_DOUBLEVAL();
                    else if (SVG_NAME_IS("width"))
                        elem.dims.x = SVG_DOUBLEVAL();
                    else if (SVG_NAME_IS("height"))
                        elem.dims.y = SVG_DOUBLEVAL();
                    else if (SVG_NAME_IS("rx"))
                        elem.radius.x = SVG_DOUBLEVAL();
                    else if (SVG_NAME_IS("ry"))
                        elem.radius.y = SVG_DOUBLEVAL();
                    break;
                case CIRCLE:
                    if (SVG_NAME_IS("cx"))
                        elem.pos.x = SVG_DOUBLEVAL();
                    else if (SVG_NAME_IS("cy"))
                        elem.pos.y = SVG_DOUBLEVAL();
                    else if (SVG_NAME_IS("r"))
                        elem.radius.x = SVG_DOUBLEVAL();
                    break;
                case ELLIPSE:
                    if (SVG_NAME_IS("cx"))
                        elem.pos.x = SVG_DOUBLEVAL();
                    else if (SVG_NAME_IS("cy"))
                        elem.pos.y = SVG_DOUBLEVAL();
                    else if (SVG_NAME_IS("rx"))
                        elem.radius.x = SVG_DOUBLEVAL();
                    else if (SVG_NAME_IS("ry"))
                        elem.radius.y = SVG_DOUBLEVAL();
                    break;
                case POLYGON:
                    if (SVG_NAME_IS("points"))
                        elem.pathDef = StrRange(valueStart, valueEnd);
                    break;
            }
            switch (curElement) {
                case PATH:
                case RECT:
                case CIRCLE:
                case ELLIPSE:
                case POLYGON:
                    if (SVG_NAME_IS("fill-rule"))
                        elem.fillRuleEvenOdd = SVG_DEC_VAL() == "evenodd";
                    // fallthrough
                case G:
                    if (SVG_NAME_IS("transform"))
                        elem.transform = StrRange(valueStart, valueEnd);
                    else if (SVG_NAME_IS("transform-origin"))
                        elem.transformOrigin = StrRange(valueStart, valueEnd);
                    break;
                default:;
            }
            return true;
        }

        bool finishAttributes() {
            switch (curElement) {
                case BEGINNING:
                case IGNORED:
                case SVG:
                    break;
                case G:
                    transformationStack.push(transformation);
                    transformation = combineTransformation(flags, transformation, elem.transform.str().c_str(), elem.transformOrigin.str().c_str());
                    break;
                case PATH:
                case RECT:
                case CIRCLE:
                case ELLIPSE:
                case POLYGON:
                    {
                        SkPath curPath;
                        switch (curElement) {
                            case PATH:
                                if (!SkParsePath::FromSVGString(elem.pathDef.str().c_str(), &curPath)) {
                                    flags |= SVG_IMPORT_PARTIAL_FAILURE_FLAG;
                                    return true;
                                }
                                break;
                            case RECT:
                                {
                                    if (!(elem.dims.x && elem.dims.y))
                                        return true;
                                    SkRect rect = SkRect::MakeLTRB(elem.pos.x, elem.pos.y, elem.pos.x+elem.dims.x, elem.pos.y+elem.dims.y);
                                    if (elem.radius.x || elem.radius.y) {
                                        SkScalar rx = SkScalar(elem.radius.x), ry = SkScalar(elem.radius.y);
                                        SkScalar radii[] = { rx, ry, rx, ry, rx, ry, rx, ry };
                                        curPath.addRoundRect(rect, radii);
                                    } else
                                        curPath.addRect(rect);
                                }
                                break;
                            case CIRCLE:
                                if (!elem.radius.x)
                                    return true;
                                curPath.addCircle(elem.pos.x, elem.pos.y, elem.radius.x);
                                break;
                            case ELLIPSE:
                                if (!(elem.radius.x && elem.radius.y))
                                    return true;
                                curPath.addOval(SkRect::MakeLTRB(elem.pos.x-elem.radius.x, elem.pos.y-elem.radius.y, elem.pos.x+elem.radius.x, elem.pos.y+elem.radius.y));
                                break;
                            case POLYGON:
                                {
                                    if (elem.pathDef.start == elem.pathDef.end) {
                                        flags |= SVG_IMPORT_PARTIAL_FAILURE_FLAG;
                                        return true;
                                    }
                                    std::string pdStr = elem.pathDef.str();
                                    const char *pd = pdStr.c_str();
                                    Point2 point;
                                    if (!readCoord(point, pd))
                                        return true;
                                    curPath.moveTo(SkScalar(point.x), SkScalar(point.y));
                                    if (!readCoord(point, pd))
                                        return true;
                                    do {
                                        curPath.lineTo(SkScalar(point.x), SkScalar(point.y));
                                    } while (readCoord(point, pd));
                                    curPath.close();
                                }
                                break;
                            default:
                                return true;
                        }
                        if (elem.fillRuleEvenOdd)
                            curPath.setFillType(SkPathFillType::kEvenOdd);
                        curPath.transform(combineTransformation(flags, transformation, elem.transform.str().c_str(), elem.transformOrigin.str().c_str()));
                        if (Op(fullPath, curPath, kUnion_SkPathOp, &fullPath))
                            flags |= SVG_IMPORT_SUCCESS_FLAG;
                        else
                            flags |= SVG_IMPORT_PARTIAL_FAILURE_FLAG;
                    }
                    break;
            }
            return true;
        }

        bool finish() {
            return !ignoredDepth && transformationStack.empty();
        }

    };

    SvgConsumer svg;
    if (!(
        dropXML::parse(svg, svgData, svgData+svgLength) &&
        (svg.flags&SVG_IMPORT_SUCCESS_FLAG) &&
        Simplify(svg.fullPath, &svg.fullPath)
    ))
        return SVG_IMPORT_FAILURE;

    shapeFromSkiaPath(output, svg.fullPath);
    output.inverseYAxis = true;
    output.orientContours();

    viewBox = svg.viewBox;
    if (svg.dimensions.x > 0 && viewBox.r == viewBox.l)
        viewBox.r += svg.dimensions.x;
    if (svg.dimensions.y > 0 && viewBox.t == viewBox.b)
        viewBox.t += svg.dimensions.y;
    return svg.flags;
}

int loadSvgShape(Shape &output, Shape::Bounds &viewBox, const char *filename) {
    std::vector<char> svgData;
    if (!readFile(svgData, filename))
        return SVG_IMPORT_FAILURE;
    return parseSvgShape(output, viewBox, svgData.empty() ? NULL : &svgData[0], svgData.size());
}

#endif

#endif

}

#endif
