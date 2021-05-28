
/*
 * MULTI-CHANNEL SIGNED DISTANCE FIELD GENERATOR v1.9 (2021-05-28) - standalone console program
 * --------------------------------------------------------------------------------------------
 * A utility by Viktor Chlumsky, (c) 2014 - 2021
 *
 */

#ifdef MSDFGEN_STANDALONE

#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cmath>
#include <cstring>

#include "msdfgen.h"
#include "msdfgen-ext.h"

#include "core/ShapeDistanceFinder.h"

#define SDF_ERROR_ESTIMATE_PRECISION 19
#define DEFAULT_ANGLE_THRESHOLD 3.

using namespace msdfgen;

enum Format {
    AUTO,
    PNG,
    BMP,
    TIFF,
    TEXT,
    TEXT_FLOAT,
    BINARY,
    BINARY_FLOAT,
    BINARY_FLOAT_BE
};

static bool is8bitFormat(Format format) {
    return format == PNG || format == BMP || format == TEXT || format == BINARY;
}

static char toupper(char c) {
    return c >= 'a' && c <= 'z' ? c-'a'+'A' : c;
}

static bool parseUnsigned(unsigned &value, const char *arg) {
    char c;
    return sscanf(arg, "%u%c", &value, &c) == 1;
}

static bool parseUnsignedDecOrHex(unsigned &value, const char *arg) {
    if (arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X')) {
        char c;
        return sscanf(arg+2, "%x%c", &value, &c) == 1;
    }
    return parseUnsigned(value, arg);
}

static bool parseUnsignedLL(unsigned long long &value, const char *arg) {
    char c;
    return sscanf(arg, "%llu%c", &value, &c) == 1;
}

static bool parseDouble(double &value, const char *arg) {
    char c;
    return sscanf(arg, "%lf%c", &value, &c) == 1;
}

static bool parseUnicode(unicode_t &unicode, const char *arg) {
    unsigned uuc;
    if (parseUnsignedDecOrHex(uuc, arg)) {
        unicode = uuc;
        return true;
    }
    if (arg[0] == '\'' && arg[1] && arg[2] == '\'' && !arg[3]) {
        unicode = (unicode_t) (unsigned char) arg[1];
        return true;
    }
    return false;
}

static bool parseAngle(double &value, const char *arg) {
    char c1, c2;
    int result = sscanf(arg, "%lf%c%c", &value, &c1, &c2);
    if (result == 1)
        return true;
    if (result == 2 && (c1 == 'd' || c1 == 'D')) {
        value *= M_PI/180;
        return true;
    }
    return false;
}

static void parseColoring(Shape &shape, const char *edgeAssignment) {
    unsigned c = 0, e = 0;
    if (shape.contours.size() < c) return;
    Contour *contour = &shape.contours[c];
    bool change = false;
    bool clear = true;
    for (const char *in = edgeAssignment; *in; ++in) {
        switch (*in) {
            case ',':
                if (change)
                    ++e;
                if (clear)
                    while (e < contour->edges.size()) {
                        contour->edges[e]->color = WHITE;
                        ++e;
                    }
                ++c, e = 0;
                if (shape.contours.size() <= c) return;
                contour = &shape.contours[c];
                change = false;
                clear = true;
                break;
            case '?':
                clear = false;
                break;
            case 'C': case 'M': case 'W': case 'Y': case 'c': case 'm': case 'w': case 'y':
                if (change) {
                    ++e;
                    change = false;
                }
                if (e < contour->edges.size()) {
                    contour->edges[e]->color = EdgeColor(
                        (*in == 'C' || *in == 'c')*CYAN|
                        (*in == 'M' || *in == 'm')*MAGENTA|
                        (*in == 'Y' || *in == 'y')*YELLOW|
                        (*in == 'W' || *in == 'w')*WHITE);
                    change = true;
                }
                break;
        }
    }
}

template <int N>
static void invertColor(const BitmapRef<float, N> &bitmap) {
    const float *end = bitmap.pixels+N*bitmap.width*bitmap.height;
    for (float *p = bitmap.pixels; p < end; ++p)
        *p = 1.f-*p;
}

static bool writeTextBitmap(FILE *file, const float *values, int cols, int rows) {
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            int v = clamp(int((*values++)*0x100), 0xff);
            fprintf(file, col ? " %02X" : "%02X", v);
        }
        fprintf(file, "\n");
    }
    return true;
}

static bool writeTextBitmapFloat(FILE *file, const float *values, int cols, int rows) {
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            fprintf(file, col ? " %.9g" : "%.9g", *values++);
        }
        fprintf(file, "\n");
    }
    return true;
}

static bool writeBinBitmap(FILE *file, const float *values, int count) {
    for (int pos = 0; pos < count; ++pos) {
        unsigned char v = clamp(int((*values++)*0x100), 0xff);
        fwrite(&v, 1, 1, file);
    }
    return true;
}

#ifdef __BIG_ENDIAN__
static bool writeBinBitmapFloatBE(FILE *file, const float *values, int count)
#else
static bool writeBinBitmapFloat(FILE *file, const float *values, int count)
#endif
{
    return (int) fwrite(values, sizeof(float), count, file) == count;
}

#ifdef __BIG_ENDIAN__
static bool writeBinBitmapFloat(FILE *file, const float *values, int count)
#else
static bool writeBinBitmapFloatBE(FILE *file, const float *values, int count)
#endif
{
    for (int pos = 0; pos < count; ++pos) {
        const unsigned char *b = reinterpret_cast<const unsigned char *>(values++);
        for (int i = sizeof(float)-1; i >= 0; --i)
            fwrite(b+i, 1, 1, file);
    }
    return true;
}

static bool cmpExtension(const char *path, const char *ext) {
    for (const char *a = path+strlen(path)-1, *b = ext+strlen(ext)-1; b >= ext; --a, --b)
        if (a < path || toupper(*a) != toupper(*b))
            return false;
    return true;
}

