
## Version 1.12 (2024-05-18)

- Added the possibility to specify asymmetrical distance range (`-arange`, `-apxrange`)
- Added the ability to export the shape into an SVG file (`-exportsvg`)
- Edge coloring no longer colors smooth contours white, which has been found suboptimal
- Fixed incorrect scaling of font glyph coordinates. To preserve backwards compatibility, the user has to enable the fix with an explicit additional argument:
    - `-emnormalize` in standalone, `FONT_SCALING_EM_NORMALIZED` in API for coordinates in ems
    - `-noemnormalize` in standalone, `FONT_SCALING_NONE` in API for raw integer coordinates
    - The default (backwards-compatible) behavior will change in a future version; a warning will be displayed if neither option is set
- Added two new developer-friendly export image formats: RGBA and FL32
- `-size` parameter renamed to `-dimensions` for clarity (old one will be kept for compatibility)
- `generate*SDF` functions now combine projection and range into a single argument (`SDFTransformation`)
- Conversion of floating point color values to 8-bit integers adjusted to match graphics hardware
- Improved edge deconvergence procedure and made sure that calling `Shape::normalize` a second time has no effect
- Fixed certain edge cases where Skia geometry preprocessing wouldn't make the geometry fully compliant
- The term "perpendicular distance" now used instead of "pseudo-distance" (PSDF instead of PseudoSDF in API)
- Fixed a bug in `savePng` where `fclose` could be called on null pointer
- Minor code improvements

## Version 1.11 (2023-11-11)

- Reworked SVG parser, which now supports multiple paths and other shapes - requires Skia
- Major performance improvements due to inlining certain low-level classes
- A limited version of the standalone executable can now be built without any dependencies
- Fixed `listFontVariationAxes` which previously reported incorrectly scaled values
- Fixed potential crash when generating SDF from an empty `Shape`
- Fixed a small bug in the error correction routine
- Errors now reported to `stderr` rather than `stdout`
- All command line arguments can now also be passed with two dashes instead of one
- Added `-version` argument to print the program's version
- `Shape` can now be loaded from a pointer to FreeType's `FT_Outline`
- Added hidden CMake options to selectively disable PNG, SVG, or variable font support
- Added CMake presets
- Other minor bug fixes

## Version 1.10 (2023-01-15)

- Switched to vcpkg as the primary dependency management system
- Switched to libpng as the primary PNG file encoder
- Parameters of variable fonts can be specified
- Fixed a bug that prevented glyph 0 to be specified in a glyphset

### Version 1.9.2 (2021-12-01)

- Improved detection of numerical errors in cubic equation solver
- Added -windingpreprocess option
- Fixed edge coloring not restored if lost during preprocessing

### Version 1.9.1 (2021-07-09)

- Fixed an edge case bug in the new MSDF error correction algorithm

## Version 1.9 (2021-05-28)

- Error correction of multi-channel distance fields has been completely reworked
- Added new edge coloring strategy that optimizes colors based on distances between edges
- Added some minor functions for the library API
- Minor code refactor and optimizations

## Version 1.8 (2020-10-17)

- Integrated the Skia library into the project, which is used to preprocess the shape geometry and eliminate any self-intersections and other irregularities previously unsupported by the software
    - The scanline pass and overlapping contour mode is made obsolete by this step and has been disabled by default. The preprocess step can be disabled by the new `-nopreprocess` switch and the former enabled by `-scanline` and `-overlap` respectively.
    - The project can be built without the Skia library, forgoing the geometry preprocessing feature. This is controlled by the macro definition `MSDFGEN_USE_SKIA`
- Significantly improved performance of the core algorithm by reusing results from previously computed pixels
- Introduced an additional error correction routine which eliminates MSDF artifacts by analytically predicting results of bilinear interpolation
- Added the possibility to load font glyphs by their index rather than a Unicode value (use the prefix `g` before the character code in `-font` argument)
- Added `-distanceshift` argument that can be used to adjust the center of the distance range in the output distance field
- Fixed several errors in the evaluation of curve distances
- Fixed an issue with paths containing convergent corners (those whose inner angle is zero)
- The algorithm for pseudo-distance computation slightly changed, fixing certain rare edge cases and improving consistency
- Added the ability to supply own `FT_Face` handle to the msdfgen library
- Minor refactor of the core algorithm

### Version 1.7.1 (2020-03-09)

- Fixed an edge case bug in scanline rasterization

## Version 1.7 (2020-03-07)

- Added `mtsdf` mode - a combination of `msdf` with `sdf` in the alpha channel
- Distance fields can now be stored as uncompressed TIFF image files with floating point precision
- Bitmap class refactor - template argument split into data type and number of channels, bitmap reference classes introduced
- Added a secondary "ink trap" edge coloring heuristic, can be selected using `-coloringstrategy inktrap`
- Added computation of estimated rendering error for a given SDF
- Added computation of bounding box that includes sharp mitered corners
- The API for bounds computation of the `Shape` class changed for clarity
- Fixed several edge case bugs

## Version 1.6 (2019-04-08)

- Core algorithm rewritten to split up advanced edge selection logic into modular template arguments.
- Pseudo-distance evaluation reworked to eliminate discontinuities at the midpoint between edges.
- MSDF error correction reworked to also fix distances away from edges and consider diagonal pairs. Code simplified.
- Added scanline rasterization support for `Shape`.
- Added a scanline pass in the standalone version, which corrects the signs in the distance field according to the selected fill rule (`-fillrule`). Can be disabled using `-noscanline`.
- Fixed autoframe scaling, which previously caused the output to have unnecessary empty border.
- `-guessorder` switch no longer enabled by default, as the functionality is now provided by the scanline pass.
- Updated FreeType and other libraries, changed to static linkage
- Added 64-bit and static library builds to the Visual Studio solution

## Version 1.5 (2017-07-23)

- Fixed rounding error in cubic curve splitting.
- SVG parser fixes and support for additional path commands.
- Added CMake build script.

## Version 1.4 (2017-02-09)

- Reworked contour combining logic to support overlapping contours. Original algorithm preserved in functions with `_legacy` suffix, which are invoked by the new `-legacy` switch.
- Fixed a severe bug in cubic curve distance computation, where a control point lies at the endpoint.
- Standalone version now automatically detects if the input has the wrong orientation and adjusts the distance field accordingly. Can be disabled by `-keeporder` or `-reverseorder` switch.
- SVG parser fixes and improvements.

## Version 1.3 (2016-12-07)

- Fixed `-reverseorder` switch.
- Fixed glyph loading to use the proper method of acquiring outlines from FreeType.

## Version 1.2 (2016-07-20)

- Added option to specify that shape vertices are listed in reverse order (`-reverseorder`).
- Added option to set a seed for the edge coloring heuristic (-seed \<n\>), which can be used to adjust the output.
- Fixed parsing of glyph contours that start with a curve control point.

## Version 1.1 (2016-05-08)

- Switched to MIT license due to popular demand.
- Fixed SDF rendering anti-aliasing when the output is smaller than the distance field.

## Version 1.0 (2016-04-28)

- Project published.
