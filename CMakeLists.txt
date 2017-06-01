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


file(GLOB_RECURSE msdfgen_SOURCES
	"core/*.cpp"
	"lib/*.cpp"
	"ext/*.cpp"
)

include_directories(${FREETYPE_INCLUDE_DIRS})
include_directories("include")

# Build the library (aliased name because it's the same target name the exe)

add_library(lib_msdfgen ${msdfgen_SOURCES})
set_target_properties(lib_msdfgen PROPERTIES OUTPUT_NAME msdfgen)
target_link_libraries(lib_msdfgen ${FREETYPE_LIBRARIES})

# Build the executable

add_executable(msdfgen main.cpp)
target_compile_definitions(msdfgen PRIVATE MSDFGEN_STANDALONE)
target_link_libraries(msdfgen lib_msdfgen)
