
#pragma once

#include <cmath>
#include <cfloat>

namespace msdfgen {

/// Represents a signed distance and alignment, which together can be compared to uniquely determine the closest edge segment.
class SignedDistance {

public:
    real distance;
    real dot;

    inline SignedDistance() : distance(-FLT_MAX), dot(0) { }
    inline SignedDistance(real dist, real d) : distance(dist), dot(d) { }

};

inline bool operator<(const SignedDistance a, const SignedDistance b) {
    return fabs(a.distance) < fabs(b.distance) || (fabs(a.distance) == fabs(b.distance) && a.dot < b.dot);
}

inline bool operator>(const SignedDistance a, const SignedDistance b) {
    return fabs(a.distance) > fabs(b.distance) || (fabs(a.distance) == fabs(b.distance) && a.dot > b.dot);
}

inline bool operator<=(const SignedDistance a, const SignedDistance b) {
    return fabs(a.distance) < fabs(b.distance) || (fabs(a.distance) == fabs(b.distance) && a.dot <= b.dot);
}

inline bool operator>=(const SignedDistance a, const SignedDistance b) {
    return fabs(a.distance) > fabs(b.distance) || (fabs(a.distance) == fabs(b.distance) && a.dot >= b.dot);
}

}
