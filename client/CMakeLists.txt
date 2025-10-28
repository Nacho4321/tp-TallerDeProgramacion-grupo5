cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(SDL2pp-tutorial LANGUAGES CXX)

# set up SDL2pp library
set(SDL2PP_WITH_IMAGE YES)
set(SDL2PP_WITH_TTF YES)
add_subdirectory(SDL2pp)

# add compilation flags
set(CMAKE_CXX_STANDARD 17)
add_definitions(-DDATA_PATH="${PROJECT_SOURCE_DIR}/data")
include_directories(${SDL2PP_INCLUDE_DIRS})

# define targetss
add_executable(render test_sdl.cpp)
target_link_libraries(render PUBLIC SDL2pp)

