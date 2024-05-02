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

#include "../msdfgen_c.h"
#include "../msdfgen.h"

#include <utility>

namespace {
    msdf_allocator_t g_allocator = {malloc, realloc, free};

    template<typename T, typename... TArgs>
    [[nodiscard]] auto _new(TArgs&&... args) noexcept -> T* {
        auto* memory = static_cast<T*>(g_allocator.alloc_callback(sizeof(T)));
        new(memory) T(std::forward<TArgs>(args)...);
        return memory;
    }

    auto _delete(void* memory) noexcept -> void {
        g_allocator.free_callback(memory);
    }
}// namespace

extern "C" {

// msdf_allocator

MSDF_API void msdf_allocator_set(const msdf_allocator_t* allocator) {
    g_allocator = *allocator;
}

MSDF_API const msdf_allocator_t* msdf_allocator_get() {
    return &g_allocator;
}

// msdf_bitmap

MSDF_API int msdf_bitmap_alloc(int type, int num_channels, int width, int height, msdf_bitmap_handle* bitmap) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_bitmap_get_type(msdf_bitmap_handle bitmap, int* type) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_bitmap_get_channel_count(msdf_bitmap_handle bitmap, int* channel_count) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_bitmap_get_width(msdf_bitmap_handle bitmap, int* width) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_bitmap_get_height(msdf_bitmap_handle bitmap, int* height) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_bitmap_get_pixels(msdf_bitmap_handle bitmap, void** pixels) {
    return MSDF_SUCCESS;
}

MSDF_API void msdf_bitmap_free(msdf_bitmap_handle bitmap) {
}

// msdf_shape

MSDF_API int msdf_shape_alloc(msdf_shape_handle* shape) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_get_bounds(msdf_shape_handle shape, msdf_bounds_t* bounds) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_add_contour(msdf_shape_handle shape, msdf_contour_handle* contour) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_get_contour_count(msdf_shape_handle shape, int* contour_count) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_get_contours(msdf_shape_handle shape, msdf_contour_handle* contours) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_get_edge_counts(msdf_shape_handle shape, int* edge_count) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_has_inverse_y_axis(msdf_shape_handle shape, int* inverse_y_axis) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_normalize(msdf_shape_handle shape) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_validate(msdf_shape_handle shape, int* result) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_bound(msdf_shape_handle shape, msdf_bounds_t* bounds) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_bound_miters(msdf_shape_handle shape, msdf_bounds_t* bounds, double border, double miterLimit, int polarity) {
    return MSDF_SUCCESS;
}

MSDF_API void msdf_shape_free(msdf_shape_handle shape) {
}

// msdf_contour

MSDF_API int msdf_contour_alloc(msdf_contour_handle* contour) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_contour_add_edge(msdf_contour_handle contour, msdf_edge_holder_handle* edge) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_contour_get_edge(msdf_contour_handle contour, int index, msdf_edge_holder_handle* edge) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_contour_get_edge_count(msdf_contour_handle contour, int* edge_count) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_contour_get_edges(msdf_contour_handle contour, msdf_edge_holder_handle* edges) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_contour_bound(msdf_contour_handle contour, msdf_bounds_t* bounds) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_contour_bound_miters(msdf_contour_handle contour, msdf_bounds_t* bounds, double border, double miterLimit, int polarity) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_contour_get_winding(msdf_contour_handle contour, int* winding) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_contour_reverse(msdf_contour_handle contour) {
    return MSDF_SUCCESS;
}

MSDF_API void msdf_contour_free(msdf_contour_handle contour) {
}

// msdf_edge

MSDF_API int msdf_edge_alloc(msdf_edge_holder_handle* edge) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_edge_add_segment(msdf_edge_holder_handle edge, msdf_segment_handle segment) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_edge_get_segment(msdf_edge_holder_handle edge, int index, msdf_segment_handle* segment) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_edge_get_segment_count(msdf_edge_holder_handle edge, int* segment_count) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_edge_get_segments(msdf_edge_holder_handle edge, msdf_segment_handle* segments) {
    return MSDF_SUCCESS;
}

MSDF_API void msdf_edge_free(msdf_edge_holder_handle edge) {
}

// msdf_segment

MSDF_API int msdf_segment_alloc(int type, msdf_segment_handle* segment) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_segment_get_type(msdf_segment_handle segment, int* type) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_segment_get_point_count(msdf_segment_handle segment, int* point_count) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_segment_get_points(msdf_segment_handle segment, msdf_vector2_t const** points) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_segment_get_point(msdf_segment_handle segment, int index, msdf_vector2_t* point) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_segment_set_point(msdf_segment_handle segment, int index, const msdf_vector2_t* point) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_segment_set_color(msdf_segment_handle segment, int color) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_segment_get_color(msdf_segment_handle segment, int* color) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_segment_get_direction(msdf_segment_handle segment, double param, msdf_vector2_t* direction) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_segment_get_direction_change(msdf_segment_handle segment, double param, msdf_vector2_t* direction_change) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_segment_point(msdf_segment_handle segment, double param, msdf_vector2_t* point) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_segment_bound(msdf_segment_handle segment, msdf_bounds_t* bounds) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_segment_move_start_point(msdf_segment_handle segment, const msdf_vector2_t* point) {
    return MSDF_SUCCESS;
}

MSDF_API int msdf_segment_move_end_point(msdf_segment_handle segment, const msdf_vector2_t* point) {
    return MSDF_SUCCESS;
}

MSDF_API void msdf_segment_free(msdf_segment_handle segment) {
}
}