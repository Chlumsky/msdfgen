
#pragma once

namespace msdfgen {

/// Represents a signed squared distance and alignment, which together can be compared to uniquely determine the closest edge segment.
class SignedDistance {

public:
    static const SignedDistance INFINITE;

    double sqDistance;
    double dot;

    SignedDistance();
    SignedDistance(double sqDistance, double d);
    double distance() const;

    friend bool operator<(SignedDistance a, SignedDistance b);
    friend bool operator>(SignedDistance a, SignedDistance b);
    friend bool operator<=(SignedDistance a, SignedDistance b);
    friend bool operator>=(SignedDistance a, SignedDistance b);

};

}
