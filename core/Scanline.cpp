
#include "Scanline.h"

#include <algorithm>
#include "arithmetics.hpp"

namespace msdfgen {

static int compareIntersections(const void *a, const void *b) {
    return sign(reinterpret_cast<const Scanline::Intersection *>(a)->x-reinterpret_cast<const Scanline::Intersection *>(b)->x);
}

Scanline::Scanline() : lastIndex(0) { }

void Scanline::preprocess() {
    lastIndex = 0;
    if (!this->intersections.empty()) {
        qsort(&this->intersections[0], this->intersections.size(), sizeof(Intersection), compareIntersections);
        int totalDirection = 0;
        for (std::vector<Intersection>::iterator intersection = intersections.begin(); intersection != intersections.end(); ++intersection) {
            totalDirection += intersection->direction;
            intersection->direction = totalDirection;
        }
    }
}

void Scanline::setIntersections(const std::vector<Intersection> &intersections) {
    this->intersections = intersections;
    preprocess();
}

#ifdef MSDFGEN_USE_CPP11
void Scanline::setIntersections(std::vector<Intersection> &&intersections) {
    this->intersections = (std::vector<Intersection> &&) intersections;
    preprocess();
}
#endif

int Scanline::moveTo(double x) const {
    if (intersections.empty())
        return -1;
    int index = lastIndex;
    if (x < intersections[index].x) {
        do {
            if (index == 0) {
                lastIndex = 0;
                return -1;
            }
            --index;
        } while (x < intersections[index].x);
    } else {
        while (index < (int) intersections.size()-1 && x >= intersections[index+1].x)
            ++index;
    }
    lastIndex = index;
    return index;
}

int Scanline::countIntersections(double x) const {
    return moveTo(x)+1;
}

int Scanline::sumIntersections(double x) const {
    int index = moveTo(x);
    if (index >= 0)
        return intersections[index].direction;
    return 0;
}

}
