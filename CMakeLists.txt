
cmake_minimum_required(VERSION 3.15)
include(cmake/version.cmake)

option(MSDFGEN_CORE_ONLY "Only build the core library with no dependencies" OFF)
option(MSDFGEN_BUILD_STANDALONE "Build the msdfgen standalone executable" ON)
option(MSDFGEN_USE_VCPKG "Use vcpkg package manager to link project dependencies" ON)
option(MSDFGEN_USE_OPENMP "Build with OpenMP support for multithreaded code" OFF)
option(MSDFGEN_USE_CPP11 "Build with C++11 enabled" ON)
option(MSDFGEN_USE_SKIA "Build with the Skia library" ON)
option(MSDFGEN_INSTALL "Generate installation target" OFF)

if(MSDFGEN_CORE_ONLY AND MSDFGEN_BUILD_STANDALONE)
    message(WARNING "Option MSDFGEN_CORE_ONLY ignored - extensions are required for standalone executable")
    set(MSDFGEN_CORE_ONLY OFF)
endif()
if(MSDFGEN_CORE_ONLY AND MSDFGEN_USE_VCPKG)
    message(STATUS "Option MSDFGEN_USE_VCPKG ignored due to MSDFGEN_CORE_ONLY - core has no dependencies")
    set(MSDFGEN_USE_VCPKG OFF)
endif()

if(MSDFGEN_USE_VCPKG)
    # Make sure that vcpkg toolchain file is set
    if(NOT CMAKE_TOOLCHAIN_FILE)
        if(DEFINED ENV{VCPKG_ROOT})
            set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        else()
            message(SEND_ERROR "Vcpkg toolchain not configured. Either set VCPKG_ROOT environment variable or pass -DCMAKE_TOOLCHAIN_FILE=VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake to cmake")
        endif()
    endif()
    # Default to statically linked vcpkg triplet on Windows
    if(WIN32 AND NOT VCPKG_TARGET_TRIPLET)
        if(${CMAKE_GENERATOR_PLATFORM} MATCHES "64$")
            set(VCPKG_TARGET_TRIPLET "x64-windows-static")
        elseif(${CMAKE_GENERATOR_PLATFORM} MATCHES "32$" OR ${CMAKE_GENERATOR_PLATFORM} STREQUAL "x86")
            set(VCPKG_TARGET_TRIPLET "x86-windows-static")
        else()
            message(WARNING "Vcpkg triplet not explicitly specified and could not be deduced. Recommend using -DVCPKG_TARGET_TRIPLET=x86-windows-static or similar")
        endif()
    endif()
endif()

# Version is specified in vcpkg.json
project(msdfgen VERSION ${MSDFGEN_VERSION} LANGUAGES CXX)

file(GLOB_RECURSE MSDFGEN_CORE_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "core/*.h" "core/*.hpp")
file(GLOB_RECURSE MSDFGEN_CORE_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "core/*.cpp")
file(GLOB_RECURSE MSDFGEN_EXT_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "ext/*.h" "ext/*.hpp")
file(GLOB_RECURSE MSDFGEN_EXT_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "ext/*.cpp" "lib/*.cpp")

