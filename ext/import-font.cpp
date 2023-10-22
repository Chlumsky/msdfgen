
#include "import-font.h"

#include <cstring>
#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#ifndef MSDFGEN_DISABLE_VARIABLE_FONTS
#include FT_MULTIPLE_MASTERS_H
#endif

namespace msdfgen {

#define F26DOT6_TO_REAL(x) (::msdfgen::real(1)/::msdfgen::real(64)*::msdfgen::real(x))
#define F16DOT16_TO_REAL(x) (::msdfgen::real(1)/::msdfgen::real(65536)*::msdfgen::real(x))
#define REAL_TO_F16DOT16(x) FT_Fixed(::msdfgen::real(65536)*::msdfgen::real(x))

class FreetypeHandle {
    friend FreetypeHandle *initializeFreetype();
    friend void deinitializeFreetype(FreetypeHandle *library);
    friend FontHandle *loadFont(FreetypeHandle *library, const char *filename);
    friend FontHandle *loadFontData(FreetypeHandle *library, const byte *data, int length);
#ifndef MSDFGEN_DISABLE_VARIABLE_FONTS
    friend bool setFontVariationAxis(FreetypeHandle *library, FontHandle *font, const char *name, real coordinate);
    friend bool listFontVariationAxes(std::vector<FontVariationAxis> &axes, FreetypeHandle *library, FontHandle *font);
#endif

    FT_Library library;

};

class FontHandle {
    friend FontHandle *adoptFreetypeFont(FT_Face ftFace);
    friend FontHandle *loadFont(FreetypeHandle *library, const char *filename);
    friend FontHandle *loadFontData(FreetypeHandle *library, const byte *data, int length);
    friend void destroyFont(FontHandle *font);
    friend bool getFontMetrics(FontMetrics &metrics, FontHandle *font);
    friend bool getFontWhitespaceWidth(real &spaceAdvance, real &tabAdvance, FontHandle *font);
    friend bool getGlyphIndex(GlyphIndex &glyphIndex, FontHandle *font, unicode_t unicode);
    friend bool loadGlyph(Shape &output, FontHandle *font, GlyphIndex glyphIndex, real *advance);
    friend bool loadGlyph(Shape &output, FontHandle *font, unicode_t unicode, real *advance);
    friend bool getKerning(real &output, FontHandle *font, GlyphIndex glyphIndex1, GlyphIndex glyphIndex2);
    friend bool getKerning(real &output, FontHandle *font, unicode_t unicode1, unicode_t unicode2);
#ifndef MSDFGEN_DISABLE_VARIABLE_FONTS
    friend bool setFontVariationAxis(FreetypeHandle *library, FontHandle *font, const char *name, real coordinate);
    friend bool listFontVariationAxes(std::vector<FontVariationAxis> &axes, FreetypeHandle *library, FontHandle *font);
#endif

