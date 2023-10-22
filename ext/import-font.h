
#pragma once

#include <cstddef>
#include "../core/types.h"
#include "../core/Shape.h"

namespace msdfgen {

class FreetypeHandle;
class FontHandle;

class GlyphIndex {

public:
    explicit GlyphIndex(unsigned index = 0);
    unsigned getIndex() const;

private:
    unsigned index;

};

/// Global metrics of a typeface (in font units).
struct FontMetrics {
    /// The size of one EM.
    real emSize;
    /// The vertical position of the ascender and descender relative to the baseline.
    real ascenderY, descenderY;
    /// The vertical difference between consecutive baselines.
    real lineHeight;
    /// The vertical position and thickness of the underline.
    real underlineY, underlineThickness;
};

/// A structure to model a given axis of a variable font.
struct FontVariationAxis {
    /// The name of the variation axis.
    const char *name;
    /// The axis's minimum coordinate value.
    real minValue;
    /// The axis's maximum coordinate value.
    real maxValue;
    /// The axis's default coordinate value. FreeType computes meaningful default values for Adobe MM fonts.
    real defaultValue;
};

/// Initializes the FreeType library.
FreetypeHandle *initializeFreetype();
/// Deinitializes the FreeType library.
void deinitializeFreetype(FreetypeHandle *library);

#ifdef FT_LOAD_DEFAULT // FreeType included
/// Creates a FontHandle from FT_Face that was loaded by the user. destroyFont must still be called but will not affect the FT_Face.
FontHandle *adoptFreetypeFont(FT_Face ftFace);
/// Converts the geometry of FreeType's FT_Outline to a Shape object.
FT_Error readFreetypeOutline(Shape &output, FT_Outline *outline);
#endif

/// Loads a font file and returns its handle.
FontHandle *loadFont(FreetypeHandle *library, const char *filename);
/// Loads a font from binary data and returns its handle.
FontHandle *loadFontData(FreetypeHandle *library, const byte *data, int length);
/// Unloads a font file.
void destroyFont(FontHandle *font);
/// Outputs the metrics of a font file.
bool getFontMetrics(FontMetrics &metrics, FontHandle *font);
/// Outputs the width of the space and tab characters.
bool getFontWhitespaceWidth(real &spaceAdvance, real &tabAdvance, FontHandle *font);
/// Outputs the glyph index corresponding to the specified Unicode character.
bool getGlyphIndex(GlyphIndex &glyphIndex, FontHandle *font, unicode_t unicode);
/// Loads the geometry of a glyph from a font file.
bool loadGlyph(Shape &output, FontHandle *font, GlyphIndex glyphIndex, real *advance = NULL);
bool loadGlyph(Shape &output, FontHandle *font, unicode_t unicode, real *advance = NULL);
/// Outputs the kerning distance adjustment between two specific glyphs.
bool getKerning(real &output, FontHandle *font, GlyphIndex glyphIndex1, GlyphIndex glyphIndex2);
bool getKerning(real &output, FontHandle *font, unicode_t unicode1, unicode_t unicode2);

#ifndef MSDFGEN_DISABLE_VARIABLE_FONTS
/// Sets a single variation axis of a variable font.
bool setFontVariationAxis(FreetypeHandle *library, FontHandle *font, const char *name, real coordinate);
/// Lists names and ranges of variation axes of a variable font.
bool listFontVariationAxes(std::vector<FontVariationAxis> &axes, FreetypeHandle *library, FontHandle *font);
#endif

}
