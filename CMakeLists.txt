cmake_minimum_required(VERSION 2.8.12)

project(msdfgen)

find_package(Freetype REQUIRED)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if (COMPILER_SUPPORTS_CXX11)
	add_definitions(-DMSDFGEN_USE_CPP11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# Make release mode default (turn on optimizations)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()


# Note: Clang doesn't support openMP by default...
#find_package(OpenMP)
#if (OPENMP_FOUND)
#	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
#	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
#endif()

#----------------------------------------------------------------
# Support Functions
#----------------------------------------------------------------

# Mirror the folder structure for sources inside the IDE...
function(folderize_sources sources prefix)
	foreach(FILE ${${sources}}) 
	  get_filename_component(PARENT_DIR "${FILE}" PATH)

	  # skip src or include and changes /'s to \\'s
	  string(REPLACE "${prefix}" "" GROUP "${PARENT_DIR}")
	  string(REPLACE "/" "\\" GROUP "${GROUP}")

      # If it's got a path, then append a "\\" separator (otherwise leave it blank)
	  if ("${GROUP}" MATCHES ".+")
	  	set(GROUP "\\${GROUP}")
	  endif()

	  source_group("${GROUP}" FILES "${FILE}")
	endforeach()
endfunction(folderize_sources)



file(GLOB_RECURSE msdfgen_HEADERS
	"core/*.h"
	"lib/*.h"
	"ext/*.h"
	"include/*.h"
)

file(GLOB_RECURSE msdfgen_SOURCES
	"core/*.cpp"
	"lib/*.cpp"
	"ext/*.cpp"
)

include_directories(${FREETYPE_INCLUDE_DIRS})
include_directories("include")

# Build the library (aliased name because it's the same target name the exe)
folderize_sources(msdfgen_HEADERS ${CMAKE_SOURCE_DIR})
folderize_sources(msdfgen_SOURCES ${CMAKE_SOURCE_DIR})

add_library(lib_msdfgen ${msdfgen_SOURCES} ${msdfgen_HEADERS})
set_target_properties(lib_msdfgen PROPERTIES OUTPUT_NAME msdfgen)
target_link_libraries(lib_msdfgen ${FREETYPE_LIBRARIES})

# Build the executable

add_executable(msdfgen main.cpp)
target_compile_definitions(msdfgen PRIVATE MSDFGEN_STANDALONE)
target_link_libraries(msdfgen lib_msdfgen)
