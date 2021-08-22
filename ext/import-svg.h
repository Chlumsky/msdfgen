
#pragma once

#include <cstdlib>
#include "../core/Shape.h"

namespace msdfgen {

/// Builds a shape from an SVG path string
bool buildShapeFromSvgPath(Shape &shape, const char *pathDef, double endpointSnapRange = 0);

/// Reads the first path found in the specified SVG file and stores it as a Shape in output.
bool loadSvgShape(Shape &output, const char *filename, int pathIndex = 0, Vector2 *dimensions = NULL);

}
