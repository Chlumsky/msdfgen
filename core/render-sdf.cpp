
#include "render-sdf.h"

#include "arithmetics.hpp"
#include "pixel-conversion.hpp"
#include "bitmap-interpolation.hpp"

namespace msdfgen {

static float distVal(float dist, real pxRange, float midValue) {
    if (!pxRange)
        return (float) (dist > midValue);
    return (float) clamp((dist-midValue)*pxRange+real(.5));
}

void renderSDF(const BitmapRef<float, 1> &output, const BitmapConstRef<float, 1> &sdf, real pxRange, float midValue) {
    Vector2 scale(real(sdf.width)/real(output.width), real(sdf.height)/real(output.height));
    pxRange *= real(output.width+output.height)/real(sdf.width+sdf.height);
    for (int y = 0; y < output.height; ++y)
        for (int x = 0; x < output.width; ++x) {
            float sd;
            interpolate(&sd, sdf, scale*Point2(real(x)+real(.5), real(y)+real(.5)));
            *output(x, y) = distVal(sd, pxRange, midValue);
        }
}

void renderSDF(const BitmapRef<float, 3> &output, const BitmapConstRef<float, 1> &sdf, real pxRange, float midValue) {
    Vector2 scale(real(sdf.width)/real(output.width), real(sdf.height)/real(output.height));
    pxRange *= real(output.width+output.height)/real(sdf.width+sdf.height);
    for (int y = 0; y < output.height; ++y)
        for (int x = 0; x < output.width; ++x) {
            float sd;
            interpolate(&sd, sdf, scale*Point2(real(x)+real(.5), real(y)+real(.5)));
            float v = distVal(sd, pxRange, midValue);
            output(x, y)[0] = v;
            output(x, y)[1] = v;
            output(x, y)[2] = v;
        }
}

void renderSDF(const BitmapRef<float, 1> &output, const BitmapConstRef<float, 3> &sdf, real pxRange, float midValue) {
    Vector2 scale(real(sdf.width)/real(output.width), real(sdf.height)/real(output.height));
    pxRange *= real(output.width+output.height)/real(sdf.width+sdf.height);
    for (int y = 0; y < output.height; ++y)
        for (int x = 0; x < output.width; ++x) {
            float sd[3];
            interpolate(sd, sdf, scale*Point2(real(x)+real(.5), real(y)+real(.5)));
            *output(x, y) = distVal(median(sd[0], sd[1], sd[2]), pxRange, midValue);
        }
}

void renderSDF(const BitmapRef<float, 3> &output, const BitmapConstRef<float, 3> &sdf, real pxRange, float midValue) {
    Vector2 scale(real(sdf.width)/real(output.width), real(sdf.height)/real(output.height));
    pxRange *= real(output.width+output.height)/real(sdf.width+sdf.height);
    for (int y = 0; y < output.height; ++y)
        for (int x = 0; x < output.width; ++x) {
            float sd[3];
            interpolate(sd, sdf, scale*Point2(real(x)+real(.5), real(y)+real(.5)));
            output(x, y)[0] = distVal(sd[0], pxRange, midValue);
            output(x, y)[1] = distVal(sd[1], pxRange, midValue);
            output(x, y)[2] = distVal(sd[2], pxRange, midValue);
        }
}

void renderSDF(const BitmapRef<float, 1> &output, const BitmapConstRef<float, 4> &sdf, real pxRange, float midValue) {
    Vector2 scale(real(sdf.width)/real(output.width), real(sdf.height)/real(output.height));
    pxRange *= real(output.width+output.height)/real(sdf.width+sdf.height);
    for (int y = 0; y < output.height; ++y)
        for (int x = 0; x < output.width; ++x) {
            float sd[4];
            interpolate(sd, sdf, scale*Point2(real(x)+real(.5), real(y)+real(.5)));
            *output(x, y) = distVal(median(sd[0], sd[1], sd[2]), pxRange, midValue);
        }
}

void renderSDF(const BitmapRef<float, 4> &output, const BitmapConstRef<float, 4> &sdf, real pxRange, float midValue) {
    Vector2 scale(real(sdf.width)/real(output.width), real(sdf.height)/real(output.height));
    pxRange *= real(output.width+output.height)/real(sdf.width+sdf.height);
    for (int y = 0; y < output.height; ++y)
        for (int x = 0; x < output.width; ++x) {
            float sd[4];
            interpolate(sd, sdf, scale*Point2(real(x)+real(.5), real(y)+real(.5)));
            output(x, y)[0] = distVal(sd[0], pxRange, midValue);
            output(x, y)[1] = distVal(sd[1], pxRange, midValue);
            output(x, y)[2] = distVal(sd[2], pxRange, midValue);
            output(x, y)[3] = distVal(sd[3], pxRange, midValue);
        }
}

void simulate8bit(const BitmapRef<float, 1> &bitmap) {
    const float *end = bitmap.pixels+1*bitmap.width*bitmap.height;
    for (float *p = bitmap.pixels; p < end; ++p)
        *p = pixelByteToFloat(pixelFloatToByte(*p));
}

void simulate8bit(const BitmapRef<float, 3> &bitmap) {
    const float *end = bitmap.pixels+3*bitmap.width*bitmap.height;
    for (float *p = bitmap.pixels; p < end; ++p)
        *p = pixelByteToFloat(pixelFloatToByte(*p));
}

void simulate8bit(const BitmapRef<float, 4> &bitmap) {
    const float *end = bitmap.pixels+4*bitmap.width*bitmap.height;
    for (float *p = bitmap.pixels; p < end; ++p)
        *p = pixelByteToFloat(pixelFloatToByte(*p));
}

}
