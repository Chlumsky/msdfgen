
#include "edge-segments.h"

#include "arithmetics.hpp"
#include "equation-solver.h"
#include "bezier-solver.hpp"

#define MSDFGEN_USE_BEZIER_SOLVER

namespace msdfgen {

void EdgeSegment::distanceToPseudoDistance(SignedDistance &distance, Point2 origin, real param) const {
    if (param < real(0)) {
        Vector2 dir = direction(0).normalize();
        Vector2 aq = origin-point(0);
        real ts = dotProduct(aq, dir);
        if (ts < real(0)) {
            real pseudoDistance = crossProduct(aq, dir);
            if (fabs(pseudoDistance) <= fabs(distance.distance)) {
                distance.distance = pseudoDistance;
                distance.dot = 0;
            }
        }
    } else if (param > real(1)) {
        Vector2 dir = direction(1).normalize();
        Vector2 bq = origin-point(1);
        real ts = dotProduct(bq, dir);
        if (ts > real(0)) {
            real pseudoDistance = crossProduct(bq, dir);
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
        p1 = real(.5)*(p0+p2);
    p[0] = p0;
    p[1] = p1;
    p[2] = p2;
}

CubicSegment::CubicSegment(Point2 p0, Point2 p1, Point2 p2, Point2 p3, EdgeColor edgeColor) : EdgeSegment(edgeColor) {
    if ((p1 == p0 || p1 == p3) && (p2 == p0 || p2 == p3)) {
        p1 = mix(p0, p3, real(1)/real(3));
        p2 = mix(p0, p3, real(2)/real(3));
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

Point2 LinearSegment::point(real param) const {
    return mix(p[0], p[1], param);
}

Point2 QuadraticSegment::point(real param) const {
    return mix(mix(p[0], p[1], param), mix(p[1], p[2], param), param);
}

Point2 CubicSegment::point(real param) const {
    Vector2 p12 = mix(p[1], p[2], param);
    return mix(mix(mix(p[0], p[1], param), p12, param), mix(p12, mix(p[2], p[3], param), param), param);
}

Vector2 LinearSegment::direction(real param) const {
    return p[1]-p[0];
}

Vector2 QuadraticSegment::direction(real param) const {
    Vector2 tangent = mix(p[1]-p[0], p[2]-p[1], param);
    if (!tangent)
        return p[2]-p[0];
    return tangent;
}

Vector2 CubicSegment::direction(real param) const {
    Vector2 tangent = mix(mix(p[1]-p[0], p[2]-p[1], param), mix(p[2]-p[1], p[3]-p[2], param), param);
    if (!tangent) {
        if (param == 0) return p[2]-p[0];
        if (param == 1) return p[3]-p[1];
    }
    return tangent;
}

Vector2 LinearSegment::directionChange(real param) const {
    return Vector2();
}

Vector2 QuadraticSegment::directionChange(real param) const {
    return (p[2]-p[1])-(p[1]-p[0]);
}

Vector2 CubicSegment::directionChange(real param) const {
    return mix((p[2]-p[1])-(p[1]-p[0]), (p[3]-p[2])-(p[2]-p[1]), param);
}

real LinearSegment::length() const {
    return (p[1]-p[0]).length();
}

real QuadraticSegment::length() const {
    Vector2 ab = p[1]-p[0];
    Vector2 br = p[2]-p[1]-ab;
    real abab = dotProduct(ab, ab);
    real abbr = dotProduct(ab, br);
    real brbr = dotProduct(br, br);
    real abLen = sqrt(abab);
    real brLen = sqrt(brbr);
    real crs = crossProduct(ab, br);
    real h = sqrt(abab+abbr+abbr+brbr);
    return (
        brLen*((abbr+brbr)*h-abbr*abLen)+
        crs*crs*log((brLen*h+abbr+brbr)/(brLen*abLen+abbr))
    )/(brbr*brLen);
}

SignedDistance LinearSegment::signedDistance(Point2 origin, real &param) const {
    Vector2 aq = origin-p[0];
    Vector2 ab = p[1]-p[0];
    param = dotProduct(aq, ab)/dotProduct(ab, ab);
    Vector2 eq = p[param > real(.5)]-origin;
    real endpointDistance = eq.length();
    if (param > real(0) && param < real(1)) {
        real orthoDistance = dotProduct(ab.getOrthonormal(false), aq);
        if (fabs(orthoDistance) < endpointDistance)
            return SignedDistance(orthoDistance, 0);
    }
    return SignedDistance(real(nonZeroSign(crossProduct(aq, ab)))*endpointDistance, fabs(dotProduct(ab.normalize(), eq.normalize())));
}

#ifdef MSDFGEN_USE_BEZIER_SOLVER

SignedDistance QuadraticSegment::signedDistance(Point2 origin, real &param) const {
    Vector2 ap = origin-p[0];
    Vector2 bp = origin-p[2];
    Vector2 q = real(2)*(p[1]-p[0]);
    Vector2 r = p[2]-2*p[1]+p[0];
    real aSqD = ap.squaredLength();
    real bSqD = bp.squaredLength();
    real t = quadraticNearPoint(ap, q, r);
    if (t > real(0) && t < real(1)) {
        Vector2 tp = ap-(q+r*t)*t;
        real tSqD = tp.squaredLength();
        if (tSqD < aSqD && tSqD < bSqD) {
            param = t;
            return SignedDistance(real(nonZeroSign(crossProduct(tp, q+real(2)*r*t)))*sqrt(tSqD), 0);
        }
    }
    if (bSqD < aSqD) {
        Vector2 d = q+r+r;
        if (!d)
            d = p[2]-p[0];
        param = dotProduct(bp, d)/d.squaredLength()+real(1);
        return SignedDistance(real(nonZeroSign(crossProduct(bp, d)))*sqrt(bSqD), dotProduct(bp.normalize(), d.normalize()));
    }
    if (!q)
        q = p[2]-p[0];
    param = dotProduct(ap, q)/q.squaredLength();
    return SignedDistance(real(nonZeroSign(crossProduct(ap, q)))*sqrt(aSqD), -dotProduct(ap.normalize(), q.normalize()));
}

SignedDistance CubicSegment::signedDistance(Point2 origin, real &param) const {
    Vector2 ap = origin-p[0];
    Vector2 bp = origin-p[3];
    Vector2 q = real(3)*(p[1]-p[0]);
    Vector2 r = real(3)*(p[2]-p[1])-q;
    Vector2 s = p[3]-real(3)*(p[2]-p[1])-p[0];
    real aSqD = ap.squaredLength();
    real bSqD = bp.squaredLength();
    real tSqD;
    real t = cubicNearPoint(ap, q, r, s, tSqD);
    if (t > real(0) && t < real(1)) {
        if (tSqD < aSqD && tSqD < bSqD) {
            param = t;
            return SignedDistance(real(nonZeroSign(crossProduct(ap-(q+(r+s*t)*t)*t, q+(r+r+real(3)*s*t)*t)))*sqrt(tSqD), 0);
        }
    }
    if (bSqD < aSqD) {
        Vector2 d = q+r+r+real(3)*s;
        if (!d)
            d = p[3]-p[1];
        param = dotProduct(bp, d)/d.squaredLength()+real(1);
        return SignedDistance(real(nonZeroSign(crossProduct(bp, d)))*sqrt(bSqD), dotProduct(bp.normalize(), d.normalize()));
    }
    if (!q)
        q = p[2]-p[0];
    param = dotProduct(ap, q)/q.squaredLength();
    return SignedDistance(real(nonZeroSign(crossProduct(ap, q)))*sqrt(aSqD), -dotProduct(ap.normalize(), q.normalize()));
}

#else

SignedDistance QuadraticSegment::signedDistance(Point2 origin, real &param) const {
    Vector2 qa = p[0]-origin;
    Vector2 ab = p[1]-p[0];
    Vector2 br = p[2]-p[1]-ab;
    real a = dotProduct(br, br);
    real b = real(3)*dotProduct(ab, br);
    real c = real(2)*dotProduct(ab, ab)+dotProduct(qa, br);
    real d = dotProduct(qa, ab);
    real t[3];
    int solutions = solveCubic(t, a, b, c, d);

    Vector2 epDir = direction(0);
    real minDistance = real(nonZeroSign(crossProduct(epDir, qa)))*qa.length(); // distance from A
    param = -dotProduct(qa, epDir)/dotProduct(epDir, epDir);
    {
        epDir = direction(1);
        real distance = (p[2]-origin).length(); // distance from B
        if (distance < fabs(minDistance)) {
            minDistance = real(nonZeroSign(crossProduct(epDir, p[2]-origin)))*distance;
            param = dotProduct(origin-p[1], epDir)/dotProduct(epDir, epDir);
        }
    }
    for (int i = 0; i < solutions; ++i) {
        if (t[i] > real(0) && t[i] < real(1)) {
            Point2 qe = qa+real(2)*t[i]*ab+t[i]*t[i]*br;
            real distance = qe.length();
            if (distance <= fabs(minDistance)) {
                minDistance = real(nonZeroSign(crossProduct(ab+t[i]*br, qe)))*distance;
                param = t[i];
            }
        }
    }

    if (param >= real(0) && param <= real(1))
        return SignedDistance(minDistance, 0);
    if (param < real(.5))
        return SignedDistance(minDistance, fabs(dotProduct(direction(0).normalize(), qa.normalize())));
    else
        return SignedDistance(minDistance, fabs(dotProduct(direction(1).normalize(), (p[2]-origin).normalize())));
}

SignedDistance CubicSegment::signedDistance(Point2 origin, real &param) const {
    Vector2 qa = p[0]-origin;
    Vector2 ab = p[1]-p[0];
    Vector2 br = p[2]-p[1]-ab;
    Vector2 as = (p[3]-p[2])-(p[2]-p[1])-br;

    Vector2 epDir = direction(0);
    real minDistance = real(nonZeroSign(crossProduct(epDir, qa)))*qa.length(); // distance from A
    param = -dotProduct(qa, epDir)/dotProduct(epDir, epDir);
    {
        epDir = direction(1);
        real distance = (p[3]-origin).length(); // distance from B
        if (distance < fabs(minDistance)) {
            minDistance = real(nonZeroSign(crossProduct(epDir, p[3]-origin)))*distance;
            param = dotProduct(epDir-(p[3]-origin), epDir)/dotProduct(epDir, epDir);
        }
    }
    // Iterative minimum distance search
    for (int i = 0; i <= MSDFGEN_CUBIC_SEARCH_STARTS; ++i) {
        real t = real(1)/real(MSDFGEN_CUBIC_SEARCH_STARTS)*real(i);
        Vector2 qe = qa+real(3)*t*ab+real(3)*t*t*br+t*t*t*as;
        for (int step = 0; step < MSDFGEN_CUBIC_SEARCH_STEPS; ++step) {
            // Improve t
            Vector2 d1 = real(3)*ab+real(6)*t*br+real(3)*t*t*as;
            Vector2 d2 = real(6)*br+real(6)*t*as;
            t -= dotProduct(qe, d1)/(dotProduct(d1, d1)+dotProduct(qe, d2));
            if (t <= real(0) || t >= real(1))
                break;
            qe = qa+real(3)*t*ab+real(3)*t*t*br+t*t*t*as;
            real distance = qe.length();
            if (distance < fabs(minDistance)) {
                minDistance = real(nonZeroSign(crossProduct(d1, qe)))*distance;
                param = t;
            }
        }
    }

    if (param >= real(0) && param <= real(1))
        return SignedDistance(minDistance, 0);
    if (param < real(.5))
        return SignedDistance(minDistance, fabs(dotProduct(direction(0).normalize(), qa.normalize())));
    else
        return SignedDistance(minDistance, fabs(dotProduct(direction(1).normalize(), (p[3]-origin).normalize())));
}

#endif

int LinearSegment::scanlineIntersections(real x[3], int dy[3], real y) const {
    if ((y >= p[0].y && y < p[1].y) || (y >= p[1].y && y < p[0].y)) {
        real param = (y-p[0].y)/(p[1].y-p[0].y);
        x[0] = mix(p[0].x, p[1].x, param);
        dy[0] = sign(p[1].y-p[0].y);
        return 1;
    }
    return 0;
}

int QuadraticSegment::scanlineIntersections(real x[3], int dy[3], real y) const {
    int total = 0;
    int nextDY = y > p[0].y ? 1 : -1;
    x[total] = p[0].x;
    if (p[0].y == y) {
        if (p[0].y < p[1].y || (p[0].y == p[1].y && p[0].y < p[2].y))
            dy[total++] = 1;
        else
            nextDY = 1;
    }
    {
        Vector2 ab = p[1]-p[0];
        Vector2 br = p[2]-p[1]-ab;
        real t[2];
        int solutions = solveQuadratic(t, br.y, 2*ab.y, p[0].y-y);
        // Sort solutions
        real tmp;
        if (solutions >= 2 && t[0] > t[1])
            tmp = t[0], t[0] = t[1], t[1] = tmp;
        for (int i = 0; i < solutions && total < 2; ++i) {
            if (t[i] >= 0 && t[i] <= 1) {
                x[total] = p[0].x+2*t[i]*ab.x+t[i]*t[i]*br.x;
                if (real(nextDY)*(ab.y+t[i]*br.y) >= real(0)) {
                    dy[total++] = nextDY;
                    nextDY = -nextDY;
                }
            }
        }
    }
    if (p[2].y == y) {
        if (nextDY > 0 && total > 0) {
            --total;
            nextDY = -1;
        }
        if ((p[2].y < p[1].y || (p[2].y == p[1].y && p[2].y < p[0].y)) && total < 2) {
            x[total] = p[2].x;
            if (nextDY < 0) {
                dy[total++] = -1;
                nextDY = 1;
            }
        }
    }
    if (nextDY != (y >= p[2].y ? 1 : -1)) {
        if (total > 0)
            --total;
        else {
            if (fabs(p[2].y-y) < fabs(p[0].y-y))
                x[total] = p[2].x;
            dy[total++] = nextDY;
        }
    }
    return total;
}

int CubicSegment::scanlineIntersections(real x[3], int dy[3], real y) const {
    int total = 0;
    int nextDY = y > p[0].y ? 1 : -1;
    x[total] = p[0].x;
    if (p[0].y == y) {
        if (p[0].y < p[1].y || (p[0].y == p[1].y && (p[0].y < p[2].y || (p[0].y == p[2].y && p[0].y < p[3].y))))
            dy[total++] = 1;
        else
            nextDY = 1;
    }
    {
        Vector2 ab = p[1]-p[0];
        Vector2 br = p[2]-p[1]-ab;
        Vector2 as = (p[3]-p[2])-(p[2]-p[1])-br;
        real t[3];
        int solutions = solveCubic(t, as.y, 3*br.y, 3*ab.y, p[0].y-y);
        // Sort solutions
        real tmp;
        if (solutions >= 2) {
            if (t[0] > t[1])
                tmp = t[0], t[0] = t[1], t[1] = tmp;
            if (solutions >= 3 && t[1] > t[2]) {
                tmp = t[1], t[1] = t[2], t[2] = tmp;
                if (t[0] > t[1])
                    tmp = t[0], t[0] = t[1], t[1] = tmp;
            }
        }
        for (int i = 0; i < solutions && total < 3; ++i) {
            if (t[i] >= 0 && t[i] <= 1) {
                x[total] = p[0].x+real(3)*t[i]*ab.x+real(3)*t[i]*t[i]*br.x+t[i]*t[i]*t[i]*as.x;
                if (real(nextDY)*(ab.y+real(2)*t[i]*br.y+t[i]*t[i]*as.y) >= real(0)) {
                    dy[total++] = nextDY;
                    nextDY = -nextDY;
                }
            }
        }
    }
    if (p[3].y == y) {
        if (nextDY > 0 && total > 0) {
            --total;
            nextDY = -1;
        }
        if ((p[3].y < p[2].y || (p[3].y == p[2].y && (p[3].y < p[1].y || (p[3].y == p[1].y && p[3].y < p[0].y)))) && total < 3) {
            x[total] = p[3].x;
            if (nextDY < 0) {
                dy[total++] = -1;
                nextDY = 1;
            }
        }
    }
    if (nextDY != (y >= p[3].y ? 1 : -1)) {
        if (total > 0)
            --total;
        else {
            if (fabs(p[3].y-y) < fabs(p[0].y-y))
                x[total] = p[3].x;
            dy[total++] = nextDY;
        }
    }
    return total;
}

static void pointBounds(Point2 p, real &l, real &b, real &r, real &t) {
    if (p.x < l) l = p.x;
    if (p.y < b) b = p.y;
    if (p.x > r) r = p.x;
    if (p.y > t) t = p.y;
}

void LinearSegment::bound(real &l, real &b, real &r, real &t) const {
    pointBounds(p[0], l, b, r, t);
    pointBounds(p[1], l, b, r, t);
}

void QuadraticSegment::bound(real &l, real &b, real &r, real &t) const {
    pointBounds(p[0], l, b, r, t);
    pointBounds(p[2], l, b, r, t);
    Vector2 bot = (p[1]-p[0])-(p[2]-p[1]);
    if (bot.x) {
        real param = (p[1].x-p[0].x)/bot.x;
        if (param > real(0) && param < real(1))
            pointBounds(point(param), l, b, r, t);
    }
    if (bot.y) {
        real param = (p[1].y-p[0].y)/bot.y;
        if (param > real(0) && param < real(1))
            pointBounds(point(param), l, b, r, t);
    }
}

void CubicSegment::bound(real &l, real &b, real &r, real &t) const {
    pointBounds(p[0], l, b, r, t);
    pointBounds(p[3], l, b, r, t);
    Vector2 a0 = p[1]-p[0];
    Vector2 a1 = real(2)*(p[2]-p[1]-a0);
    Vector2 a2 = p[3]-real(3)*p[2]+real(3)*p[1]-p[0];
    real params[2];
    int solutions;
    solutions = solveQuadratic(params, a2.x, a1.x, a0.x);
    for (int i = 0; i < solutions; ++i)
        if (params[i] > real(0) && params[i] < real(1))
            pointBounds(point(params[i]), l, b, r, t);
    solutions = solveQuadratic(params, a2.y, a1.y, a0.y);
    for (int i = 0; i < solutions; ++i)
        if (params[i] > real(0) && params[i] < real(1))
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
    if (dotProduct(origSDir, p[0]-p[1]) < real(0))
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
    if (dotProduct(origEDir, p[2]-p[1]) < real(0))
        p[1] = origP1;
}

void CubicSegment::moveEndPoint(Point2 to) {
    p[2] += to-p[3];
    p[3] = to;
}

void LinearSegment::splitInThirds(EdgeSegment *&part1, EdgeSegment *&part2, EdgeSegment *&part3) const {
    part1 = new LinearSegment(p[0], point(real(1)/real(3)), color);
    part2 = new LinearSegment(point(real(1)/real(3)), point(real(2)/real(3)), color);
    part3 = new LinearSegment(point(real(2)/real(3)), p[1], color);
}

void QuadraticSegment::splitInThirds(EdgeSegment *&part1, EdgeSegment *&part2, EdgeSegment *&part3) const {
    part1 = new QuadraticSegment(p[0], mix(p[0], p[1], real(1)/real(3)), point(real(1)/real(3)), color);
    part2 = new QuadraticSegment(point(real(1)/real(3)), mix(mix(p[0], p[1], real(5)/real(9)), mix(p[1], p[2], real(4)/real(9)), real(.5)), point(real(2)/real(3)), color);
    part3 = new QuadraticSegment(point(real(2)/real(3)), mix(p[1], p[2], real(2)/real(3)), p[2], color);
}

void CubicSegment::splitInThirds(EdgeSegment *&part1, EdgeSegment *&part2, EdgeSegment *&part3) const {
    part1 = new CubicSegment(p[0], p[0] == p[1] ? p[0] : mix(p[0], p[1], real(1)/real(3)), mix(mix(p[0], p[1], real(1)/real(3)), mix(p[1], p[2], real(1)/real(3)), real(1)/real(3)), point(real(1)/real(3)), color);
    part2 = new CubicSegment(point(real(1)/real(3)),
        mix(mix(mix(p[0], p[1], real(1)/real(3)), mix(p[1], p[2], real(1)/real(3)), real(1)/real(3)), mix(mix(p[1], p[2], real(1)/real(3)), mix(p[2], p[3], real(1)/real(3)), real(1)/real(3)), real(2)/real(3)),
        mix(mix(mix(p[0], p[1], real(2)/real(3)), mix(p[1], p[2], real(2)/real(3)), real(2)/real(3)), mix(mix(p[1], p[2], real(2)/real(3)), mix(p[2], p[3], real(2)/real(3)), real(2)/real(3)), real(1)/real(3)),
        point(real(2)/real(3)), color);
    part3 = new CubicSegment(point(real(2)/real(3)), mix(mix(p[1], p[2], real(2)/real(3)), mix(p[2], p[3], real(2)/real(3)), real(2)/real(3)), p[2] == p[3] ? p[3] : mix(p[2], p[3], real(2)/real(3)), p[3], color);
}

EdgeSegment *QuadraticSegment::convertToCubic() const {
    return new CubicSegment(p[0], mix(p[0], p[1], real(2)/real(3)), mix(p[1], p[2], real(1)/real(3)), p[2], color);
}

void CubicSegment::deconverge(int param, real amount) {
    Vector2 dir = direction(param);
    Vector2 normal = dir.getOrthonormal();
    real h = dotProduct(directionChange(param)-dir, normal);
    switch (param) {
        case 0:
            p[1] += amount*(dir+real(sign(h))*sqrt(fabs(h))*normal);
            break;
        case 1:
            p[2] -= amount*(dir-real(sign(h))*sqrt(fabs(h))*normal);
            break;
    }
}

}
