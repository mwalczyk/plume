# Project initialization
cmake_minimum_required(VERSION 3.0)
project("plume")

# Compiler settings
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(CMAKE_CONFIGURATION_TYPES Debug Release)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set(warnings "/W4 /EHsc")
  set(CMAKE_CONFIGURATION_TYPES Debug Release)
endif()

# Set C++ language standard
set(CMAKE_CXX_STANDARD 11)

# Set variables
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(DEPS_DIR ${CMAKE_SOURCE_DIR}/deps)

# Add include directories
include_directories($ENV{VULKAN_SDK}/include)
include_directories($ENV{VULKAN_SDK}/include/vulkan)
include_directories(${INCLUDE_DIR}/vk/misc)
include_directories(${INCLUDE_DIR}/vk/wrappers)
include_directories(${DEPS_DIR}/glfw/include/GLFW)
include_directories(${DEPS_DIR}/glm/glm)
include_directories(${DEPS_DIR}/spirv-cross)
include_directories(${DEPS_DIR}/stb)

# Add source files
#add_subdirectory(${SRC_DIR})
#add_subdirectory(${SRC_DIR}/vk/misc)
#add_subdirectory(${SRC_DIR}/vk/wrappers)
#add_subdirectory(${DEPS_DIR}/spirv-cross)

# Add libraries
#find_library(GLFW_LIB libglfw deps/glfw/src)
link_directories(deps/glfw/src)

# Add the executable
file(GLOB_RECURSE sources src/vk/misc/*.cpp src/vk/wrappers/*.cpp deps/spirv-cross/*.cpp)
add_executable(plume_app ${sources})
target_link_libraries(plume_app libglfw)
#"${GLFW_LIB}")


# Build and include GLFW
#set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
#set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
#set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
#add_subdirectory(${INCLUDE_DIR}/glfw)
#include_directories(${INCLUDE_DIR}/glfw/include)
#include_directories(${INCLUDE_DIR}/glfw/deps)

