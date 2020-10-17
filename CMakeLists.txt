cmake_minimum_required(VERSION 3.10)

project(msdfgen VERSION 1.8 LANGUAGES CXX)
option(MSDFGEN_BUILD_MSDFGEN_STANDALONE "Build the msdfgen standalone executable" ON)
option(MSDFGEN_USE_OPENMP "Build with OpenMP support for multithreaded code" OFF)
option(MSDFGEN_USE_CPP11 "Build with C++11 enabled" ON)
option(MSDFGEN_USE_SKIA "Build with the Skia library" OFF)
option(FREETYPE_WITH_PNG "Link libpng and zlib because FreeType is configured to require it" OFF)
option(FREETYPE_WITH_HARFBUZZ "Link HarfBuzz because FreeType is configured to require it" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Freetype REQUIRED)

#----------------------------------------------------------------
# Gathering File
#----------------------------------------------------------------

file(GLOB_RECURSE msdfgen_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	"core/*.h"
	"core/*.hpp"
)

file(GLOB_RECURSE msdfgen_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
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
	"lib/*.cpp"
)

# Build the library (aliased name because it's the same target name the exe)
include(folderize)
folderize_sources(msdfgen_HEADERS ${CMAKE_SOURCE_DIR})
folderize_sources(msdfgen_SOURCES ${CMAKE_SOURCE_DIR})
folderize_sources(msdfgen-ext_PUBLIC_HEADERS ${CMAKE_SOURCE_DIR})
folderize_sources(msdfgen-ext_PRIVATE_HEADERS ${CMAKE_SOURCE_DIR})
folderize_sources(msdfgen-ext_SOURCES ${CMAKE_SOURCE_DIR})

#----------------------------------------------------------------
# Target configuration
#----------------------------------------------------------------

add_library(msdfgen ${msdfgen_SOURCES} ${msdfgen_HEADERS} "./msdfgen.h")
add_library(msdfgen::msdfgen ALIAS msdfgen)
set_target_properties(msdfgen PROPERTIES PUBLIC_HEADER "${msdfgen_HEADERS}")
target_include_directories(msdfgen INTERFACE
	$<INSTALL_INTERFACE:include>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
)

if(MSDFGEN_USE_CPP11)
	target_compile_features(msdfgen PUBLIC cxx_std_11)
	target_compile_definitions(msdfgen PUBLIC MSDFGEN_USE_CPP11)
endif()

if(MSDFGEN_USE_OPENMP)
	# Note: Clang doesn't support OpenMP by default...
	find_package(OpenMP REQUIRED COMPONENTS CXX)
	target_link_libraries(msdfgen PRIVATE OpenMP::OpenMP_CXX)
	target_compile_definitions(msdfgen PRIVATE MSDFGEN_USE_OPENMP)
endif()

if(MSDFGEN_USE_SKIA)
	find_package(Skia REQUIRED)
	target_link_libraries(msdfgen-ext PRIVATE Skia::Skia)
	target_compile_definitions(msdfgen-ext PUBLIC MSDFGEN_USE_SKIA)
endif()

add_library(msdfgen-ext ${msdfgen-ext_SOURCES} ${msdfgen-ext_PUBLIC_HEADERS} ${msdfgen-ext_PRIVATE_HEADERS} "./msdfgen-ext.h")
add_library(msdfgen::msdfgen-ext ALIAS msdfgen-ext)
set_target_properties(msdfgen-ext PROPERTIES
	PUBLIC_HEADER "${msdfgen-ext_PUBLIC_HEADERS}"
)
target_link_libraries(msdfgen-ext PUBLIC msdfgen::msdfgen Freetype::Freetype)
target_include_directories(msdfgen-ext
PUBLIC
	$<INSTALL_INTERFACE:include>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
	
PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSDFGEN_USE_CPP11)
	target_compile_features(msdfgen-ext PUBLIC cxx_std_11)
	target_compile_definitions(msdfgen-ext PUBLIC MSDFGEN_USE_CPP11)
endif()

# Build the executable if requested
if(MSDFGEN_BUILD_MSDFGEN_STANDALONE)
	add_executable(msdfgen-standalone main.cpp)
	set_target_properties(msdfgen-standalone PROPERTIES ARCHIVE_OUTPUT_DIRECTORY archive OUTPUT_NAME msdfgen)
	target_compile_definitions(msdfgen-standalone PRIVATE MSDFGEN_STANDALONE)
	target_link_libraries(msdfgen-standalone PRIVATE msdfgen::msdfgen msdfgen::msdfgen-ext)

	if(FREETYPE_WITH_PNG)
		find_package(ZLIB REQUIRED)
		find_package(PNG REQUIRED)
		target_link_libraries(msdfgen-standalone PRIVATE ZLIB::ZLIB PNG::PNG)
	endif()
	if(FREETYPE_WITH_HARFBUZZ)
		# No FindHarfBuzz.cmake available so falling back to CMAKE_LIBRARY_PATH
		find_library(FREETYPE_HARFBUZZ_LIB "harfbuzz")
		target_link_libraries(msdfgen-standalone PRIVATE "${FREETYPE_HARFBUZZ_LIB}")
	endif()
endif()

#----------------------------------------------------------------
# Installation and exportation of the libraries
#----------------------------------------------------------------

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

install(TARGETS msdfgen EXPORT msdfgenTargets
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	FRAMEWORK DESTINATION lib
	PUBLIC_HEADER DESTINATION include/msdfgen/core
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/msdfgen.h" "${CMAKE_CURRENT_SOURCE_DIR}/msdfgen-ext.h" DESTINATION include/msdfgen)

install(TARGETS msdfgen-ext EXPORT msdfgenTargets
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	FRAMEWORK DESTINATION lib
	PUBLIC_HEADER DESTINATION include/msdfgen/ext
)

if(MSDFGEN_BUILD_MSDFGEN_STANDALONE)
	install(TARGETS msdfgen-standalone EXPORT msdfgenTargets RUNTIME DESTINATION bin)
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
