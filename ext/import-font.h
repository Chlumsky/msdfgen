
#pragma once

#include <cstdlib>
#include "../core/Shape.h"

namespace msdfgen {

typedef unsigned char byte;
typedef unsigned unicode_t;

class FreetypeHandle;
class FontHandle;

class GlyphIndex {

public:
    explicit GlyphIndex(unsigned index = 0);
    unsigned getIndex() const;
    bool operator!() const;

private:
    unsigned index;

};

/// Global metrics of a typeface (in font units).
struct FontMetrics {
    /// The size of one EM.
    double emSize;
    /// The vertical position of the ascender and descender relative to the baseline.
    double ascenderY, descenderY;
    /// The vertical difference between consecutive baselines.
    double lineHeight;
    /// The vertical position and thickness of the underline.
    double underlineY, underlineThickness;
};

/// Initializes the FreeType library.
FreetypeHandle * initializeFreetype();
/// Deinitializes the FreeType library.
void deinitializeFreetype(FreetypeHandle *library);

#ifdef FT_FREETYPE_H
/// Creates a FontHandle from FT_Face that was loaded by the user. destroyFont must still be called but will not affect the FT_Face.
FontHandle * adoptFreetypeFont(FT_Face ftFace);
#endif
/// Loads a font file and returns its handle.
FontHandle * loadFont(FreetypeHandle *library, const char *filename);
/// Loads a font from binary data and returns its handle.
FontHandle * loadFontData(FreetypeHandle *library, const byte *data, int length);
/// Unloads a font file.
void destroyFont(FontHandle *font);
/// Outputs the metrics of a font file.
bool getFontMetrics(FontMetrics &metrics, FontHandle *font);
/// Outputs the width of the space and tab characters.
bool getFontWhitespaceWidth(double &spaceAdvance, double &tabAdvance, FontHandle *font);
/// Outputs the glyph index corresponding to the specified Unicode character.
bool getGlyphIndex(GlyphIndex &glyphIndex, FontHandle *font, unicode_t unicode);
/// Loads the geometry of a glyph from a font file.
bool loadGlyph(Shape &output, FontHandle *font, GlyphIndex glyphIndex, double *advance = NULL);
bool loadGlyph(Shape &output, FontHandle *font, unicode_t unicode, double *advance = NULL);
/// Outputs the kerning distance adjustment between two specific glyphs.
bool getKerning(double &output, FontHandle *font, GlyphIndex glyphIndex1, GlyphIndex glyphIndex2);
bool getKerning(double &output, FontHandle *font, unicode_t unicode1, unicode_t unicode2);

}
