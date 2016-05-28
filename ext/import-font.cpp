
#include "import-font.h"

#include <cstdlib>
#include <queue>
#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef _WIN32
    #pragma comment(lib, "freetype.lib")
#endif

namespace msdfgen {

#define REQUIRE(cond) { if (!(cond)) return false; }

class FreetypeHandle {
    friend FreetypeHandle * initializeFreetype();
    friend void deinitializeFreetype(FreetypeHandle *library);
    friend FontHandle * loadFont(FreetypeHandle *library, const char *filename);

    FT_Library library;

};

class FontHandle {
    friend FontHandle * loadFont(FreetypeHandle *library, const char *filename);
    friend void destroyFont(FontHandle *font);
    friend bool getFontScale(double &output, FontHandle *font);
    friend bool getFontWhitespaceWidth(double &spaceAdvance, double &tabAdvance, FontHandle *font);
    friend bool loadGlyph(Shape &output, FontHandle *font, int unicode, double *advance);
    friend bool getKerning(double &output, FontHandle *font, int unicode1, int unicode2);

    FT_Face face;

};

FreetypeHandle * initializeFreetype() {
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

FontHandle * loadFont(FreetypeHandle *library, const char *filename) {
    if (!library)
        return NULL;
    FontHandle *handle = new FontHandle;
    FT_Error error = FT_New_Face(library->library, filename, 0, &handle->face);
    if (error) {
        delete handle;
        return NULL;
    }
    return handle;
}

void destroyFont(FontHandle *font) {
    FT_Done_Face(font->face);
    delete font;
}

bool getFontScale(double &output, FontHandle *font) {
    output = font->face->units_per_EM/64.;
    return true;
}

bool getFontWhitespaceWidth(double &spaceAdvance, double &tabAdvance, FontHandle *font) {
    FT_Error error = FT_Load_Char(font->face, ' ', FT_LOAD_NO_SCALE);
    if (error)
        return false;
    spaceAdvance = font->face->glyph->advance.x/64.;
    error = FT_Load_Char(font->face, '\t', FT_LOAD_NO_SCALE);
    if (error)
        return false;
    tabAdvance = font->face->glyph->advance.x/64.;
    return true;
}

bool loadGlyph(Shape &output, FontHandle *font, int unicode, double *advance) {
    enum PointType {
        NONE = 0,
        PATH_POINT,
        QUADRATIC_POINT,
        CUBIC_POINT,
        CUBIC_POINT2
    };

    if (!font)
        return false;
    FT_Error error = FT_Load_Char(font->face, unicode, FT_LOAD_NO_SCALE);
    if (error)
        return false;
    output.contours.clear();
    output.inverseYAxis = false;
    if (advance)
        *advance = font->face->glyph->advance.x/64.;

    int last = -1;
    // For each contour
    for (int i = 0; i < font->face->glyph->outline.n_contours; ++i) {

        Contour &contour = output.addContour();
        int first = last+1;
        last = font->face->glyph->outline.contours[i];

        PointType state = NONE;
        Point2 startPoint;
        Point2 controlPoint[2];

        // For each point on the contour
        for (int round = 0, index = first; round == 0; ++index) {
            // Close contour
            if (index > last) {
                index = first;
                round++;
            }

            Point2 point(font->face->glyph->outline.points[index].x/64., font->face->glyph->outline.points[index].y/64.);
            PointType pointType = font->face->glyph->outline.tags[index]&1 ? PATH_POINT : font->face->glyph->outline.tags[index]&2 ? CUBIC_POINT : QUADRATIC_POINT;

            switch (state) {
                case NONE:
                    REQUIRE(pointType == PATH_POINT);
                    startPoint = point;
                    state = PATH_POINT;
                    break;
                case PATH_POINT:
                    if (pointType == PATH_POINT) {
                        contour.addEdge(new LinearSegment(startPoint, point));
                        startPoint = point;
                    } else {
                        controlPoint[0] = point;
                        state = pointType;
                    }
                    break;
                case QUADRATIC_POINT:
                    REQUIRE(pointType != CUBIC_POINT);
                    if (pointType == PATH_POINT) {
                        contour.addEdge(new QuadraticSegment(startPoint, controlPoint[0], point));
                        startPoint = point;
                        state = PATH_POINT;
                    } else {
                        Point2 midPoint = .5*controlPoint[0]+.5*point;
                        contour.addEdge(new QuadraticSegment(startPoint, controlPoint[0], midPoint));
                        startPoint = midPoint;
                        controlPoint[0] = point;
                    }
                    break;
                case CUBIC_POINT:
                    REQUIRE(pointType == CUBIC_POINT);
                    controlPoint[1] = point;
                    state = CUBIC_POINT2;
                    break;
                case CUBIC_POINT2:
                    REQUIRE(pointType != QUADRATIC_POINT);
                    if (pointType == PATH_POINT) {
                        contour.addEdge(new CubicSegment(startPoint, controlPoint[0], controlPoint[1], point));
                        startPoint = point;
                    } else {
                        Point2 midPoint = .5*controlPoint[1]+.5*point;
                        contour.addEdge(new CubicSegment(startPoint, controlPoint[0], controlPoint[1], midPoint));
                        startPoint = midPoint;
                        controlPoint[0] = point;
                    }
                    state = pointType;
                    break;
            }

        }
    }
    return true;
}

bool getKerning(double &output, FontHandle *font, int unicode1, int unicode2) {
    FT_Vector kerning;
    if (FT_Get_Kerning(font->face, FT_Get_Char_Index(font->face, unicode1), FT_Get_Char_Index(font->face, unicode2), FT_KERNING_UNSCALED, &kerning)) {
        output = 0;
        return false;
    }
    output = kerning.x/64.;
    return true;
}

}
