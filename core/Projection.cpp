
#include "Projection.h"

namespace msdfgen {

Projection::Projection() : scale(1), translate(0) { }

Projection::Projection(const Vector2 &scale, const Vector2 &translate) : scale(scale), translate(translate) { }

Point2 Projection::project(const Point2 &coord) const {
    return scale*(coord+translate);
}

Point2 Projection::unproject(const Point2 &coord) const {
    return coord/scale-translate;
}

Vector2 Projection::projectVector(const Vector2 &vector) const {
    return scale*vector;
}

Vector2 Projection::unprojectVector(const Vector2 &vector) const {
    return vector/scale;
}

real Projection::projectX(real x) const {
    return scale.x*(x+translate.x);
}

real Projection::projectY(real y) const {
    return scale.y*(y+translate.y);
}

real Projection::unprojectX(real x) const {
    return x/scale.x-translate.x;
}

real Projection::unprojectY(real y) const {
    return y/scale.y-translate.y;
}

}