template <int N>
static const char * writeOutput(const BitmapConstRef<float, N> &bitmap, const char *filename, Format &format) {
    if (filename) {
        if (format == AUTO) {
            if (cmpExtension(filename, ".png")) format = PNG;
            else if (cmpExtension(filename, ".bmp")) format = BMP;
            else if (cmpExtension(filename, ".tif") || cmpExtension(filename, ".tiff")) format = TIFF;
            else if (cmpExtension(filename, ".txt")) format = TEXT;
            else if (cmpExtension(filename, ".bin")) format = BINARY;
            else
                return "Could not deduce format from output file name.";
        }
        switch (format) {
            case PNG: return savePng(bitmap, filename) ? NULL : "Failed to write output PNG image.";
            case BMP: return saveBmp(bitmap, filename) ? NULL : "Failed to write output BMP image.";
            case TIFF: return saveTiff(bitmap, filename) ? NULL : "Failed to write output TIFF image.";
            case TEXT: case TEXT_FLOAT: {
                FILE *file = fopen(filename, "w");
                if (!file) return "Failed to write output text file.";
                if (format == TEXT)
                    writeTextBitmap(file, bitmap.pixels, N*bitmap.width, bitmap.height);
                else if (format == TEXT_FLOAT)
                    writeTextBitmapFloat(file, bitmap.pixels, N*bitmap.width, bitmap.height);
                fclose(file);
                return NULL;
            }
            case BINARY: case BINARY_FLOAT: case BINARY_FLOAT_BE: {
                FILE *file = fopen(filename, "wb");
                if (!file) return "Failed to write output binary file.";
                if (format == BINARY)
                    writeBinBitmap(file, bitmap.pixels, N*bitmap.width*bitmap.height);
                else if (format == BINARY_FLOAT)
                    writeBinBitmapFloat(file, bitmap.pixels, N*bitmap.width*bitmap.height);
                else if (format == BINARY_FLOAT_BE)
                    writeBinBitmapFloatBE(file, bitmap.pixels, N*bitmap.width*bitmap.height);
                fclose(file);
                return NULL;
            }
            default:;
        }
    } else {
        if (format == AUTO || format == TEXT)
            writeTextBitmap(stdout, bitmap.pixels, N*bitmap.width, bitmap.height);
        else if (format == TEXT_FLOAT)
            writeTextBitmapFloat(stdout, bitmap.pixels, N*bitmap.width, bitmap.height);
        else
            return "Unsupported format for standard output.";
    }
    return NULL;
}

#if defined(MSDFGEN_USE_SKIA) && defined(MSDFGEN_USE_OPENMP)
    #define TITLE_SUFFIX    " with Skia & OpenMP"
    #define EXTRA_UNDERLINE "-------------------"
#elif defined(MSDFGEN_USE_SKIA)
    #define TITLE_SUFFIX    " with Skia"
    #define EXTRA_UNDERLINE "----------"
#elif defined(MSDFGEN_USE_OPENMP)
    #define TITLE_SUFFIX    " with OpenMP"
    #define EXTRA_UNDERLINE "------------"
#else
    #define TITLE_SUFFIX
    #define EXTRA_UNDERLINE
#endif

static const char *helpText =
    "\n"
    "Multi-channel signed distance field generator by Viktor Chlumsky v" MSDFGEN_VERSION TITLE_SUFFIX "\n"
    "---------------------------------------------------------------------" EXTRA_UNDERLINE "\n"
    "  Usage: msdfgen"
    #ifdef _WIN32
        ".exe"
    #endif
        " <mode> <input specification> <options>\n"
    "\n"
    "MODES\n"
    "  sdf - Generate conventional monochrome (true) signed distance field.\n"
    "  psdf - Generate monochrome signed pseudo-distance field.\n"
    "  msdf - Generate multi-channel signed distance field. This is used by default if no mode is specified.\n"
    "  mtsdf - Generate combined multi-channel and true signed distance field in the alpha channel.\n"
    "  metrics - Report shape metrics only.\n"
    "\n"
    "INPUT SPECIFICATION\n"
    "  -defineshape <definition>\n"
        "\tDefines input shape using the ad-hoc text definition.\n"
    "  -font <filename.ttf> <character code>\n"
        "\tLoads a single glyph from the specified font file.\n"
        "\tFormat of character code is '?', 63, 0x3F (Unicode value), or g34 (glyph index).\n"
    "  -shapedesc <filename.txt>\n"
        "\tLoads text shape description from a file.\n"
    "  -stdin\n"
        "\tReads text shape description from the standard input.\n"
    "  -svg <filename.svg>\n"
        "\tLoads the last vector path found in the specified SVG file.\n"
    "\n"
    // Keep alphabetical order!
    "OPTIONS\n"
    "  -angle <angle>\n"
        "\tSpecifies the minimum angle between adjacent edges to be considered a corner. Append D for degrees.\n"
    "  -ascale <x scale> <y scale>\n"
        "\tSets the scale used to convert shape units to pixels asymmetrically.\n"
    "  -autoframe\n"
        "\tAutomatically scales (unless specified) and translates the shape to fit.\n"
    "  -coloringstrategy <simple / inktrap / distance>\n"
        "\tSelects the strategy of the edge coloring heuristic.\n"
    "  -distanceshift <shift>\n"
        "\tShifts all normalized distances in the output distance field by this value.\n"
    "  -edgecolors <sequence>\n"
        "\tOverrides automatic edge coloring with the specified color sequence.\n"
    "  -errorcorrection <mode>\n"
        "\tChanges the MSDF/MTSDF error correction mode. Use -errorcorrection help for a list of valid modes.\n"
    "  -errordeviationratio <ratio>\n"
        "\tSets the minimum ratio between the actual and maximum expected distance delta to be considered an error.\n"
    "  -errorimproveratio <ratio>\n"
        "\tSets the minimum ratio between the pre-correction distance error and the post-correction distance error.\n"
    "  -estimateerror\n"
        "\tComputes and prints the distance field's estimated fill error to the standard output.\n"
    "  -exportshape <filename.txt>\n"
        "\tSaves the shape description into a text file that can be edited and loaded using -shapedesc.\n"
    "  -fillrule <nonzero / evenodd / positive / negative>\n"
        "\tSets the fill rule for the scanline pass. Default is nonzero.\n"
    "  -format <png / bmp / tiff / text / textfloat / bin / binfloat / binfloatbe>\n"
        "\tSpecifies the output format of the distance field. Otherwise it is chosen based on output file extension.\n"
    "  -guessorder\n"
        "\tAttempts to detect if shape contours have the wrong winding and generates the SDF with the right one.\n"
    "  -help\n"
        "\tDisplays this help.\n"
    "  -legacy\n"
        "\tUses the original (legacy) distance field algorithms.\n"
