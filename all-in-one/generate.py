#!/usr/bin/env python3

import os
import re

rootDir = os.path.join(os.path.dirname(__file__), '..')

sourceList = [
    'core/arithmetics.hpp',
    'core/equation-solver.h',
    'core/equation-solver.cpp',
    'core/Vector2.h',
    'core/Vector2.cpp',
    'core/pixel-conversion.hpp',
    'core/BitmapRef.hpp',
    'core/Bitmap.h',
    'core/Bitmap.hpp',
    'core/Projection.h',
    'core/Projection.cpp',
    'core/SignedDistance.h',
    'core/SignedDistance.cpp',
    'core/Scanline.h',
    'core/Scanline.cpp',
    'core/EdgeColor.h',
    'core/edge-segments.h',
    'core/edge-segments.cpp',
    'core/EdgeHolder.h',
    'core/EdgeHolder.cpp',
    'core/Contour.h',
    'core/Contour.cpp',
    'core/Shape.h',
    'core/Shape.cpp',
    'core/bitmap-interpolation.hpp',
    'core/edge-coloring.h',
    'core/edge-coloring.cpp',
    'core/edge-selectors.h',
    'core/edge-selectors.cpp',
    'core/contour-combiners.h',
    'core/contour-combiners.cpp',
    'core/ShapeDistanceFinder.h',
    'core/ShapeDistanceFinder.hpp',
    'core/generator-config.h',
    'core/msdf-error-correction.h',
    'core/msdf-error-correction.cpp',
    'core/MSDFErrorCorrection.h',
    'core/MSDFErrorCorrection.cpp',
    'core/msdfgen.cpp',
    'ext/import-font.h',
    'ext/import-font.cpp',
    'ext/resolve-shape-geometry.h',
    'ext/resolve-shape-geometry.cpp',
    'msdfgen.h'
]

header = """
#pragma once

#define MSDFGEN_USE_CPP11
#define MSDFGEN_USE_FREETYPE

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
"""

source = """
#include "msdfgen.h"

#include <algorithm>
#include <queue>

#ifdef MSDFGEN_USE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#ifndef MSDFGEN_DISABLE_VARIABLE_FONTS
#include FT_MULTIPLE_MASTERS_H
#endif
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4456 4458)
#endif

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif
"""

with open(os.path.join(rootDir, 'LICENSE.txt'), 'r') as file:
    license = file.read()
license = '\n'.join([' * '+line for line in license.strip().split('\n')])

for filename in sourceList:
    with open(os.path.join(rootDir, *filename.split('/')), 'r') as file:
        src = file.read()
    src = '\n'.join([line for line in src.split('\n') if not re.match(r'^\s*#(include\s.*|pragma\s+once)\s*$', line)])
    if filename.startswith('ext/import-font.'):
        src = '#ifdef MSDFGEN_USE_FREETYPE\n\n'+src+'\n\n#endif\n\n'
    if filename.endswith('.h') or filename.endswith('.hpp'):
        header += '\n\n'+src
    if filename.endswith('.cpp'):
        source += '\n\n'+src

header = '\n'+re.sub(r'\n{3,}', '\n\n', re.sub(r'}\s*namespace\s+msdfgen\s*{', '', re.sub(r'\/\*[^\*].*?\*\/', '', header, flags=re.DOTALL))).strip()+'\n'
source = '\n'+re.sub(r'\n{3,}', '\n\n', re.sub(r'}\s*namespace\s+msdfgen\s*{', '', re.sub(r'\/\*[^\*].*?\*\/', '', source, flags=re.DOTALL))).strip()+'\n'

header = """
/*
 * MULTI-CHANNEL SIGNED DISTANCE FIELD GENERATOR
 * ---------------------------------------------
 * A utility by Viktor Chlumsky, (c) 2014 - 2023
 * https://github.com/Chlumsky/msdfgen
 * Published under the MIT license
 *
 * The technique used to generate multi-channel distance fields in this code
 * was developed by Viktor Chlumsky in 2014 for his master's thesis,
 * "Shape Decomposition for Multi-Channel Distance Fields". It provides improved
 * quality of sharp corners in glyphs and other 2D shapes compared to monochrome
 * distance fields. To reconstruct an image of the shape, apply the median of three
 * operation on the triplet of sampled signed distance values.
 *
 */
"""+header

source = """
/*
 * MULTI-CHANNEL SIGNED DISTANCE FIELD GENERATOR
 * ---------------------------------------------
 * https://github.com/Chlumsky/msdfgen
 *
"""+license+"""
 *
 */
"""+source

with open(os.path.join(os.path.dirname(__file__), 'msdfgen.h'), 'w') as file:
    file.write(header)
with open(os.path.join(os.path.dirname(__file__), 'msdfgen.cpp'), 'w') as file:
    file.write(source)
