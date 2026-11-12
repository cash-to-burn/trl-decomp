cmake_minimum_required(VERSION 3.20)
project(trl_decomp C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    src
)

if (WIN32 OR MINGW)
    add_definitions(-D_WIN32 -DWIN32_LEAN_AND_MEAN -DNOMINMAX)
endif()

file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*.c"
)

add_library(trl_decomp_core STATIC ${SOURCES})