# Core library
add_library(msdfgen-core "${CMAKE_CURRENT_SOURCE_DIR}/msdfgen.h" ${MSDFGEN_CORE_HEADERS} ${MSDFGEN_CORE_SOURCES})
add_library(msdfgen::msdfgen-core ALIAS msdfgen-core)
set_target_properties(msdfgen-core PROPERTIES PUBLIC_HEADER "${MSDFGEN_CORE_HEADERS}")
set_property(TARGET msdfgen-core PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_compile_definitions(msdfgen-core PUBLIC
    MSDFGEN_VERSION=${MSDFGEN_VERSION}
    MSDFGEN_VERSION_MAJOR=${MSDFGEN_VERSION_MAJOR}
    MSDFGEN_VERSION_MINOR=${MSDFGEN_VERSION_MINOR}
    MSDFGEN_VERSION_REVISION=${MSDFGEN_VERSION_REVISION}
    MSDFGEN_COPYRIGHT_YEAR=${MSDFGEN_COPYRIGHT_YEAR}
)
target_include_directories(msdfgen-core INTERFACE
    $<INSTALL_INTERFACE:include/msdfgen>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT msdfgen-core)

if(MSDFGEN_USE_CPP11)
    target_compile_features(msdfgen-core PUBLIC cxx_std_11)
    target_compile_definitions(msdfgen-core PUBLIC MSDFGEN_USE_CPP11)
endif()

if(MSDFGEN_USE_OPENMP)
    # Note: Clang doesn't support OpenMP by default...
    find_package(OpenMP REQUIRED COMPONENTS CXX)
    target_compile_definitions(msdfgen-core PUBLIC MSDFGEN_USE_OPENMP)
    target_link_libraries(msdfgen-core PUBLIC OpenMP::OpenMP_CXX)
endif()

# Extensions library
if(NOT MSDFGEN_CORE_ONLY)
    if(NOT TARGET Freetype::Freetype)
        find_package(Freetype REQUIRED)
    endif()
    if(NOT TARGET tinyxml2::tinyxml2)
        find_package(tinyxml2 REQUIRED)
    endif()
    if(NOT TARGET PNG::PNG)
        find_package(PNG REQUIRED)
    endif()

    add_library(msdfgen-ext "${CMAKE_CURRENT_SOURCE_DIR}/msdfgen-ext.h" ${MSDFGEN_EXT_HEADERS} ${MSDFGEN_EXT_SOURCES})
    add_library(msdfgen::msdfgen-ext ALIAS msdfgen-ext)
    set_target_properties(msdfgen-ext PROPERTIES PUBLIC_HEADER "${MSDFGEN_EXT_HEADERS}")
    set_property(TARGET msdfgen-ext PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_definitions(msdfgen-ext PUBLIC MSDFGEN_USE_LIBPNG)
    target_link_libraries(msdfgen-ext PRIVATE msdfgen::msdfgen-core Freetype::Freetype tinyxml2::tinyxml2 PNG::PNG)
    target_include_directories(msdfgen-ext
        PUBLIC
            $<INSTALL_INTERFACE:include/msdfgen>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT msdfgen-ext)

    if(MSDFGEN_USE_SKIA)
        if(NOT TARGET skia)
            find_package(skia REQUIRED)
        endif()
        target_compile_definitions(msdfgen-ext PUBLIC MSDFGEN_USE_SKIA)
        target_link_libraries(msdfgen-ext PRIVATE skia)
    endif()

    add_library(msdfgen-full INTERFACE)
    add_library(msdfgen::msdfgen ALIAS msdfgen-full)
    target_link_libraries(msdfgen-full INTERFACE msdfgen::msdfgen-core msdfgen::msdfgen-ext)
endif()

# Standalone executable
if(MSDFGEN_BUILD_STANDALONE)
    set(MSDFGEN_STANDALONE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
    if(MSVC)
        set(MSDFGEN_STANDALONE_SOURCES ${MSDFGEN_STANDALONE_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/msdfgen.rc")
    endif()
    add_executable(msdfgen ${MSDFGEN_STANDALONE_SOURCES})
    target_compile_definitions(msdfgen PUBLIC MSDFGEN_STANDALONE)
    target_compile_definitions(msdfgen PRIVATE MSDFGEN_VERSION_UNDERLINE=${MSDFGEN_VERSION_UNDERLINE})
    set_property(TARGET msdfgen PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(msdfgen PRIVATE msdfgen::msdfgen)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT msdfgen)
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
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/msdfgen.h" DESTINATION include/msdfgen)

    if(NOT MSDFGEN_CORE_ONLY)
        install(TARGETS msdfgen-ext EXPORT msdfgenTargets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/msdfgen/ext
        )
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/msdfgen-ext.h" DESTINATION include/msdfgen)
        install(TARGETS msdfgen-full EXPORT msdfgenTargets)
    endif()

    export(EXPORT msdfgenTargets NAMESPACE msdfgen:: FILE "${CMAKE_CURRENT_BINARY_DIR}/msdfgenTargets.cmake")
    install(EXPORT msdfgenTargets FILE msdfgenTargets.cmake NAMESPACE msdfgen:: DESTINATION ${MSDFGEN_CONFIG_PATH})

    if(MSDFGEN_BUILD_STANDALONE)
        install(TARGETS msdfgen EXPORT msdfgenBinaryTargets DESTINATION ${CMAKE_INSTALL_BINDIR})
        export(EXPORT msdfgenBinaryTargets NAMESPACE msdfgen-standalone:: FILE "${CMAKE_CURRENT_BINARY_DIR}/msdfgenBinaryTargets.cmake")
        install(EXPORT msdfgenBinaryTargets FILE msdfgenBinaryTargets.cmake NAMESPACE msdfgen-standalone:: DESTINATION ${MSDFGEN_CONFIG_PATH})
    endif()

    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${MSDFGEN_CONFIG_PATH}/msdfgenConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/msdfgenConfigVersion.cmake"
        DESTINATION ${MSDFGEN_CONFIG_PATH}
    )
endif()