    FT_Face face;
    bool ownership;

};

struct FtContext {
    Point2 position;
    Shape *shape;
    Contour *contour;
};

static Point2 ftPoint2(const FT_Vector &vector) {
    return Point2(F26DOT6_TO_REAL(vector.x), F26DOT6_TO_REAL(vector.y));
}

static int ftMoveTo(const FT_Vector *to, void *user) {
    FtContext *context = reinterpret_cast<FtContext *>(user);
    if (!(context->contour && context->contour->edges.empty()))
        context->contour = &context->shape->addContour();
    context->position = ftPoint2(*to);
    return 0;
}

static int ftLineTo(const FT_Vector *to, void *user) {
    FtContext *context = reinterpret_cast<FtContext *>(user);
    Point2 endpoint = ftPoint2(*to);
    if (endpoint != context->position) {
        context->contour->addEdge(new LinearSegment(context->position, endpoint));
        context->position = endpoint;
    }
    return 0;
}

static int ftConicTo(const FT_Vector *control, const FT_Vector *to, void *user) {
    FtContext *context = reinterpret_cast<FtContext *>(user);
    context->contour->addEdge(new QuadraticSegment(context->position, ftPoint2(*control), ftPoint2(*to)));
    context->position = ftPoint2(*to);
    return 0;
}

static int ftCubicTo(const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to, void *user) {
    FtContext *context = reinterpret_cast<FtContext *>(user);
    context->contour->addEdge(new CubicSegment(context->position, ftPoint2(*control1), ftPoint2(*control2), ftPoint2(*to)));
    context->position = ftPoint2(*to);
    return 0;
}

GlyphIndex::GlyphIndex(unsigned index) : index(index) { }

unsigned GlyphIndex::getIndex() const {
    return index;
}

FreetypeHandle *initializeFreetype() {
    FreetypeHandle *handle = new FreetypeHandle;
    FT_Error error = FT_Init_FreeType(&handle->library);
    if (error) {
        delete handle;
        return NULL;
    }
    return handle;
}

void deinitializeFreetype(FreetypeHandle *library) {
    FT_Done_FreeType(library->library);
    delete library;
}

FontHandle *adoptFreetypeFont(FT_Face ftFace) {
    FontHandle *handle = new FontHandle;
    handle->face = ftFace;
    handle->ownership = false;
    return handle;
}

FT_Error readFreetypeOutline(Shape &output, FT_Outline *outline) {
    output.contours.clear();
    output.inverseYAxis = false;
    FtContext context = { };
    context.shape = &output;
    FT_Outline_Funcs ftFunctions;
    ftFunctions.move_to = &ftMoveTo;
    ftFunctions.line_to = &ftLineTo;
    ftFunctions.conic_to = &ftConicTo;
    ftFunctions.cubic_to = &ftCubicTo;
    ftFunctions.shift = 0;
    ftFunctions.delta = 0;
    FT_Error error = FT_Outline_Decompose(outline, &ftFunctions, &context);
    if (!output.contours.empty() && output.contours.back().edges.empty())
        output.contours.pop_back();
    return error;
}

FontHandle *loadFont(FreetypeHandle *library, const char *filename) {
    if (!library)
        return NULL;
    FontHandle *handle = new FontHandle;
    FT_Error error = FT_New_Face(library->library, filename, 0, &handle->face);
    if (error) {
        delete handle;
        return NULL;
    }
    handle->ownership = true;
    return handle;
}

FontHandle *loadFontData(FreetypeHandle *library, const byte *data, int length) {
    if (!library)
        return NULL;
    FontHandle *handle = new FontHandle;
    FT_Error error = FT_New_Memory_Face(library->library, data, length, 0, &handle->face);
    if (error) {
        delete handle;
        return NULL;
    }
    handle->ownership = true;
    return handle;
}

void destroyFont(FontHandle *font) {
    if (font->ownership)
        FT_Done_Face(font->face);
    delete font;
}

bool getFontMetrics(FontMetrics &metrics, FontHandle *font) {
    metrics.emSize = F26DOT6_TO_REAL(font->face->units_per_EM);
    metrics.ascenderY = F26DOT6_TO_REAL(font->face->ascender);
    metrics.descenderY = F26DOT6_TO_REAL(font->face->descender);
    metrics.lineHeight = F26DOT6_TO_REAL(font->face->height);
    metrics.underlineY = F26DOT6_TO_REAL(font->face->underline_position);
    metrics.underlineThickness = F26DOT6_TO_REAL(font->face->underline_thickness);
    return true;
}

bool getFontWhitespaceWidth(real &spaceAdvance, real &tabAdvance, FontHandle *font) {
    FT_Error error = FT_Load_Char(font->face, ' ', FT_LOAD_NO_SCALE);
    if (error)
        return false;
    spaceAdvance = F26DOT6_TO_REAL(font->face->glyph->advance.x);
    error = FT_Load_Char(font->face, '\t', FT_LOAD_NO_SCALE);
    if (error)
        return false;
    tabAdvance = F26DOT6_TO_REAL(font->face->glyph->advance.x);
    return true;
}

bool getGlyphIndex(GlyphIndex &glyphIndex, FontHandle *font, unicode_t unicode) {
    glyphIndex = GlyphIndex(FT_Get_Char_Index(font->face, unicode));
    return glyphIndex.getIndex() != 0;
}

bool loadGlyph(Shape &output, FontHandle *font, GlyphIndex glyphIndex, real *advance) {
    if (!font)
        return false;
    FT_Error error = FT_Load_Glyph(font->face, glyphIndex.getIndex(), FT_LOAD_NO_SCALE);
    if (error)
        return false;
    if (advance)
        *advance = F26DOT6_TO_REAL(font->face->glyph->advance.x);
    return !readFreetypeOutline(output, &font->face->glyph->outline);
}

bool loadGlyph(Shape &output, FontHandle *font, unicode_t unicode, real *advance) {
    return loadGlyph(output, font, GlyphIndex(FT_Get_Char_Index(font->face, unicode)), advance);
}

bool getKerning(real &output, FontHandle *font, GlyphIndex glyphIndex1, GlyphIndex glyphIndex2) {
    FT_Vector kerning;
    if (FT_Get_Kerning(font->face, glyphIndex1.getIndex(), glyphIndex2.getIndex(), FT_KERNING_UNSCALED, &kerning)) {
        output = 0;
        return false;
    }
    output = F26DOT6_TO_REAL(kerning.x);
    return true;
}

bool getKerning(real &output, FontHandle *font, unicode_t unicode1, unicode_t unicode2) {
    return getKerning(output, font, GlyphIndex(FT_Get_Char_Index(font->face, unicode1)), GlyphIndex(FT_Get_Char_Index(font->face, unicode2)));
}

#ifndef MSDFGEN_DISABLE_VARIABLE_FONTS

bool setFontVariationAxis(FreetypeHandle *library, FontHandle *font, const char *name, real coordinate) {
    bool success = false;
    if (font->face->face_flags&FT_FACE_FLAG_MULTIPLE_MASTERS) {
        FT_MM_Var *master = NULL;
        if (FT_Get_MM_Var(font->face, &master))
            return false;
        if (master && master->num_axis) {
            std::vector<FT_Fixed> coords(master->num_axis);
            if (!FT_Get_Var_Design_Coordinates(font->face, FT_UInt(coords.size()), &coords[0])) {
                for (FT_UInt i = 0; i < master->num_axis; ++i) {
                    if (!strcmp(name, master->axis[i].name)) {
                        coords[i] = REAL_TO_F16DOT16(coordinate);
                        success = true;
                        break;
                    }
                }
            }
            if (FT_Set_Var_Design_Coordinates(font->face, FT_UInt(coords.size()), &coords[0]))
                success = false;
        }
        FT_Done_MM_Var(library->library, master);
    }
    return success;
}

bool listFontVariationAxes(std::vector<FontVariationAxis> &axes, FreetypeHandle *library, FontHandle *font) {
    if (font->face->face_flags&FT_FACE_FLAG_MULTIPLE_MASTERS) {
        FT_MM_Var *master = NULL;
        if (FT_Get_MM_Var(font->face, &master))
            return false;
        axes.resize(master->num_axis);
        for (FT_UInt i = 0; i < master->num_axis; ++i) {
            FontVariationAxis &axis = axes[i];
            axis.name = master->axis[i].name;
            axis.minValue = F16DOT16_TO_REAL(master->axis[i].minimum);
            axis.maxValue = F16DOT16_TO_REAL(master->axis[i].maximum);
            axis.defaultValue = F16DOT16_TO_REAL(master->axis[i].def);
        }
        FT_Done_MM_Var(library->library, master);
        return true;
    }
    return false;
}

#endif

}