#ifdef MSDFGEN_USE_SKIA
    "  -nopreprocess\n"
        "\tDisables path preprocessing which resolves self-intersections and overlapping contours.\n"
#else
    "  -nooverlap\n"
        "\tDisables resolution of overlapping contours.\n"
    "  -noscanline\n"
        "\tDisables the scanline pass, which corrects the distance field's signs according to the selected fill rule.\n"
#endif
    "  -o <filename>\n"
        "\tSets the output file name. The default value is \"output.png\".\n"
#ifdef MSDFGEN_USE_SKIA
    "  -overlap\n"
        "\tSwitches to distance field generator with support for overlapping contours.\n"
#endif
    "  -printmetrics\n"
        "\tPrints relevant metrics of the shape to the standard output.\n"
    "  -pxrange <range>\n"
        "\tSets the width of the range between the lowest and highest signed distance in pixels.\n"
    "  -range <range>\n"
        "\tSets the width of the range between the lowest and highest signed distance in shape units.\n"
    "  -reverseorder\n"
        "\tGenerates the distance field as if the shape's vertices were in reverse order.\n"
    "  -scale <scale>\n"
        "\tSets the scale used to convert shape units to pixels.\n"
#ifdef MSDFGEN_USE_SKIA
    "  -scanline\n"
        "\tPerforms an additional scanline pass to fix the signs of the distances.\n"
#endif
    "  -seed <n>\n"
        "\tSets the random seed for edge coloring heuristic.\n"
    "  -size <width> <height>\n"
        "\tSets the dimensions of the output image.\n"
    "  -stdout\n"
        "\tPrints the output instead of storing it in a file. Only text formats are supported.\n"
    "  -testrender <filename.png> <width> <height>\n"
        "\tRenders an image preview using the generated distance field and saves it as a PNG file.\n"
    "  -testrendermulti <filename.png> <width> <height>\n"
        "\tRenders an image preview without flattening the color channels.\n"
    "  -translate <x> <y>\n"
        "\tSets the translation of the shape in shape units.\n"
    "  -yflip\n"
        "\tInverts the Y axis in the output distance field. The default order is bottom to top.\n"
    "\n";

static const char *errorCorrectionHelpText =
    "\n"
    "ERROR CORRECTION MODES\n"
    "  auto-fast\n"
        "\tDetects inversion artifacts and distance errors that do not affect edges by range testing.\n"
    "  auto-full\n"
        "\tDetects inversion artifacts and distance errors that do not affect edges by exact distance evaluation.\n"
    "  auto-mixed (default)\n"
        "\tDetects inversions by distance evaluation and distance errors that do not affect edges by range testing.\n"
    "  disabled\n"
        "\tDisables error correction.\n"
    "  distance-fast\n"
        "\tDetects distance errors by range testing. Does not care if edges and corners are affected.\n"
    "  distance-full\n"
        "\tDetects distance errors by exact distance evaluation. Does not care if edges and corners are affected, slow.\n"
    "  edge-fast\n"
        "\tDetects inversion artifacts only by range testing.\n"
    "  edge-full\n"
        "\tDetects inversion artifacts only by exact distance evaluation.\n"
    "  help\n"
        "\tDisplays this help.\n"
    "\n";

