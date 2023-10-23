
#include "edge-segments.h"

#include "arithmetics.hpp"
#include "equation-solver.h"
#include "bezier-solver.hpp"

#define MSDFGEN_USE_BEZIER_SOLVER

namespace msdfgen {

void EdgeSegment::distanceToPseudoDistance(SignedDistance &distance, Point2 origin, double param) const {
    if (param < 0) {
        Vector2 dir = direction(0).normalize();
        Vector2 aq = origin-point(0);
        double ts = dotProduct(aq, dir);
        if (ts < 0) {
            double pseudoDistance = crossProduct(aq, dir);
            if (fabs(pseudoDistance) <= fabs(distance.distance)) {
                distance.distance = pseudoDistance;
                distance.dot = 0;
            }
        }
    } else if (param > 1) {
        Vector2 dir = direction(1).normalize();
        Vector2 bq = origin-point(1);
        double ts = dotProduct(bq, dir);
        if (ts > 0) {
            double pseudoDistance = crossProduct(bq, dir);
            if (fabs(pseudoDistance) <= fabs(distance.distance)) {
                distance.distance = pseudoDistance;
                distance.dot = 0;
            }
        }
    }
}

LinearSegment::LinearSegment(Point2 p0, Point2 p1, EdgeColor edgeColor) : EdgeSegment(edgeColor) {
    p[0] = p0;
    p[1] = p1;
}

QuadraticSegment::QuadraticSegment(Point2 p0, Point2 p1, Point2 p2, EdgeColor edgeColor) : EdgeSegment(edgeColor) {
    if (p1 == p0 || p1 == p2)
        p1 = 0.5*(p0+p2);
    p[0] = p0;
    p[1] = p1;
    p[2] = p2;
}

CubicSegment::CubicSegment(Point2 p0, Point2 p1, Point2 p2, Point2 p3, EdgeColor edgeColor) : EdgeSegment(edgeColor) {
    if ((p1 == p0 || p1 == p3) && (p2 == p0 || p2 == p3)) {
        p1 = mix(p0, p3, 1/3.);
        p2 = mix(p0, p3, 2/3.);
    }
    p[0] = p0;
    p[1] = p1;
    p[2] = p2;
    p[3] = p3;
}

LinearSegment *LinearSegment::clone() const {
    return new LinearSegment(p[0], p[1], color);
}

QuadraticSegment *QuadraticSegment::clone() const {
    return new QuadraticSegment(p[0], p[1], p[2], color);
}

CubicSegment *CubicSegment::clone() const {
    return new CubicSegment(p[0], p[1], p[2], p[3], color);
}

int LinearSegment::type() const {
    return (int) EDGE_TYPE;
}

int QuadraticSegment::type() const {
    return (int) EDGE_TYPE;
}

int CubicSegment::type() const {
    return (int) EDGE_TYPE;
}

const Point2 *LinearSegment::controlPoints() const {
    return p;
}

const Point2 *QuadraticSegment::controlPoints() const {
    return p;
}

const Point2 *CubicSegment::controlPoints() const {
    return p;
}

Point2 LinearSegment::point(double param) const {
    return mix(p[0], p[1], param);
}

Point2 QuadraticSegment::point(double param) const {
    return mix(mix(p[0], p[1], param), mix(p[1], p[2], param), param);
}

Point2 CubicSegment::point(double param) const {
    Vector2 p12 = mix(p[1], p[2], param);
    return mix(mix(mix(p[0], p[1], param), p12, param), mix(p12, mix(p[2], p[3], param), param), param);
}

Vector2 LinearSegment::direction(double param) const {
    return p[1]-p[0];
}

Vector2 QuadraticSegment::direction(double param) const {
    Vector2 tangent = mix(p[1]-p[0], p[2]-p[1], param);
    if (!tangent)
        return p[2]-p[0];
    return tangent;
}

Vector2 CubicSegment::direction(double param) const {
    Vector2 tangent = mix(mix(p[1]-p[0], p[2]-p[1], param), mix(p[2]-p[1], p[3]-p[2], param), param);
    if (!tangent) {
        if (param == 0) return p[2]-p[0];
        if (param == 1) return p[3]-p[1];
    }
    return tangent;
}

Vector2 LinearSegment::directionChange(double param) const {
    return Vector2();
}

Vector2 QuadraticSegment::directionChange(double param) const {
    return (p[2]-p[1])-(p[1]-p[0]);
}

Vector2 CubicSegment::directionChange(double param) const {
    return mix((p[2]-p[1])-(p[1]-p[0]), (p[3]-p[2])-(p[2]-p[1]), param);
}

double LinearSegment::length() const {
    return (p[1]-p[0]).length();
}

double QuadraticSegment::length() const {
    Vector2 ab = p[1]-p[0];
    Vector2 br = p[2]-p[1]-ab;
    double abab = dotProduct(ab, ab);
    double abbr = dotProduct(ab, br);
    double brbr = dotProduct(br, br);
    double abLen = sqrt(abab);
    double brLen = sqrt(brbr);
    double crs = crossProduct(ab, br);
    double h = sqrt(abab+abbr+abbr+brbr);
    return (
        brLen*((abbr+brbr)*h-abbr*abLen)+
        crs*crs*log((brLen*h+abbr+brbr)/(brLen*abLen+abbr))
    )/(brbr*brLen);
}

SignedDistance LinearSegment::signedDistance(Point2 origin, double &param) const {
    Vector2 aq = origin-p[0];
    Vector2 ab = p[1]-p[0];
    param = dotProduct(aq, ab)/dotProduct(ab, ab);
    Vector2 eq = p[param > .5]-origin;
    double endpointDistance = eq.length();
    if (param > 0 && param < 1) {
        double orthoDistance = dotProduct(ab.getOrthonormal(false), aq);
        if (fabs(orthoDistance) < endpointDistance)
            return SignedDistance(orthoDistance, 0);
    }
    return SignedDistance(nonZeroSign(crossProduct(aq, ab))*endpointDistance, fabs(dotProduct(ab.normalize(), eq.normalize())));
}

#ifdef MSDFGEN_USE_BEZIER_SOLVER

SignedDistance QuadraticSegment::signedDistance(Point2 origin, double &param) const {
    Vector2 ap = origin-p[0];
    Vector2 bp = origin-p[2];
    Vector2 q = 2*(p[1]-p[0]);
    Vector2 r = p[2]-2*p[1]+p[0];
    double aSqD = ap.squaredLength();
    double bSqD = bp.squaredLength();
    double t = quadraticNearPoint(ap, q, r);
    if (t > 0 && t < 1) {
        Vector2 tp = ap-(q+r*t)*t;
        double tSqD = tp.squaredLength();
        if (tSqD < aSqD && tSqD < bSqD) {
            param = t;
            return SignedDistance(nonZeroSign(crossProduct(tp, q+2*r*t))*sqrt(tSqD), 0);
        }
    }
    if (bSqD < aSqD) {
        Vector2 d = q+r+r;
        if (!d)
            d = p[2]-p[0];
        param = dotProduct(bp, d)/d.squaredLength()+1;
        return SignedDistance(nonZeroSign(crossProduct(bp, d))*sqrt(bSqD), dotProduct(bp.normalize(), d.normalize()));
    }
    if (!q)
        q = p[2]-p[0];
    param = dotProduct(ap, q)/q.squaredLength();
    return SignedDistance(nonZeroSign(crossProduct(ap, q))*sqrt(aSqD), -dotProduct(ap.normalize(), q.normalize()));
}

SignedDistance CubicSegment::signedDistance(Point2 origin, double &param) const {
    Vector2 ap = origin-p[0];
    Vector2 bp = origin-p[3];
    Vector2 q = 3*(p[1]-p[0]);
    Vector2 r = 3*(p[2]-p[1])-q;
    Vector2 s = p[3]-3*(p[2]-p[1])-p[0];
    double aSqD = ap.squaredLength();
    double bSqD = bp.squaredLength();
    double tSqD;
    double t = cubicNearPoint(ap, q, r, s, tSqD);
    if (t > 0 && t < 1) {
        if (tSqD < aSqD && tSqD < bSqD) {
            param = t;
            return SignedDistance(nonZeroSign(crossProduct(ap-(q+(r+s*t)*t)*t, q+(r+r+3*s*t)*t))*sqrt(tSqD), 0);
        }
    }
    if (bSqD < aSqD) {
        Vector2 d = q+r+r+3*s;
        if (!d)
            d = p[3]-p[1];
        param = dotProduct(bp, d)/d.squaredLength()+1;
        return SignedDistance(nonZeroSign(crossProduct(bp, d))*sqrt(bSqD), dotProduct(bp.normalize(), d.normalize()));
    }
    if (!q)
        q = p[2]-p[0];
    param = dotProduct(ap, q)/q.squaredLength();
    return SignedDistance(nonZeroSign(crossProduct(ap, q))*sqrt(aSqD), -dotProduct(ap.normalize(), q.normalize()));
}

#else

SignedDistance QuadraticSegment::signedDistance(Point2 origin, double &param) const {
    Vector2 qa = p[0]-origin;
    Vector2 ab = p[1]-p[0];
    Vector2 br = p[2]-p[1]-ab;
    double a = dotProduct(br, br);
    double b = 3*dotProduct(ab, br);
    double c = 2*dotProduct(ab, ab)+dotProduct(qa, br);
    double d = dotProduct(qa, ab);
    double t[3];
    int solutions = solveCubic(t, a, b, c, d);

    Vector2 epDir = direction(0);
    double minDistance = nonZeroSign(crossProduct(epDir, qa))*qa.length(); // distance from A
    param = -dotProduct(qa, epDir)/dotProduct(epDir, epDir);
    {
        epDir = direction(1);
        double distance = (p[2]-origin).length(); // distance from B
        if (distance < fabs(minDistance)) {
            minDistance = nonZeroSign(crossProduct(epDir, p[2]-origin))*distance;
            param = dotProduct(origin-p[1], epDir)/dotProduct(epDir, epDir);
        }
    }
    for (int i = 0; i < solutions; ++i) {
        if (t[i] > 0 && t[i] < 1) {
            Point2 qe = qa+2*t[i]*ab+t[i]*t[i]*br;
            double distance = qe.length();
            if (distance <= fabs(minDistance)) {
                minDistance = nonZeroSign(crossProduct(ab+t[i]*br, qe))*distance;
                param = t[i];
            }
        }
    }

    if (param >= 0 && param <= 1)
        return SignedDistance(minDistance, 0);
    if (param < .5)
        return SignedDistance(minDistance, fabs(dotProduct(direction(0).normalize(), qa.normalize())));
    else
        return SignedDistance(minDistance, fabs(dotProduct(direction(1).normalize(), (p[2]-origin).normalize())));
}

SignedDistance CubicSegment::signedDistance(Point2 origin, double &param) const {
    Vector2 qa = p[0]-origin;
    Vector2 ab = p[1]-p[0];
    Vector2 br = p[2]-p[1]-ab;
    Vector2 as = (p[3]-p[2])-(p[2]-p[1])-br;

    Vector2 epDir = direction(0);
    double minDistance = nonZeroSign(crossProduct(epDir, qa))*qa.length(); // distance from A
    param = -dotProduct(qa, epDir)/dotProduct(epDir, epDir);
    {
        epDir = direction(1);
        double distance = (p[3]-origin).length(); // distance from B
        if (distance < fabs(minDistance)) {
            minDistance = nonZeroSign(crossProduct(epDir, p[3]-origin))*distance;
            param = dotProduct(epDir-(p[3]-origin), epDir)/dotProduct(epDir, epDir);
        }
    }
    // Iterative minimum distance search
    for (int i = 0; i <= MSDFGEN_CUBIC_SEARCH_STARTS; ++i) {
        double t = (double) i/MSDFGEN_CUBIC_SEARCH_STARTS;
        Vector2 qe = qa+3*t*ab+3*t*t*br+t*t*t*as;
        for (int step = 0; step < MSDFGEN_CUBIC_SEARCH_STEPS; ++step) {
            // Improve t
            Vector2 d1 = 3*ab+6*t*br+3*t*t*as;
            Vector2 d2 = 6*br+6*t*as;
            t -= dotProduct(qe, d1)/(dotProduct(d1, d1)+dotProduct(qe, d2));
            if (t <= 0 || t >= 1)
                break;
            qe = qa+3*t*ab+3*t*t*br+t*t*t*as;
            double distance = qe.length();
            if (distance < fabs(minDistance)) {
                minDistance = nonZeroSign(crossProduct(d1, qe))*distance;
                param = t;
            }
        }
    }

    if (param >= 0 && param <= 1)
        return SignedDistance(minDistance, 0);
    if (param < .5)
        return SignedDistance(minDistance, fabs(dotProduct(direction(0).normalize(), qa.normalize())));
    else
        return SignedDistance(minDistance, fabs(dotProduct(direction(1).normalize(), (p[3]-origin).normalize())));
}

#endif

int LinearSegment::scanlineIntersections(double x[3], int dy[3], double y) const {
    return horizontalScanlineIntersections(x, dy, y);
}

int QuadraticSegment::scanlineIntersections(double x[3], int dy[3], double y) const {
    return horizontalScanlineIntersections(x, dy, y);
}

int CubicSegment::scanlineIntersections(double x[3], int dy[3], double y) const {
    return horizontalScanlineIntersections(x, dy, y);
}

int LinearSegment::horizontalScanlineIntersections(double x[3], int dy[3], double y) const {
    if ((y >= p[0].y && y < p[1].y) || (y >= p[1].y && y < p[0].y)) {
        double param = (y-p[0].y)/(p[1].y-p[0].y);
        x[0] = mix(p[0].x, p[1].x, param);
        dy[0] = sign(p[1].y-p[0].y);
        return 1;
    }
    return 0;
}

int LinearSegment::verticalScanlineIntersections(double y[3], int dx[3], double x) const {
    if ((x >= p[0].x && x < p[1].x) || (x >= p[1].x && x < p[0].x)) {
        double param = (x-p[0].x)/(p[1].x-p[0].x);
        y[0] = mix(p[0].y, p[1].y, param);
        dx[0] = sign(p[1].x-p[0].x);
        return 1;
    }
    return 0;
}

// p0, p1, q, r, s in the dimension of the scanline
// p0 = scanline - first endpoint
// p1 = scanline - last endpoint
// q, r, s = first, second, third order derivatives (see bezier-solver)
// descendingAtEnd is true if the first non-zero derivative at the last endpoint is negative
static int curveScanlineIntersections(double t[3], int d[3], double p0, double p1, double q, double r, double s, bool descendingAtEnd) {
    int total = 0;
    int nextD = p0 > 0 ? 1 : -1;
    t[total] = 0;
    if (p0 == 0) {
        if (q > 0 || (q == 0 && (r > 0 || (r == 0 && s > 0))))
            d[total++] = 1;
        else
            nextD = 1;
    }
    {
        double ts[3];
        int solutions = solveCubic(ts, s, r, q, -p0);
        // Sort solutions
        double tmp;
        if (solutions >= 2) {
            if (ts[0] > ts[1])
                tmp = ts[0], ts[0] = ts[1], ts[1] = tmp;
            if (solutions >= 3 && ts[1] > ts[2]) {
                tmp = ts[1], ts[1] = ts[2], ts[2] = tmp;
                if (ts[0] > ts[1])
                    tmp = ts[0], ts[0] = ts[1], ts[1] = tmp;
            }
        }
        for (int i = 0; i < solutions && total < 3; ++i) {
            if (ts[i] >= 0 && ts[i] <= 1) {
                t[total] = ts[i];
                if (nextD*(q+(2*r+3*s*ts[i])*ts[i]) >= 0) {
                    d[total++] = nextD;
                    nextD = -nextD;
                }
            }
        }
    }
    if (p1 == 0) {
        if (nextD > 0 && total > 0) {
            --total;
            nextD = -1;
        }
        if (descendingAtEnd && total < 3) {
            t[total] = 1;
            if (nextD < 0) {
                d[total++] = -1;
                nextD = 1;
            }
        }
    }
    if (nextD != (p1 >= 0 ? 1 : -1)) {
        if (total > 0)
            --total;
        else {
            if (fabs(p0) > fabs(p1))
                t[total] = 1;
            d[total++] = nextD;
        }
    }
    return total;
}

int QuadraticSegment::horizontalScanlineIntersections(double x[3], int dy[3], double y) const {
    double p01 = p[1].y-p[0].y;
    double p12 = p[2].y-p[1].y;
    bool descendingAtEnd = p[2].y < p[1].y || (p[2].y == p[1].y && p[2].y < p[0].y);
    double t[3] = { };
    int n = curveScanlineIntersections(t, dy, y-p[0].y, y-p[2].y, 2*p01, p12-p01, 0, descendingAtEnd);
    x[0] = point(t[0]).x;
    x[1] = point(t[1]).x;
    x[2] = point(t[2]).x;
    return n;
}

int QuadraticSegment::verticalScanlineIntersections(double y[3], int dx[3], double x) const {
    double p01 = p[1].x-p[0].x;
    double p12 = p[2].x-p[1].x;
    bool descendingAtEnd = p[2].x < p[1].x || (p[2].x == p[1].x && p[2].x < p[0].x);
    double t[3] = { };
    int n = curveScanlineIntersections(t, dx, x-p[0].x, x-p[2].x, 2*p01, p12-p01, 0, descendingAtEnd);
    y[0] = point(t[0]).y;
    y[1] = point(t[1]).y;
    y[2] = point(t[2]).y;
    return n;
}

int CubicSegment::horizontalScanlineIntersections(double x[3], int dy[3], double y) const {
    double p01 = p[1].y-p[0].y;
    double p12 = p[2].y-p[1].y;
    double p23 = p[3].y-p[2].y;
    bool descendingAtEnd = p[3].y < p[2].y || (p[3].y == p[2].y && (p[3].y < p[1].y || (p[3].y == p[1].y && p[3].y < p[0].y)));
    double t[3] = { };
    int n = curveScanlineIntersections(t, dy, y-p[0].y, y-p[3].y, 3*p01, 3*(p12-p01), p23-2*p12+p01, descendingAtEnd);
    x[0] = point(t[0]).x;
    x[1] = point(t[1]).x;
    x[2] = point(t[2]).x;
    return n;
}

int CubicSegment::verticalScanlineIntersections(double y[3], int dx[3], double x) const {
    double p01 = p[1].x-p[0].x;
    double p12 = p[2].x-p[1].x;
    double p23 = p[3].x-p[2].x;
    bool descendingAtEnd = p[3].x < p[2].x || (p[3].x == p[2].x && (p[3].x < p[1].x || (p[3].x == p[1].x && p[3].x < p[0].x)));
    double t[3] = { };
    int n = curveScanlineIntersections(t, dx, x-p[0].x, x-p[3].x, 3*p01, 3*(p12-p01), p23-2*p12+p01, descendingAtEnd);
    y[0] = point(t[0]).y;
    y[1] = point(t[1]).y;
    y[2] = point(t[2]).y;
    return n;
}

static void pointBounds(Point2 p, double &l, double &b, double &r, double &t) {
    if (p.x < l) l = p.x;
    if (p.y < b) b = p.y;
    if (p.x > r) r = p.x;
    if (p.y > t) t = p.y;
}

void LinearSegment::bound(double &l, double &b, double &r, double &t) const {
    pointBounds(p[0], l, b, r, t);
    pointBounds(p[1], l, b, r, t);
}

void QuadraticSegment::bound(double &l, double &b, double &r, double &t) const {
    pointBounds(p[0], l, b, r, t);
    pointBounds(p[2], l, b, r, t);
    Vector2 bot = (p[1]-p[0])-(p[2]-p[1]);
    if (bot.x) {
        double param = (p[1].x-p[0].x)/bot.x;
        if (param > 0 && param < 1)
            pointBounds(point(param), l, b, r, t);
    }
    if (bot.y) {
        double param = (p[1].y-p[0].y)/bot.y;
        if (param > 0 && param < 1)
            pointBounds(point(param), l, b, r, t);
    }
}

void CubicSegment::bound(double &l, double &b, double &r, double &t) const {
    pointBounds(p[0], l, b, r, t);
    pointBounds(p[3], l, b, r, t);
    Vector2 a0 = p[1]-p[0];
    Vector2 a1 = 2*(p[2]-p[1]-a0);
    Vector2 a2 = p[3]-3*p[2]+3*p[1]-p[0];
    double params[2];
    int solutions;
    solutions = solveQuadratic(params, a2.x, a1.x, a0.x);
    for (int i = 0; i < solutions; ++i)
        if (params[i] > 0 && params[i] < 1)
            pointBounds(point(params[i]), l, b, r, t);
    solutions = solveQuadratic(params, a2.y, a1.y, a0.y);
    for (int i = 0; i < solutions; ++i)
        if (params[i] > 0 && params[i] < 1)
            pointBounds(point(params[i]), l, b, r, t);
}

void LinearSegment::reverse() {
    Point2 tmp = p[0];
    p[0] = p[1];
    p[1] = tmp;
}

void QuadraticSegment::reverse() {
    Point2 tmp = p[0];
    p[0] = p[2];
    p[2] = tmp;
}

void CubicSegment::reverse() {
    Point2 tmp = p[0];
    p[0] = p[3];
    p[3] = tmp;
    tmp = p[1];
    p[1] = p[2];
    p[2] = tmp;
}

void LinearSegment::moveStartPoint(Point2 to) {
    p[0] = to;
}

void QuadraticSegment::moveStartPoint(Point2 to) {
    Vector2 origSDir = p[0]-p[1];
    Point2 origP1 = p[1];
    p[1] += crossProduct(p[0]-p[1], to-p[0])/crossProduct(p[0]-p[1], p[2]-p[1])*(p[2]-p[1]);
    p[0] = to;
    if (dotProduct(origSDir, p[0]-p[1]) < 0)
        p[1] = origP1;
}

void CubicSegment::moveStartPoint(Point2 to) {
    p[1] += to-p[0];
    p[0] = to;
}

void LinearSegment::moveEndPoint(Point2 to) {
    p[1] = to;
}

void QuadraticSegment::moveEndPoint(Point2 to) {
    Vector2 origEDir = p[2]-p[1];
    Point2 origP1 = p[1];
    p[1] += crossProduct(p[2]-p[1], to-p[2])/crossProduct(p[2]-p[1], p[0]-p[1])*(p[0]-p[1]);
    p[2] = to;
    if (dotProduct(origEDir, p[2]-p[1]) < 0)
        p[1] = origP1;
}

void CubicSegment::moveEndPoint(Point2 to) {
    p[2] += to-p[3];
    p[3] = to;
}

void LinearSegment::splitInThirds(EdgeSegment *&part1, EdgeSegment *&part2, EdgeSegment *&part3) const {
    part1 = new LinearSegment(p[0], point(1/3.), color);
    part2 = new LinearSegment(point(1/3.), point(2/3.), color);
    part3 = new LinearSegment(point(2/3.), p[1], color);
}

void QuadraticSegment::splitInThirds(EdgeSegment *&part1, EdgeSegment *&part2, EdgeSegment *&part3) const {
    part1 = new QuadraticSegment(p[0], mix(p[0], p[1], 1/3.), point(1/3.), color);
    part2 = new QuadraticSegment(point(1/3.), mix(mix(p[0], p[1], 5/9.), mix(p[1], p[2], 4/9.), .5), point(2/3.), color);
    part3 = new QuadraticSegment(point(2/3.), mix(p[1], p[2], 2/3.), p[2], color);
}

void CubicSegment::splitInThirds(EdgeSegment *&part1, EdgeSegment *&part2, EdgeSegment *&part3) const {
    part1 = new CubicSegment(p[0], p[0] == p[1] ? p[0] : mix(p[0], p[1], 1/3.), mix(mix(p[0], p[1], 1/3.), mix(p[1], p[2], 1/3.), 1/3.), point(1/3.), color);
    part2 = new CubicSegment(point(1/3.),
        mix(mix(mix(p[0], p[1], 1/3.), mix(p[1], p[2], 1/3.), 1/3.), mix(mix(p[1], p[2], 1/3.), mix(p[2], p[3], 1/3.), 1/3.), 2/3.),
        mix(mix(mix(p[0], p[1], 2/3.), mix(p[1], p[2], 2/3.), 2/3.), mix(mix(p[1], p[2], 2/3.), mix(p[2], p[3], 2/3.), 2/3.), 1/3.),
        point(2/3.), color);
    part3 = new CubicSegment(point(2/3.), mix(mix(p[1], p[2], 2/3.), mix(p[2], p[3], 2/3.), 2/3.), p[2] == p[3] ? p[3] : mix(p[2], p[3], 2/3.), p[3], color);
}

EdgeSegment *QuadraticSegment::convertToCubic() const {
    return new CubicSegment(p[0], mix(p[0], p[1], 2/3.), mix(p[1], p[2], 1/3.), p[2], color);
}

void CubicSegment::deconverge(int param, double amount) {
    Vector2 dir = direction(param);
    Vector2 normal = dir.getOrthonormal();
    double h = dotProduct(directionChange(param)-dir, normal);
    switch (param) {
        case 0:
            p[1] += amount*(dir+sign(h)*sqrt(fabs(h))*normal);
            break;
        case 1:
            p[2] -= amount*(dir-sign(h)*sqrt(fabs(h))*normal);
            break;
    }
}

}
