/*
* MULTI-CHANNEL SIGNED DISTANCE FIELD GENERATOR
 * ---------------------------------------------
 * A utility by Viktor Chlumsky, (c) 2014 - 2024
 *
 * The technique used to generate multi-channel distance fields in this code
 * has been developed by Viktor Chlumsky in 2014 for his master's thesis,
 * "Shape Decomposition for Multi-Channel Distance Fields". It provides improved
 * quality of sharp corners in glyphs and other 2D shapes compared to monochrome
 * distance fields. To reconstruct an image of the shape, apply the median of
 * three operation on the triplet of sampled signed distance values.
 *
 */

#include "../msdfgen-ext-c.h"
#include "../msdfgen-ext.h"

extern "C" {

MSDF_API int msdf_ft_init(msdf_ft_handle* handle) {
    if(handle == nullptr) {
        return MSDF_ERR_INVALID_ARG;
    }
    *handle = reinterpret_cast<msdf_ft_handle>(msdfgen::initializeFreetype());
    return MSDF_SUCCESS;
}

MSDF_API int msdf_ft_font_load(msdf_ft_handle handle, const char* filename, msdf_ft_font_handle* font) {
    if(handle == nullptr || filename == nullptr || font == nullptr) {
        return MSDF_ERR_INVALID_ARG;
    }
    *font = reinterpret_cast<msdf_ft_font_handle>(msdfgen::loadFont(reinterpret_cast<msdfgen::FreetypeHandle*>(handle), filename));
    return MSDF_SUCCESS;
}

MSDF_API int msdf_ft_font_load_data(msdf_ft_handle handle, const void* data, const size_t size, msdf_ft_font_handle* font) {
    if(handle == nullptr || data == nullptr || font == nullptr) {
        return MSDF_ERR_INVALID_ARG;
    }
    *font = reinterpret_cast<msdf_ft_font_handle>(msdfgen::loadFontData(reinterpret_cast<msdfgen::FreetypeHandle*>(handle),
                                                                        static_cast<const msdfgen::byte*>(data), static_cast<int>(size)));
    return MSDF_SUCCESS;
}

MSDF_API int msdf_ft_font_load_glyph(msdf_ft_font_handle font, const unsigned cp, msdf_shape_handle* shape) {
    if(font == nullptr || shape == nullptr) {
        return MSDF_ERR_INVALID_ARG;
    }
    auto* actual_shape = new msdfgen::Shape();
    msdfgen::loadGlyph(*actual_shape, reinterpret_cast<msdfgen::FontHandle*>(font), cp);
    *shape = reinterpret_cast<msdf_shape_handle>(actual_shape);
    return MSDF_SUCCESS;
}

MSDF_API void msdf_ft_font_destroy(msdf_ft_handle handle) {
    if(handle == nullptr) {
        return;
    }
    msdfgen::destroyFont(reinterpret_cast<msdfgen::FontHandle*>(handle));
}

MSDF_API void msdf_ft_deinit(msdf_ft_handle handle) {
    if(handle == nullptr) {
        return;
    }
    msdfgen::deinitializeFreetype(reinterpret_cast<msdfgen::FreetypeHandle*>(handle));
}
}