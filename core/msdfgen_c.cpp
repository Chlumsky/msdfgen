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
    using SDFBitmap = msdfgen::Bitmap<float>;
    using SDFBitmapRef = msdfgen::BitmapRef<float>;
    using PSDFBitmap = msdfgen::Bitmap<float>;
    using PSDFBitmapRef = msdfgen::BitmapRef<float>;
    using MSDFBitmap = msdfgen::Bitmap<float, 3>;
    using MSDFBitmapRef = msdfgen::BitmapRef<float, 3>;
    using MTSDFBitmap = msdfgen::Bitmap<float, 4>;
    using MTSDFBitmapRef = msdfgen::BitmapRef<float, 4>;

    msdf_allocator_t g_allocator = {malloc, realloc, free};

    template<typename T>
    [[nodiscard]] auto msdf_alloc(const size_t count = 1) noexcept -> T* {
        return static_cast<T*>(g_allocator.alloc_callback(sizeof(T) * count));
    }

    auto msdf_free(void* memory) {
        g_allocator.free_callback(memory);
    }

    template<typename T, typename... TArgs>
    [[nodiscard]] auto msdf_new(TArgs&&... args) noexcept -> T* {
        auto* memory = static_cast<T*>(g_allocator.alloc_callback(sizeof(T)));
        new(memory) T(std::forward<TArgs>(args)...);
        return memory;
    }

    template<typename T>
    auto msdf_delete(T* memory) noexcept -> void {
        if(memory == nullptr) {
            return;
        }
        memory->~T();
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

MSDF_API int msdf_bitmap_alloc(const int type, const int width, const int height, msdf_bitmap_t** bitmap) {
    if(width < 0 || height < 0) {
        return MSDF_ERR_INVALID_SIZE;
    }
    if(bitmap == nullptr) {
        return MSDF_ERR_INVALID_ARG;
    }
    auto* wrapper = msdf_alloc<msdf_bitmap_t>();
    wrapper->type = type;
    wrapper->width = width;
    wrapper->height = height;
    switch(type) {
        case MSDF_BITMAP_TYPE_SDF:
            wrapper->handle = msdf_new<SDFBitmap>(width, height);
            break;
        case MSDF_BITMAP_TYPE_PSDF:
            wrapper->handle = msdf_new<PSDFBitmap>(width, height);
            break;
        case MSDF_BITMAP_TYPE_MSDF:
            wrapper->handle = msdf_new<MSDFBitmap>(width, height);
            break;
        case MSDF_BITMAP_TYPE_MTSDF:
            wrapper->handle = msdf_new<MTSDFBitmap>(width, height);
            break;
        default:
            return MSDF_ERR_INVALID_ARG;
    }
    *bitmap = wrapper;
    return MSDF_SUCCESS;
}

MSDF_API int msdf_bitmap_get_channel_count(const msdf_bitmap_t* bitmap, int* channel_count) {
    if(bitmap == nullptr || channel_count == nullptr) {
        return MSDF_ERR_INVALID_ARG;
    }
    switch(bitmap->type) {
        case MSDF_BITMAP_TYPE_MSDF:
            *channel_count = 3;
            break;
        case MSDF_BITMAP_TYPE_MTSDF:
            *channel_count = 4;
            break;
        default:
            *channel_count = 1;
            break;
    }
    return MSDF_SUCCESS;
}

MSDF_API int msdf_bitmap_get_size(const msdf_bitmap_t* bitmap, size_t* size) {
    if(bitmap == nullptr || size == nullptr) {
        return MSDF_ERR_INVALID_ARG;
    }
    int channel_count;
    if(msdf_bitmap_get_channel_count(bitmap, &channel_count) != MSDF_SUCCESS) {
        return MSDF_ERR_FAILED;
    }
    // << 2 because we only support floats right now, sizeof(float) is always 4
    *size = static_cast<size_t>(bitmap->width) * static_cast<size_t>(bitmap->height) * static_cast<size_t>(channel_count) << 2;
    return MSDF_SUCCESS;
}

MSDF_API int msdf_bitmap_get_pixels(const msdf_bitmap_t* bitmap, void** pixels) {
    if(bitmap == nullptr || pixels == nullptr) {
        return MSDF_ERR_INVALID_ARG;
    }
    switch(bitmap->type) {
        case MSDF_BITMAP_TYPE_SDF:
            *pixels = static_cast<SDFBitmapRef>(*static_cast<SDFBitmap*>(bitmap->handle)).pixels;
            break;
        case MSDF_BITMAP_TYPE_PSDF:
            *pixels = static_cast<PSDFBitmapRef>(*static_cast<PSDFBitmap*>(bitmap->handle)).pixels;
            break;
        case MSDF_BITMAP_TYPE_MSDF:
            *pixels = static_cast<MSDFBitmapRef>(*static_cast<MSDFBitmap*>(bitmap->handle)).pixels;
            break;
        case MSDF_BITMAP_TYPE_MTSDF:
            *pixels = static_cast<MTSDFBitmapRef>(*static_cast<MTSDFBitmap*>(bitmap->handle)).pixels;
            break;
        default:
            return MSDF_ERR_INVALID_TYPE;
    }
    return MSDF_SUCCESS;
}

MSDF_API void msdf_bitmap_free(msdf_bitmap_t* bitmap) {
    if(bitmap == nullptr) {
        return;
    }
    switch(bitmap->type) {
        case MSDF_BITMAP_TYPE_SDF:
            msdf_delete(static_cast<SDFBitmap*>(bitmap->handle));
            break;
        case MSDF_BITMAP_TYPE_PSDF:
            msdf_delete(static_cast<PSDFBitmap*>(bitmap->handle));
            break;
        case MSDF_BITMAP_TYPE_MSDF:
            msdf_delete(static_cast<MSDFBitmap*>(bitmap->handle));
            break;
        case MSDF_BITMAP_TYPE_MTSDF:
            msdf_delete(static_cast<MTSDFBitmap*>(bitmap->handle));
            break;
        default:
            return;
    }
    msdf_free(bitmap);
}

// msdf_shape

MSDF_API int msdf_shape_alloc(msdf_shape_handle* shape) {
    if(shape == nullptr) {
        return MSDF_ERR_INVALID_ARG;
    }
    *shape = reinterpret_cast<msdf_shape_handle>(msdf_new<msdfgen::Shape>());
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_get_bounds(msdf_shape_handle shape, msdf_bounds_t* bounds) {
    if(shape == nullptr || bounds == nullptr) {
        return MSDF_ERR_INVALID_ARG;
    }
    const auto src_bounds = reinterpret_cast<msdfgen::Shape*>(shape)->getBounds();
    memcpy(bounds, &src_bounds, sizeof(msdfgen::Shape::Bounds));
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_add_contour(msdf_shape_handle shape, msdf_contour_handle* contour) {
    if(shape == nullptr || contour == nullptr) {
        return MSDF_ERR_INVALID_ARG;
    }
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_get_contour_count(msdf_shape_handle shape, size_t* contour_count) {
    if(shape == nullptr || contour_count == nullptr) {
        return MSDF_ERR_INVALID_ARG;
    }
    *contour_count = reinterpret_cast<msdfgen::Shape*>(shape)->contours.size();
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_get_contours(msdf_shape_handle shape, msdf_contour_handle* contours) {
    if(shape == nullptr || contours == nullptr) {
        return MSDF_ERR_INVALID_ARG;
    }
    *contours = reinterpret_cast<msdf_contour_handle>(reinterpret_cast<msdfgen::Shape*>(shape)->contours.data());
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_get_edge_counts(msdf_shape_handle shape, size_t* edge_count) {
    if(shape == nullptr || edge_count == nullptr) {
        return MSDF_ERR_INVALID_ARG;
    }
    *edge_count = reinterpret_cast<msdfgen::Shape*>(shape)->edgeCount();
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_has_inverse_y_axis(msdf_shape_handle shape, int* inverse_y_axis) {
    if(shape == nullptr || inverse_y_axis == nullptr) {
        return MSDF_ERR_INVALID_ARG;
    }
    *inverse_y_axis = reinterpret_cast<msdfgen::Shape*>(shape)->inverseYAxis ? MSDF_TRUE : MSDF_FALSE;
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_normalize(msdf_shape_handle shape) {
    if(shape == nullptr) {
        return MSDF_ERR_INVALID_ARG;
    }
    reinterpret_cast<msdfgen::Shape*>(shape)->normalize();
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_validate(msdf_shape_handle shape, int* result) {
    if(shape == nullptr || result == nullptr) {
        return MSDF_ERR_INVALID_ARG;
    }
    *result = reinterpret_cast<msdfgen::Shape*>(shape)->validate();
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_bound(msdf_shape_handle shape, msdf_bounds_t* bounds) {
    if(shape == nullptr || bounds == nullptr) {
        return MSDF_ERR_INVALID_ARG;
    }
    reinterpret_cast<msdfgen::Shape*>(shape)->bound(bounds->l, bounds->b, bounds->r, bounds->t);
    return MSDF_SUCCESS;
}

MSDF_API int msdf_shape_bound_miters(msdf_shape_handle shape, msdf_bounds_t* bounds, double border, double miterLimit, int polarity) {
    if(shape == nullptr || bounds == nullptr) {
        return MSDF_ERR_INVALID_ARG;
    }
    reinterpret_cast<msdfgen::Shape*>(shape)->boundMiters(bounds->l, bounds->b, bounds->r, bounds->t, border, miterLimit, polarity);
    return MSDF_SUCCESS;
}

MSDF_API void msdf_shape_free(msdf_shape_handle shape) {
    msdf_delete(reinterpret_cast<msdfgen::Shape*>(shape));
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