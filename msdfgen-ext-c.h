#pragma once

/*
 * MULTI-CHANNEL SIGNED DISTANCE FIELD GENERATOR
 * ---------------------------------------------
 * A utility by Viktor Chlumsky, (c) 2014 - 2024
 *
 * The technique used to generate multi-channel distance fields in this code
 * has been developed by Viktor Chlumsky in 2014 for his master's thesis,
 * "Shape Decomposition for Multi-Channel Distance Fields". It provides improved
 * quality of sharp corners in glyphs and other 2D shapes compared to monochrome
 * distance fields. To reconstruct an image of the shape, apply the median of three
 * operation on the triplet of sampled signed distance values.
 */

#include "msdfgen-c.h"

MSDF_DEFINE_HANDLE_TYPE(msdf_ft);
MSDF_DEFINE_HANDLE_TYPE(msdf_ft_font);

#ifdef __cplusplus
extern "C" {
#endif

MSDF_API int msdf_ft_init(msdf_ft_handle* handle);

MSDF_API int msdf_ft_font_load(msdf_ft_handle handle, const char* filename, msdf_ft_font_handle* font);

MSDF_API int msdf_ft_font_load_data(msdf_ft_handle handle, const void* data, size_t size, msdf_ft_font_handle* font);

MSDF_API int msdf_ft_font_load_glyph(msdf_ft_font_handle font, unsigned cp, msdf_shape_handle* shape);

MSDF_API void msdf_ft_font_destroy(msdf_ft_handle handle);

MSDF_API void msdf_ft_deinit(msdf_ft_handle handle);

#ifdef __cplusplus
}
#endif