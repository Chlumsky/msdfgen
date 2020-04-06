cmake_minimum_required(VERSION 3.9)

project(msdfgen VERSION 1.7.1 LANGUAGES CXX)
option(MSDFGEN_BUILD_CMD_TOOLS "Build command line tools for msdfgen" ON)
option(MSDFGEN_USE_OPENMP "Build with OpenMP support for multithreaded code" OFF)

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
target_compile_features(msdfgen PUBLIC cxx_std_11)
target_include_directories(msdfgen INTERFACE
	$<INSTALL_INTERFACE:include>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
)

if(MSDFGEN_USE_OPENMP)
	# Note: Clang doesn't support OpenMP by default...
	find_package(OpenMP REQUIRED COMPONENTS CXX)
	target_link_libraries(msdfgen PRIVATE OpenMP::OpenMP_CXX)
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

# Build the executable if requested
if(MSDFGEN_BUILD_CMD_TOOLS)
	add_executable(msdfgentools main.cpp  )
	set_target_properties(msdfgentools PROPERTIES OUTPUT_NAME msdfgen)
	target_compile_definitions(msdfgentools PRIVATE MSDFGEN_STANDALONE)
	target_link_libraries(msdfgentools PRIVATE msdfgen::msdfgen msdfgen::msdfgen-ext)
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
