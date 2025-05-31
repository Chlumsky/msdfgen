
#pragma once

/*
 * MULTI-CHANNEL SIGNED DISTANCE FIELD GENERATOR
 * ---------------------------------------------
 * A utility by Viktor Chlumsky, (c) 2014 - 2025
 *
 * The extension module provides ways to easily load input and save output using popular formats.
 *
 * Third party dependencies in extension module:
 * - Skia by Google
 *   (to resolve self-intersecting paths)
 * - FreeType 2
 *   (to load input font files)
 * - TinyXML 2 by Lee Thomason
 *   (to aid in parsing input SVG files)
 * - libpng by the PNG Development Group
 * - or LodePNG by Lode Vandevenne
 *   (to save output PNG images)
 *
 */

#include "ext/resolve-shape-geometry.h"
#include "ext/save-png.h"
#include "ext/import-svg.h"
#include "ext/import-font.h"
