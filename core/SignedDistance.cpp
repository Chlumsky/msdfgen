
#include "SignedDistance.h"

#include <cmath>
#include "arithmetics.hpp"

namespace msdfgen {

const SignedDistance SignedDistance::INFINITE(-1e240, 1);

SignedDistance::SignedDistance() : sqDistance(-1e240), dot(1) { }

SignedDistance::SignedDistance(double sqDistance, double d) : sqDistance(sqDistance), dot(d) { }

double SignedDistance::distance() const {
    return sign(sqDistance)*sqrt(fabs(sqDistance));
}

bool operator<(SignedDistance a, SignedDistance b) {
    return fabs(a.sqDistance) < fabs(b.sqDistance) || (fabs(a.sqDistance) == fabs(b.sqDistance) && a.dot < b.dot);
}

bool operator>(SignedDistance a, SignedDistance b) {
    return fabs(a.sqDistance) > fabs(b.sqDistance) || (fabs(a.sqDistance) == fabs(b.sqDistance) && a.dot > b.dot);
}

bool operator<=(SignedDistance a, SignedDistance b) {
    return fabs(a.sqDistance) < fabs(b.sqDistance) || (fabs(a.sqDistance) == fabs(b.sqDistance) && a.dot <= b.dot);
}

bool operator>=(SignedDistance a, SignedDistance b) {
    return fabs(a.sqDistance) > fabs(b.sqDistance) || (fabs(a.sqDistance) == fabs(b.sqDistance) && a.dot >= b.dot);
}

}
