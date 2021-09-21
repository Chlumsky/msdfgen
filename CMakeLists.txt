cmake_minimum_required(VERSION 3.10)

project(msdfgen VERSION 1.9 LANGUAGES CXX)
option(MSDFGEN_BUILD_STANDALONE "Build the msdfgen standalone executable" ON)
option(MSDFGEN_USE_OPENMP "Build with OpenMP support for multithreaded code" OFF)
option(MSDFGEN_USE_CPP11 "Build with C++11 enabled" ON)
option(MSDFGEN_USE_SKIA "Build with the Skia library" OFF)
option(MSDFGEN_INSTALL "Generate installation target" ON)
option(FREETYPE_WITH_PNG "Link libpng and zlib because FreeType is configured to require it" OFF)
option(FREETYPE_WITH_HARFBUZZ "Link HarfBuzz because FreeType is configured to require it" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(NOT TARGET Freetype::Freetype)
    find_package(Freetype REQUIRED)
endif()

file(GLOB_RECURSE msdfgen-core_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "core/*.h"
    "core/*.hpp"
)

file(GLOB_RECURSE msdfgen-core_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "core/*.cpp"
)

file(GLOB_RECURSE msdfgen-ext_PUBLIC_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "ext/*.h"
    "ext/*.hpp"
)

file(GLOB_RECURSE msdfgen-ext_PRIVATE_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "include/*.h"
)

file(GLOB_RECURSE msdfgen-ext_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "ext/*.cpp"
    "lib/*.cpp"
)

include(folderize)
folderize_sources(msdfgen-core_HEADERS ${CMAKE_SOURCE_DIR})
folderize_sources(msdfgen-core_SOURCES ${CMAKE_SOURCE_DIR})
folderize_sources(msdfgen-ext_PUBLIC_HEADERS ${CMAKE_SOURCE_DIR})
folderize_sources(msdfgen-ext_PRIVATE_HEADERS ${CMAKE_SOURCE_DIR})
folderize_sources(msdfgen-ext_SOURCES ${CMAKE_SOURCE_DIR})

# Core library
add_library(msdfgen-core ${msdfgen-core_SOURCES} ${msdfgen-core_HEADERS} "./msdfgen.h")
add_library(msdfgen::msdfgen-core ALIAS msdfgen-core)
set_target_properties(msdfgen-core PROPERTIES PUBLIC_HEADER "${msdfgen-core_HEADERS}")
target_include_directories(msdfgen-core INTERFACE
    $<INSTALL_INTERFACE:include/msdfgen>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
)

if(MSDFGEN_USE_CPP11)
    target_compile_features(msdfgen-core PUBLIC cxx_std_11)
    target_compile_definitions(msdfgen-core PUBLIC MSDFGEN_USE_CPP11)
endif()

if(MSDFGEN_USE_OPENMP)
    # Note: Clang doesn't support OpenMP by default...
    find_package(OpenMP REQUIRED COMPONENTS CXX)
    target_link_libraries(msdfgen-core PUBLIC OpenMP::OpenMP_CXX)
    target_compile_definitions(msdfgen-core PUBLIC MSDFGEN_USE_OPENMP)
endif()

# Extensions library
add_library(msdfgen-ext ${msdfgen-ext_SOURCES} ${msdfgen-ext_PUBLIC_HEADERS} ${msdfgen-ext_PRIVATE_HEADERS} "./msdfgen-ext.h")
add_library(msdfgen::msdfgen-ext ALIAS msdfgen-ext)
set_target_properties(msdfgen-ext PROPERTIES PUBLIC_HEADER "${msdfgen-ext_PUBLIC_HEADERS}")
target_link_libraries(msdfgen-ext PUBLIC msdfgen::msdfgen-core Freetype::Freetype)
target_include_directories(msdfgen-ext
PUBLIC
    $<INSTALL_INTERFACE:include/msdfgen>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(FREETYPE_WITH_PNG)
    find_package(ZLIB REQUIRED)
    find_package(PNG REQUIRED)
    target_link_libraries(msdfgen-ext PUBLIC ZLIB::ZLIB PNG::PNG)
endif()
if(FREETYPE_WITH_HARFBUZZ)
    # No FindHarfBuzz.cmake available so falling back to CMAKE_LIBRARY_PATH
    find_library(FREETYPE_HARFBUZZ_LIB "harfbuzz")
    target_link_libraries(msdfgen-ext PUBLIC "${FREETYPE_HARFBUZZ_LIB}")
endif()

if(MSDFGEN_USE_SKIA)
    find_package(Skia REQUIRED)
    target_link_libraries(msdfgen-ext PUBLIC Skia::Skia)
    target_compile_definitions(msdfgen-ext PUBLIC MSDFGEN_USE_SKIA)
endif()

# Standalone executable
if(MSDFGEN_BUILD_STANDALONE)
    add_executable(msdfgen main.cpp)
    target_compile_definitions(msdfgen PUBLIC MSDFGEN_STANDALONE)
    target_link_libraries(msdfgen PUBLIC msdfgen::msdfgen-core msdfgen::msdfgen-ext)
endif()

# Installation
if(MSDFGEN_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
    set(MSDFGEN_CONFIG_PATH "lib/cmake/msdfgen")

    # install tree package config
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/msdfgenConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    configure_package_config_file(
        cmake/msdfgenConfig.cmake.in
        ${MSDFGEN_CONFIG_PATH}/msdfgenConfig.cmake
        INSTALL_DESTINATION ${MSDFGEN_CONFIG_PATH}
        NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )

    # build tree package config
    configure_file(
        cmake/msdfgenConfig.cmake.in
        msdfgenConfig.cmake
        @ONLY
    )

    install(TARGETS msdfgen-core EXPORT msdfgenTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/msdfgen/core
    )

    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/msdfgen.h" "${CMAKE_CURRENT_SOURCE_DIR}/msdfgen-ext.h" DESTINATION include/msdfgen)

    install(TARGETS msdfgen-ext EXPORT msdfgenTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/msdfgen/ext
    )

    if(MSDFGEN_BUILD_STANDALONE)
        install(TARGETS msdfgen EXPORT msdfgenTargets DESTINATION bin)
    endif()

    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${MSDFGEN_CONFIG_PATH}/msdfgenConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/msdfgenConfigVersion.cmake"
        DESTINATION ${MSDFGEN_CONFIG_PATH}
    )

    export(
        EXPORT msdfgenTargets
        NAMESPACE msdfgen::
        FILE "${CMAKE_CURRENT_BINARY_DIR}/msdfgenTargets.cmake"
    )

    install(
        EXPORT msdfgenTargets FILE msdfgenTargets.cmake
        NAMESPACE msdfgen::
        DESTINATION ${MSDFGEN_CONFIG_PATH}
    )
endif()
