cmake_policy(SET CMP0048 NEW)
cmake_minimum_required(VERSION 3.0)

project(OpenGLFun VERSION 0.1.0)

set(EXECUTABLE_NAME openglfun)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_C_STANDARD 11)
# set(CMAKE_C_STANDARD_REQUIRED ON)

# configure_file(config.h.in config.h)

# Glfw 
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(include/GLFW)
add_subdirectory(include/glad)

file(GLOB SOURCES *.cpp)
add_executable(${EXECUTABLE_NAME} ${SOURCES})

target_link_libraries(${EXECUTABLE_NAME}
  PRIVATE
  glfw
  glad
)

target_include_directories(${EXECUTABLE_NAME} 
  PUBLIC
  "${PROJECT_BINARY_DIR}"
)

