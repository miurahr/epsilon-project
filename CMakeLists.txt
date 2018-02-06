cmake_minimum_required(VERSION 3.5)
project(epsilon C)

set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_MACOSX_RPATH 1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

set(EPSILON_VERSION 0.9.2)
set(EPSILON_VERSION_MAJOR 0)
set(EPSILON_VERSION_MINOR 9)
set(EPSILON_VERSION_PATCH 2)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configs" FORCE)
file(WRITE ${CMAKE_BINARY_DIR}/epsilon_version.h "#define VERSION \"${EPSILON_VERSION}\"")

set(TARGET_LIB_NAME epsilon)
set(TARGET_CMD_NAME epsilon)

include(CMakeDependentOption)
find_package(MPI)
cmake_dependent_option(ENABLE_MPI "Enable MPI support" OFF "MPI_C_FOUND" OFF)
option(ENABLE_CLUSTER "Support Cluster" OFF)
find_package(Threads)
cmake_dependent_option(ENABLE_PTHREADS "Support pthreads" ON "CMAKE_USE_PTHRADS_INIT" OFF)
include(StringOption)
string_option(MAX_N_THREADS "maximum allowed threads(number)" 512)
string_option(DEF_N_THREADS "default threads(number)" 2)

add_subdirectory(lib)
add_subdirectory(src)
