
#include "edge-coloring.h"

namespace msdfgen {

static bool isCorner(const Vector2 &aDir, const Vector2 &bDir, double crossThreshold) {
    return dotProduct(aDir, bDir) <= 0 || fabs(crossProduct(aDir, bDir)) > crossThreshold;
}

void edgeColoringSimple(Shape &shape, double angleThreshold) {
    double crossThreshold = sin(angleThreshold);
    std::vector<int> corners;
    for (std::vector<Contour>::iterator contour = shape.contours.begin(); contour != shape.contours.end(); ++contour) {
        // Identify corners
        corners.clear();
        if (!contour->edges.empty()) {
            Vector2 prevDirection = (*(contour->edges.end()-1))->direction(1);
            int index = 0;
            for (std::vector<EdgeHolder>::const_iterator edge = contour->edges.begin(); edge != contour->edges.end(); ++edge, ++index) {
                if (isCorner(prevDirection.normalize(), (*edge)->direction(0).normalize(), crossThreshold))
                    corners.push_back(index);
                prevDirection = (*edge)->direction(1);
            }
        }

        // Smooth contour
        if (corners.empty())
            for (std::vector<EdgeHolder>::iterator edge = contour->edges.begin(); edge != contour->edges.end(); ++edge)
                (*edge)->color = WHITE;
        // "Teardrop" case
        else if (corners.size() == 1) {
            const EdgeColor colors[] = { MAGENTA, WHITE, YELLOW };
            int corner = corners[0];
            if (contour->edges.size() >= 3) {
                int m = contour->edges.size();
                for (int i = 0; i < m; ++i)
                    contour->edges[(corner+i)%m]->color = (colors+1)[int(3+2.875*i/(m-1)-1.4375+.5)-3];
            } else if (contour->edges.size() >= 1) {
                // Less than three edge segments for three colors => edges must be split
                EdgeSegment *parts[7] = { };
                contour->edges[0]->splitInThirds(parts[0+3*corner], parts[1+3*corner], parts[2+3*corner]);
                if (contour->edges.size() >= 2) {
                    contour->edges[1]->splitInThirds(parts[3-3*corner], parts[4-3*corner], parts[5-3*corner]);
                    parts[0]->color = parts[1]->color = colors[0];
                    parts[2]->color = parts[3]->color = colors[1];
                    parts[4]->color = parts[5]->color = colors[2];
                } else {
                    parts[0]->color = colors[0];
                    parts[1]->color = colors[1];
                    parts[2]->color = colors[2];
                }
                contour->edges.clear();
                for (int i = 0; parts[i]; ++i)
                    contour->edges.push_back(EdgeHolder(parts[i]));
            }
        }
        // Multiple corners
        else {
            int cornerCount = corners.size();
            // CMYCMYCMYCMY / YMYCMYC if corner count % 3 == 1
            EdgeColor colors[] = { cornerCount%3 == 1 ? YELLOW : CYAN, CYAN, MAGENTA, YELLOW };
            int spline = 0;
            int start = corners[0];
            int m = contour->edges.size();
            for (int i = 0; i < m; ++i) {
                int index = (start+i)%m;
                if (cornerCount > spline+1 && corners[spline+1] == index)
                    ++spline;
                contour->edges[index]->color = (colors+1)[spline%3-!spline];
            }
        }
    }
}

}
