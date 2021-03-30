
#pragma once

#include "Vector2.h"
#include "Shape.h"
#include "Projection.h"
#include "BitmapRef.hpp"
#include "generator-config.h"

namespace msdfgen {

void msdfPatchArtifacts(const BitmapRef<float, 3> &sdf, const Shape &shape, const Projection &projection, double range, const GeneratorConfig &generatorConfig = GeneratorConfig(), const ArtifactPatcherConfig &config = ArtifactPatcherConfig());
void msdfPatchArtifacts(const BitmapRef<float, 4> &sdf, const Shape &shape, const Projection &projection, double range, const GeneratorConfig &generatorConfig = GeneratorConfig(), const ArtifactPatcherConfig &config = ArtifactPatcherConfig());

}
