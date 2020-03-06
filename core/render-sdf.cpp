
#include "render-sdf.h"

#include "arithmetics.hpp"
#include "pixel-conversion.hpp"

namespace msdfgen {

template <typename T, int N>
static void sample(T *output, const BitmapConstRef<T, N> &bitmap, Point2 pos) {
    double x = pos.x*bitmap.width-.5;
    double y = pos.y*bitmap.height-.5;
    int l = (int) floor(x);
    int b = (int) floor(y);
    int r = l+1;
    int t = b+1;
    double lr = x-l;
    double bt = y-b;
    l = clamp(l, bitmap.width-1), r = clamp(r, bitmap.width-1);
    b = clamp(b, bitmap.height-1), t = clamp(t, bitmap.height-1);
    for (int i = 0; i < N; ++i)
        output[i] = mix(mix(bitmap(l, b)[i], bitmap(r, b)[i], lr), mix(bitmap(l, t)[i], bitmap(r, t)[i], lr), bt);
}

static float distVal(float dist, double pxRange) {
    if (!pxRange)
        return (float) (dist > .5f);
    return (float) clamp((dist-.5f)*pxRange+.5);
}

void renderSDF(const BitmapRef<float, 1> &output, const BitmapConstRef<float, 1> &sdf, double pxRange) {
    pxRange *= (double) (output.width+output.height)/(sdf.width+sdf.height);
    for (int y = 0; y < output.height; ++y)
        for (int x = 0; x < output.width; ++x) {
            float sd;
            sample(&sd, sdf, Point2((x+.5)/output.width, (y+.5)/output.height));
            *output(x, y) = distVal(sd, pxRange);
        }
}

void renderSDF(const BitmapRef<float, 3> &output, const BitmapConstRef<float, 1> &sdf, double pxRange) {
    pxRange *= (double) (output.width+output.height)/(sdf.width+sdf.height);
    for (int y = 0; y < output.height; ++y)
        for (int x = 0; x < output.width; ++x) {
            float sd;
            sample(&sd, sdf, Point2((x+.5)/output.width, (y+.5)/output.height));
            float v = distVal(sd, pxRange);
            output(x, y)[0] = v;
            output(x, y)[1] = v;
            output(x, y)[2] = v;
        }
}

void renderSDF(const BitmapRef<float, 1> &output, const BitmapConstRef<float, 3> &sdf, double pxRange) {
    pxRange *= (double) (output.width+output.height)/(sdf.width+sdf.height);
    for (int y = 0; y < output.height; ++y)
        for (int x = 0; x < output.width; ++x) {
            float sd[3];
            sample(sd, sdf, Point2((x+.5)/output.width, (y+.5)/output.height));
            *output(x, y) = distVal(median(sd[0], sd[1], sd[2]), pxRange);
        }
}

void renderSDF(const BitmapRef<float, 3> &output, const BitmapConstRef<float, 3> &sdf, double pxRange) {
    pxRange *= (double) (output.width+output.height)/(sdf.width+sdf.height);
    for (int y = 0; y < output.height; ++y)
        for (int x = 0; x < output.width; ++x) {
            float sd[3];
            sample(sd, sdf, Point2((x+.5)/output.width, (y+.5)/output.height));
            output(x, y)[0] = distVal(sd[0], pxRange);
            output(x, y)[1] = distVal(sd[1], pxRange);
            output(x, y)[2] = distVal(sd[2], pxRange);
        }
}

void renderSDF(const BitmapRef<float, 1> &output, const BitmapConstRef<float, 4> &sdf, double pxRange) {
    pxRange *= (double) (output.width+output.height)/(sdf.width+sdf.height);
    for (int y = 0; y < output.height; ++y)
        for (int x = 0; x < output.width; ++x) {
            float sd[4];
            sample(sd, sdf, Point2((x+.5)/output.width, (y+.5)/output.height));
            *output(x, y) = distVal(median(sd[0], sd[1], sd[2]), pxRange);
        }
}

void renderSDF(const BitmapRef<float, 4> &output, const BitmapConstRef<float, 4> &sdf, double pxRange) {
    pxRange *= (double) (output.width+output.height)/(sdf.width+sdf.height);
    for (int y = 0; y < output.height; ++y)
        for (int x = 0; x < output.width; ++x) {
            float sd[4];
            sample(sd, sdf, Point2((x+.5)/output.width, (y+.5)/output.height));
            output(x, y)[0] = distVal(sd[0], pxRange);
            output(x, y)[1] = distVal(sd[1], pxRange);
            output(x, y)[2] = distVal(sd[2], pxRange);
            output(x, y)[3] = distVal(sd[3], pxRange);
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
