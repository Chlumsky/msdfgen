
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
- Fixed glyph loading to use the proper method of acquiring the outlines from FreeType.

## Version 1.2 (2016-07-20)

- Added option to specify that shape vertices are listed in reverse order (`-reverseorder`).
- Added option to set a seed for the edge coloring heuristic (-seed \<n\>), which can be used to adjust the output.
- Fixed parsing of glyph contours starting that start with a curve control point.

## Version 1.1 (2016-05-08)

- Switched to MIT license due to popular demand.
- Fixed SDF rendering anti-aliasing when the output is smaller than the distance field.

## Version 1.0 (2016-04-28)

- Project published.
