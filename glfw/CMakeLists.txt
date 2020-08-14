cmake_policy(SET CMP0048 NEW)
cmake_minimum_required(VERSION 3.0)

project(FunWithGlfw VERSION 0.1.0)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

configure_file(config.h.in config.h)

# Glfw 
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(include/GLFW)
add_subdirectory(include/glad)
add_subdirectory(include/linmath.h)

add_executable(FunWithGlfw main.c)

target_link_libraries(FunWithGlfw
  PRIVATE
  glfw
  glad
  linmath.h
)

target_include_directories(FunWithGlfw 
  PUBLIC
  "${PROJECT_BINARY_DIR}"
)

