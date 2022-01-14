
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SKIA_LIB_DIR "${CMAKE_CURRENT_LIST_DIR}/win64")
    else()
        set(SKIA_LIB_DIR "${CMAKE_CURRENT_LIST_DIR}/win32")
    endif()

    add_library(Skia::Skia STATIC IMPORTED)
    
    set_target_properties(Skia::Skia PROPERTIES
        IMPORTED_LOCATION "${SKIA_LIB_DIR}/release/skia.lib"
        IMPORTED_LOCATION_DEBUG "${SKIA_LIB_DIR}/debug/skia.lib"
        IMPORTED_CONFIGURATIONS "RELEASE;DEBUG"
    )
    target_include_directories(Skia::Skia INTERFACE
        "${CMAKE_CURRENT_LIST_DIR}/include"
        "${CMAKE_CURRENT_LIST_DIR}/include/core"
        "${CMAKE_CURRENT_LIST_DIR}/include/config"
    )
endif()