int main(int argc, const char * const *argv) {
    #define ABORT(msg) { puts(msg); return 1; }

    // Parse command line arguments
    enum {
        NONE,
        SVG,
        FONT,
        DESCRIPTION_ARG,
        DESCRIPTION_STDIN,
        DESCRIPTION_FILE
    } inputType = NONE;
    enum {
        SINGLE,
        PSEUDO,
        MULTI,
        MULTI_AND_TRUE,
        METRICS
    } mode = MULTI;
    bool legacyMode = false;
    bool geometryPreproc = (
        #ifdef MSDFGEN_USE_SKIA
            true
        #else
            false
        #endif
    );
    MSDFGeneratorConfig generatorConfig;
    generatorConfig.overlapSupport = !geometryPreproc;
    bool scanlinePass = !geometryPreproc;
    FillRule fillRule = FILL_NONZERO;
    Format format = AUTO;
    const char *input = NULL;
    const char *output = "output.png";
    const char *shapeExport = NULL;
    const char *testRender = NULL;
    const char *testRenderMulti = NULL;
    bool outputSpecified = false;
    GlyphIndex glyphIndex;
    unicode_t unicode = 0;
    int svgPathIndex = 0;

    int width = 64, height = 64;
    int testWidth = 0, testHeight = 0;
    int testWidthM = 0, testHeightM = 0;
    bool autoFrame = false;
    enum {
        RANGE_UNIT,
        RANGE_PX
    } rangeMode = RANGE_PX;
    double range = 1;
    double pxRange = 2;
    Vector2 translate;
    Vector2 scale = 1;
    bool scaleSpecified = false;
    double angleThreshold = DEFAULT_ANGLE_THRESHOLD;
    float outputDistanceShift = 0.f;
    const char *edgeAssignment = NULL;
    bool yFlip = false;
    bool printMetrics = false;
    bool estimateError = false;
    bool skipColoring = false;
    enum {
        KEEP,
        REVERSE,
        GUESS
    } orientation = KEEP;
    unsigned long long coloringSeed = 0;
    void (*edgeColoring)(Shape &, double, unsigned long long) = edgeColoringSimple;
    bool explicitErrorCorrectionMode = false;

    int argPos = 1;
    bool suggestHelp = false;
    while (argPos < argc) {
        const char *arg = argv[argPos];
        #define ARG_CASE(s, p) if (!strcmp(arg, s) && argPos+(p) < argc)
        #define ARG_MODE(s, m) if (!strcmp(arg, s)) { mode = m; ++argPos; continue; }
        #define SET_FORMAT(fmt, ext) do { format = fmt; if (!outputSpecified) output = "output." ext; } while (false)

        ARG_MODE("sdf", SINGLE)
        ARG_MODE("psdf", PSEUDO)
        ARG_MODE("msdf", MULTI)
        ARG_MODE("mtsdf", MULTI_AND_TRUE)
        ARG_MODE("metrics", METRICS)

        ARG_CASE("-svg", 1) {
            inputType = SVG;
            input = argv[argPos+1];
            argPos += 2;
            continue;
        }
        ARG_CASE("-font", 2) {
            inputType = FONT;
            input = argv[argPos+1];
            const char *charArg = argv[argPos+2];
            unsigned gi;
            switch (charArg[0]) {
                case 'G': case 'g':
                    if (parseUnsignedDecOrHex(gi, charArg+1))
                        glyphIndex = GlyphIndex(gi);
                    break;
                case 'U': case 'u':
                    ++charArg;
                default:
                    parseUnicode(unicode, charArg);
            }
            argPos += 3;
            continue;
        }
        ARG_CASE("-defineshape", 1) {
            inputType = DESCRIPTION_ARG;
            input = argv[argPos+1];
            argPos += 2;
            continue;
        }
        ARG_CASE("-stdin", 0) {
            inputType = DESCRIPTION_STDIN;
            input = "stdin";
            argPos += 1;
            continue;
        }
        ARG_CASE("-shapedesc", 1) {
            inputType = DESCRIPTION_FILE;
            input = argv[argPos+1];
            argPos += 2;
            continue;
        }
        ARG_CASE("-o", 1) {
            output = argv[argPos+1];
            outputSpecified = true;
            argPos += 2;
            continue;
        }
        ARG_CASE("-stdout", 0) {
            output = NULL;
            argPos += 1;
            continue;
        }
        ARG_CASE("-legacy", 0) {
            legacyMode = true;
            argPos += 1;
            continue;
        }
        ARG_CASE("-nopreprocess", 0) {
            geometryPreproc = false;
            argPos += 1;
            continue;
        }
        ARG_CASE("-preprocess", 0) {
            geometryPreproc = true;
            argPos += 1;
            continue;
        }
        ARG_CASE("-nooverlap", 0) {
            generatorConfig.overlapSupport = false;
            argPos += 1;
            continue;
        }
        ARG_CASE("-overlap", 0) {
            generatorConfig.overlapSupport = true;
            argPos += 1;
            continue;
        }
        ARG_CASE("-noscanline", 0) {
            scanlinePass = false;
            argPos += 1;
            continue;
        }
        ARG_CASE("-scanline", 0) {
            scanlinePass = true;
            argPos += 1;
            continue;
        }
        ARG_CASE("-fillrule", 1) {
            scanlinePass = true;
            if (!strcmp(argv[argPos+1], "nonzero")) fillRule = FILL_NONZERO;
            else if (!strcmp(argv[argPos+1], "evenodd") || !strcmp(argv[argPos+1], "odd")) fillRule = FILL_ODD;
            else if (!strcmp(argv[argPos+1], "positive")) fillRule = FILL_POSITIVE;
            else if (!strcmp(argv[argPos+1], "negative")) fillRule = FILL_NEGATIVE;
            else
                puts("Unknown fill rule specified.");
            argPos += 2;
            continue;
        }
        ARG_CASE("-format", 1) {
            if (!strcmp(argv[argPos+1], "auto")) format = AUTO;
            else if (!strcmp(argv[argPos+1], "png")) SET_FORMAT(PNG, "png");
            else if (!strcmp(argv[argPos+1], "bmp")) SET_FORMAT(BMP, "bmp");
            else if (!strcmp(argv[argPos+1], "tiff")) SET_FORMAT(TIFF, "tif");
            else if (!strcmp(argv[argPos+1], "text") || !strcmp(argv[argPos+1], "txt")) SET_FORMAT(TEXT, "txt");
            else if (!strcmp(argv[argPos+1], "textfloat") || !strcmp(argv[argPos+1], "txtfloat")) SET_FORMAT(TEXT_FLOAT, "txt");
            else if (!strcmp(argv[argPos+1], "bin") || !strcmp(argv[argPos+1], "binary")) SET_FORMAT(BINARY, "bin");
            else if (!strcmp(argv[argPos+1], "binfloat") || !strcmp(argv[argPos+1], "binfloatle")) SET_FORMAT(BINARY_FLOAT, "bin");
            else if (!strcmp(argv[argPos+1], "binfloatbe")) SET_FORMAT(BINARY_FLOAT_BE, "bin");
            else
                puts("Unknown format specified.");
            argPos += 2;
            continue;
        }
        ARG_CASE("-size", 2) {
            unsigned w, h;
            if (!(parseUnsigned(w, argv[argPos+1]) && parseUnsigned(h, argv[argPos+2]) && w && h))
                ABORT("Invalid size arguments. Use -size <width> <height> with two positive integers.");
            width = w, height = h;
            argPos += 3;
            continue;
        }
        ARG_CASE("-autoframe", 0) {
            autoFrame = true;
            argPos += 1;
            continue;
        }
        ARG_CASE("-range", 1) {
            double r;
            if (!(parseDouble(r, argv[argPos+1]) && r > 0))
                ABORT("Invalid range argument. Use -range <range> with a positive real number.");
            rangeMode = RANGE_UNIT;
            range = r;
            argPos += 2;
            continue;
        }
        ARG_CASE("-pxrange", 1) {
            double r;
            if (!(parseDouble(r, argv[argPos+1]) && r > 0))
                ABORT("Invalid range argument. Use -pxrange <range> with a positive real number.");
            rangeMode = RANGE_PX;
            pxRange = r;
            argPos += 2;
            continue;
        }
        ARG_CASE("-scale", 1) {
            double s;
            if (!(parseDouble(s, argv[argPos+1]) && s > 0))
                ABORT("Invalid scale argument. Use -scale <scale> with a positive real number.");
            scale = s;
            scaleSpecified = true;
            argPos += 2;
            continue;
        }
        ARG_CASE("-ascale", 2) {
            double sx, sy;
            if (!(parseDouble(sx, argv[argPos+1]) && parseDouble(sy, argv[argPos+2]) && sx > 0 && sy > 0))
                ABORT("Invalid scale arguments. Use -ascale <x> <y> with two positive real numbers.");
            scale.set(sx, sy);
            scaleSpecified = true;
            argPos += 3;
            continue;
        }
        ARG_CASE("-translate", 2) {
            double tx, ty;
            if (!(parseDouble(tx, argv[argPos+1]) && parseDouble(ty, argv[argPos+2])))
                ABORT("Invalid translate arguments. Use -translate <x> <y> with two real numbers.");
            translate.set(tx, ty);
            argPos += 3;
            continue;
        }
        ARG_CASE("-angle", 1) {
            double at;
            if (!parseAngle(at, argv[argPos+1]))
                ABORT("Invalid angle threshold. Use -angle <min angle> with a positive real number less than PI or a value in degrees followed by 'd' below 180d.");
            angleThreshold = at;
            argPos += 2;
            continue;
        }
        ARG_CASE("-errorcorrection", 1) {
            if (!strcmp(argv[argPos+1], "disabled") || !strcmp(argv[argPos+1], "0") || !strcmp(argv[argPos+1], "none")) {
                generatorConfig.errorCorrection.mode = ErrorCorrectionConfig::DISABLED;
                generatorConfig.errorCorrection.distanceCheckMode = ErrorCorrectionConfig::DO_NOT_CHECK_DISTANCE;
            } else if (!strcmp(argv[argPos+1], "default") || !strcmp(argv[argPos+1], "auto") || !strcmp(argv[argPos+1], "auto-mixed") || !strcmp(argv[argPos+1], "mixed")) {
                generatorConfig.errorCorrection.mode = ErrorCorrectionConfig::EDGE_PRIORITY;
                generatorConfig.errorCorrection.distanceCheckMode = ErrorCorrectionConfig::CHECK_DISTANCE_AT_EDGE;
            } else if (!strcmp(argv[argPos+1], "auto-fast") || !strcmp(argv[argPos+1], "fast")) {
                generatorConfig.errorCorrection.mode = ErrorCorrectionConfig::EDGE_PRIORITY;
                generatorConfig.errorCorrection.distanceCheckMode = ErrorCorrectionConfig::DO_NOT_CHECK_DISTANCE;
            } else if (!strcmp(argv[argPos+1], "auto-full") || !strcmp(argv[argPos+1], "full")) {
                generatorConfig.errorCorrection.mode = ErrorCorrectionConfig::EDGE_PRIORITY;
                generatorConfig.errorCorrection.distanceCheckMode = ErrorCorrectionConfig::ALWAYS_CHECK_DISTANCE;
            } else if (!strcmp(argv[argPos+1], "distance") || !strcmp(argv[argPos+1], "distance-fast") || !strcmp(argv[argPos+1], "indiscriminate") || !strcmp(argv[argPos+1], "indiscriminate-fast")) {
                generatorConfig.errorCorrection.mode = ErrorCorrectionConfig::INDISCRIMINATE;
                generatorConfig.errorCorrection.distanceCheckMode = ErrorCorrectionConfig::DO_NOT_CHECK_DISTANCE;
            } else if (!strcmp(argv[argPos+1], "distance-full") || !strcmp(argv[argPos+1], "indiscriminate-full")) {
                generatorConfig.errorCorrection.mode = ErrorCorrectionConfig::INDISCRIMINATE;
                generatorConfig.errorCorrection.distanceCheckMode = ErrorCorrectionConfig::ALWAYS_CHECK_DISTANCE;
            } else if (!strcmp(argv[argPos+1], "edge-fast")) {
                generatorConfig.errorCorrection.mode = ErrorCorrectionConfig::EDGE_ONLY;
                generatorConfig.errorCorrection.distanceCheckMode = ErrorCorrectionConfig::DO_NOT_CHECK_DISTANCE;
            } else if (!strcmp(argv[argPos+1], "edge") || !strcmp(argv[argPos+1], "edge-full")) {
                generatorConfig.errorCorrection.mode = ErrorCorrectionConfig::EDGE_ONLY;
                generatorConfig.errorCorrection.distanceCheckMode = ErrorCorrectionConfig::ALWAYS_CHECK_DISTANCE;
            } else if (!strcmp(argv[argPos+1], "help")) {
                puts(errorCorrectionHelpText);
                return 0;
            } else
                puts("Unknown error correction mode. Use -errorcorrection help for more information.");
            explicitErrorCorrectionMode = true;
            argPos += 2;
            continue;
        }
        ARG_CASE("-errordeviationratio", 1) {
            double edr;
            if (!(parseDouble(edr, argv[argPos+1]) && edr > 0))
                ABORT("Invalid error deviation ratio. Use -errordeviationratio <ratio> with a positive real number.");
            generatorConfig.errorCorrection.minDeviationRatio = edr;
            argPos += 2;
            continue;
        }
        ARG_CASE("-errorimproveratio", 1) {
            double eir;
            if (!(parseDouble(eir, argv[argPos+1]) && eir > 0))
                ABORT("Invalid error improvement ratio. Use -errorimproveratio <ratio> with a positive real number.");
            generatorConfig.errorCorrection.minImproveRatio = eir;
            argPos += 2;
            continue;
        }
        ARG_CASE("-coloringstrategy", 1) {
            if (!strcmp(argv[argPos+1], "simple")) edgeColoring = edgeColoringSimple;
            else if (!strcmp(argv[argPos+1], "inktrap")) edgeColoring = edgeColoringInkTrap;
            else if (!strcmp(argv[argPos+1], "distance")) edgeColoring = edgeColoringByDistance;
            else
                puts("Unknown coloring strategy specified.");
            argPos += 2;
            continue;
        }
        ARG_CASE("-edgecolors", 1) {
            static const char *allowed = " ?,cmwyCMWY";
            for (int i = 0; argv[argPos+1][i]; ++i) {
                for (int j = 0; allowed[j]; ++j)
                    if (argv[argPos+1][i] == allowed[j])
                        goto EDGE_COLOR_VERIFIED;
                ABORT("Invalid edge coloring sequence. Use -edgecolors <color sequence> with only the colors C, M, Y, and W. Separate contours by commas and use ? to keep the default assigment for a contour.");
            EDGE_COLOR_VERIFIED:;
            }
            edgeAssignment = argv[argPos+1];
            argPos += 2;
            continue;
        }
        ARG_CASE("-distanceshift", 1) {
            double ds;
            if (!parseDouble(ds, argv[argPos+1]))
                ABORT("Invalid distance shift. Use -distanceshift <shift> with a real value.");
            outputDistanceShift = (float) ds;
            argPos += 2;
            continue;
        }
        ARG_CASE("-exportshape", 1) {
            shapeExport = argv[argPos+1];
            argPos += 2;
            continue;
        }
        ARG_CASE("-testrender", 3) {
            unsigned w, h;
            if (!parseUnsigned(w, argv[argPos+2]) || !parseUnsigned(h, argv[argPos+3]) || !w || !h)
                ABORT("Invalid arguments for test render. Use -testrender <output.png> <width> <height>.");
            testRender = argv[argPos+1];
            testWidth = w, testHeight = h;
            argPos += 4;
            continue;
        }
        ARG_CASE("-testrendermulti", 3) {
            unsigned w, h;
            if (!parseUnsigned(w, argv[argPos+2]) || !parseUnsigned(h, argv[argPos+3]) || !w || !h)
                ABORT("Invalid arguments for test render. Use -testrendermulti <output.png> <width> <height>.");
            testRenderMulti = argv[argPos+1];
            testWidthM = w, testHeightM = h;
            argPos += 4;
            continue;
        }
        ARG_CASE("-yflip", 0) {
            yFlip = true;
            argPos += 1;
            continue;
        }
        ARG_CASE("-printmetrics", 0) {
            printMetrics = true;
            argPos += 1;
            continue;
        }
        ARG_CASE("-estimateerror", 0) {
            estimateError = true;
            argPos += 1;
            continue;
        }
        ARG_CASE("-keeporder", 0) {
            orientation = KEEP;
            argPos += 1;
            continue;
        }
        ARG_CASE("-reverseorder", 0) {
            orientation = REVERSE;
            argPos += 1;
            continue;
        }
        ARG_CASE("-guessorder", 0) {
            orientation = GUESS;
            argPos += 1;
            continue;
        }
        ARG_CASE("-seed", 1) {
            if (!parseUnsignedLL(coloringSeed, argv[argPos+1]))
                ABORT("Invalid seed. Use -seed <N> with N being a non-negative integer.");
            argPos += 2;
            continue;
        }
        ARG_CASE("-help", 0) {
            puts(helpText);
            return 0;
        }
        printf("Unknown setting or insufficient parameters: %s\n", arg);
        suggestHelp = true;
        ++argPos;
    }
    if (suggestHelp)
        printf("Use -help for more information.\n");

    // Load input
    Vector2 svgDims;
    double glyphAdvance = 0;
    if (!inputType || !input)
        ABORT("No input specified! Use either -svg <file.svg> or -font <file.ttf/otf> <character code>, or see -help.");
    if (mode == MULTI_AND_TRUE && (format == BMP || (format == AUTO && output && cmpExtension(output, ".bmp"))))
        ABORT("Incompatible image format. A BMP file cannot contain alpha channel, which is required in mtsdf mode.");
    Shape shape;
    switch (inputType) {
        case SVG: {
            if (!loadSvgShape(shape, input, svgPathIndex, &svgDims))
                ABORT("Failed to load shape from SVG file.");
            break;
        }
        case FONT: {
            if (!glyphIndex && !unicode)
                ABORT("No character specified! Use -font <file.ttf/otf> <character code>. Character code can be a Unicode index (65, 0x41), a character in apostrophes ('A'), or a glyph index prefixed by g (g36, g0x24).");
            FreetypeHandle *ft = initializeFreetype();
            if (!ft) return -1;
            FontHandle *font = loadFont(ft, input);
            if (!font) {
                deinitializeFreetype(ft);
                ABORT("Failed to load font file.");
            }
            if (unicode)
                getGlyphIndex(glyphIndex, font, unicode);
            if (!loadGlyph(shape, font, glyphIndex, &glyphAdvance)) {
                destroyFont(font);
                deinitializeFreetype(ft);
                ABORT("Failed to load glyph from font file.");
            }
            destroyFont(font);
            deinitializeFreetype(ft);
            break;
        }
        case DESCRIPTION_ARG: {
            if (!readShapeDescription(input, shape, &skipColoring))
                ABORT("Parse error in shape description.");
            break;
        }
        case DESCRIPTION_STDIN: {
            if (!readShapeDescription(stdin, shape, &skipColoring))
                ABORT("Parse error in shape description.");
            break;
        }
        case DESCRIPTION_FILE: {
            FILE *file = fopen(input, "r");
            if (!file)
                ABORT("Failed to load shape description file.");
            if (!readShapeDescription(file, shape, &skipColoring))
                ABORT("Parse error in shape description.");
            fclose(file);
            break;
        }
        default:;
    }

    // Validate and normalize shape
    if (!shape.validate())
        ABORT("The geometry of the loaded shape is invalid.");
    if (geometryPreproc) {
        #ifdef MSDFGEN_USE_SKIA
            if (!resolveShapeGeometry(shape))
                puts("Shape geometry preprocessing failed, skipping.");
        #else
            ABORT("Shape geometry preprocessing (-preprocess) is not available in this version because the Skia library is not present.");
        #endif
    }
    shape.normalize();
    if (yFlip)
        shape.inverseYAxis = !shape.inverseYAxis;

    double avgScale = .5*(scale.x+scale.y);
    Shape::Bounds bounds = { };
    if (autoFrame || mode == METRICS || printMetrics || orientation == GUESS)
        bounds = shape.getBounds();

    // Auto-frame
    if (autoFrame) {
        double l = bounds.l, b = bounds.b, r = bounds.r, t = bounds.t;
        Vector2 frame(width, height);
        double m = .5+(double) outputDistanceShift;
        if (!scaleSpecified) {
            if (rangeMode == RANGE_UNIT)
                l -= m*range, b -= m*range, r += m*range, t += m*range;
            else
                frame -= 2*m*pxRange;
        }
        if (l >= r || b >= t)
            l = 0, b = 0, r = 1, t = 1;
        if (frame.x <= 0 || frame.y <= 0)
            ABORT("Cannot fit the specified pixel range.");
        Vector2 dims(r-l, t-b);
        if (scaleSpecified)
            translate = .5*(frame/scale-dims)-Vector2(l, b);
        else {
            if (dims.x*frame.y < dims.y*frame.x) {
                translate.set(.5*(frame.x/frame.y*dims.y-dims.x)-l, -b);
                scale = avgScale = frame.y/dims.y;
            } else {
                translate.set(-l, .5*(frame.y/frame.x*dims.x-dims.y)-b);
                scale = avgScale = frame.x/dims.x;
            }
        }
        if (rangeMode == RANGE_PX && !scaleSpecified)
            translate += m*pxRange/scale;
    }

    if (rangeMode == RANGE_PX)
        range = pxRange/min(scale.x, scale.y);

    // Print metrics
    if (mode == METRICS || printMetrics) {
        FILE *out = stdout;
        if (mode == METRICS && outputSpecified)
            out = fopen(output, "w");
        if (!out)
            ABORT("Failed to write output file.");
        if (shape.inverseYAxis)
            fprintf(out, "inverseY = true\n");
        if (bounds.r >= bounds.l && bounds.t >= bounds.b)
            fprintf(out, "bounds = %.12g, %.12g, %.12g, %.12g\n", bounds.l, bounds.b, bounds.r, bounds.t);
        if (svgDims.x != 0 && svgDims.y != 0)
            fprintf(out, "dimensions = %.12g, %.12g\n", svgDims.x, svgDims.y);
        if (glyphAdvance != 0)
            fprintf(out, "advance = %.12g\n", glyphAdvance);
        if (autoFrame) {
            if (!scaleSpecified)
                fprintf(out, "scale = %.12g\n", avgScale);
            fprintf(out, "translate = %.12g, %.12g\n", translate.x, translate.y);
        }
        if (rangeMode == RANGE_PX)
            fprintf(out, "range = %.12g\n", range);
        if (mode == METRICS && outputSpecified)
            fclose(out);
    }

    // Compute output
    Projection projection(scale, translate);
    Bitmap<float, 1> sdf;
    Bitmap<float, 3> msdf;
    Bitmap<float, 4> mtsdf;
    MSDFGeneratorConfig postErrorCorrectionConfig(generatorConfig);
    if (scanlinePass) {
        if (explicitErrorCorrectionMode && generatorConfig.errorCorrection.distanceCheckMode != ErrorCorrectionConfig::DO_NOT_CHECK_DISTANCE) {
            const char *fallbackModeName = "unknown";
            switch (generatorConfig.errorCorrection.mode) {
                case ErrorCorrectionConfig::DISABLED: fallbackModeName = "disabled"; break;
                case ErrorCorrectionConfig::INDISCRIMINATE: fallbackModeName = "distance-fast"; break;
                case ErrorCorrectionConfig::EDGE_PRIORITY: fallbackModeName = "auto-fast"; break;
                case ErrorCorrectionConfig::EDGE_ONLY: fallbackModeName = "edge-fast"; break;
            }
            printf("Selected error correction mode not compatible with scanline mode, falling back to %s.\n", fallbackModeName);
        }
        generatorConfig.errorCorrection.mode = ErrorCorrectionConfig::DISABLED;
        postErrorCorrectionConfig.errorCorrection.distanceCheckMode = ErrorCorrectionConfig::DO_NOT_CHECK_DISTANCE;
    }
    switch (mode) {
        case SINGLE: {
            sdf = Bitmap<float, 1>(width, height);
            if (legacyMode)
                generateSDF_legacy(sdf, shape, range, scale, translate);
            else
                generateSDF(sdf, shape, projection, range, generatorConfig);
            break;
        }
        case PSEUDO: {
            sdf = Bitmap<float, 1>(width, height);
            if (legacyMode)
                generatePseudoSDF_legacy(sdf, shape, range, scale, translate);
            else
                generatePseudoSDF(sdf, shape, projection, range, generatorConfig);
            break;
        }
        case MULTI: {
            if (!skipColoring)
                edgeColoring(shape, angleThreshold, coloringSeed);
            if (edgeAssignment)
                parseColoring(shape, edgeAssignment);
            msdf = Bitmap<float, 3>(width, height);
            if (legacyMode)
                generateMSDF_legacy(msdf, shape, range, scale, translate, generatorConfig.errorCorrection);
            else
                generateMSDF(msdf, shape, projection, range, generatorConfig);
            break;
        }
        case MULTI_AND_TRUE: {
            if (!skipColoring)
                edgeColoring(shape, angleThreshold, coloringSeed);
            if (edgeAssignment)
                parseColoring(shape, edgeAssignment);
            mtsdf = Bitmap<float, 4>(width, height);
            if (legacyMode)
                generateMTSDF_legacy(mtsdf, shape, range, scale, translate, generatorConfig.errorCorrection);
            else
                generateMTSDF(mtsdf, shape, projection, range, generatorConfig);
            break;
        }
        default:;
    }

    if (orientation == GUESS) {
        // Get sign of signed distance outside bounds
        Point2 p(bounds.l-(bounds.r-bounds.l)-1, bounds.b-(bounds.t-bounds.b)-1);
        double distance = SimpleTrueShapeDistanceFinder::oneShotDistance(shape, p);
        orientation = distance <= 0 ? KEEP : REVERSE;
    }
    if (orientation == REVERSE) {
        switch (mode) {
            case SINGLE:
            case PSEUDO:
                invertColor<1>(sdf);
                break;
            case MULTI:
                invertColor<3>(msdf);
                break;
            case MULTI_AND_TRUE:
                invertColor<4>(mtsdf);
                break;
            default:;
        }
    }
    if (scanlinePass) {
        switch (mode) {
            case SINGLE:
            case PSEUDO:
                distanceSignCorrection(sdf, shape, projection, fillRule);
                break;
            case MULTI:
                distanceSignCorrection(msdf, shape, projection, fillRule);
                msdfErrorCorrection(msdf, shape, projection, range, postErrorCorrectionConfig);
                break;
            case MULTI_AND_TRUE:
                distanceSignCorrection(mtsdf, shape, projection, fillRule);
                msdfErrorCorrection(msdf, shape, projection, range, postErrorCorrectionConfig);
                break;
            default:;
        }
    }
    if (outputDistanceShift) {
        float *pixel = NULL, *pixelsEnd = NULL;
        switch (mode) {
            case SINGLE:
            case PSEUDO:
                pixel = (float *) sdf;
                pixelsEnd = pixel+1*sdf.width()*sdf.height();
                break;
            case MULTI:
                pixel = (float *) msdf;
                pixelsEnd = pixel+3*msdf.width()*msdf.height();
                break;
            case MULTI_AND_TRUE:
                pixel = (float *) mtsdf;
                pixelsEnd = pixel+4*mtsdf.width()*mtsdf.height();
                break;
            default:;
        }
        while (pixel < pixelsEnd)
            *pixel++ += outputDistanceShift;
    }

    // Save output
    if (shapeExport) {
        FILE *file = fopen(shapeExport, "w");
        if (file) {
            writeShapeDescription(file, shape);
            fclose(file);
        } else
            puts("Failed to write shape export file.");
    }
    const char *error = NULL;
    switch (mode) {
        case SINGLE:
        case PSEUDO:
            error = writeOutput<1>(sdf, output, format);
            if (error)
                ABORT(error);
            if (is8bitFormat(format) && (testRenderMulti || testRender || estimateError))
                simulate8bit(sdf);
            if (estimateError) {
                double sdfError = estimateSDFError(sdf, shape, projection, SDF_ERROR_ESTIMATE_PRECISION, fillRule);
                printf("SDF error ~ %e\n", sdfError);
            }
            if (testRenderMulti) {
                Bitmap<float, 3> render(testWidthM, testHeightM);
                renderSDF(render, sdf, avgScale*range, .5f+outputDistanceShift);
                if (!savePng(render, testRenderMulti))
                    puts("Failed to write test render file.");
            }
            if (testRender) {
                Bitmap<float, 1> render(testWidth, testHeight);
                renderSDF(render, sdf, avgScale*range, .5f+outputDistanceShift);
                if (!savePng(render, testRender))
                    puts("Failed to write test render file.");
            }
            break;
        case MULTI:
            error = writeOutput<3>(msdf, output, format);
            if (error)
                ABORT(error);
            if (is8bitFormat(format) && (testRenderMulti || testRender || estimateError))
                simulate8bit(msdf);
            if (estimateError) {
                double sdfError = estimateSDFError(msdf, shape, projection, SDF_ERROR_ESTIMATE_PRECISION, fillRule);
                printf("SDF error ~ %e\n", sdfError);
            }
            if (testRenderMulti) {
                Bitmap<float, 3> render(testWidthM, testHeightM);
                renderSDF(render, msdf, avgScale*range, .5f+outputDistanceShift);
                if (!savePng(render, testRenderMulti))
                    puts("Failed to write test render file.");
            }
            if (testRender) {
                Bitmap<float, 1> render(testWidth, testHeight);
                renderSDF(render, msdf, avgScale*range, .5f+outputDistanceShift);
                if (!savePng(render, testRender))
                    ABORT("Failed to write test render file.");
            }
            break;
        case MULTI_AND_TRUE:
            error = writeOutput<4>(mtsdf, output, format);
            if (error)
                ABORT(error);
            if (is8bitFormat(format) && (testRenderMulti || testRender || estimateError))
                simulate8bit(mtsdf);
            if (estimateError) {
                double sdfError = estimateSDFError(mtsdf, shape, projection, SDF_ERROR_ESTIMATE_PRECISION, fillRule);
                printf("SDF error ~ %e\n", sdfError);
            }
            if (testRenderMulti) {
                Bitmap<float, 4> render(testWidthM, testHeightM);
                renderSDF(render, mtsdf, avgScale*range, .5f+outputDistanceShift);
                if (!savePng(render, testRenderMulti))
                    puts("Failed to write test render file.");
            }
            if (testRender) {
                Bitmap<float, 1> render(testWidth, testHeight);
                renderSDF(render, mtsdf, avgScale*range, .5f+outputDistanceShift);
                if (!savePng(render, testRender))
                    ABORT("Failed to write test render file.");
            }
            break;
        default:;
    }

    return 0;
}

#endif
