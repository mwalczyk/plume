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
set(CMAKE_CXX_STANDARD 14)
#add_definitions(-D_CRT_SECURE_NO_WARNINGS)
#add_definitions(-std=c++11)

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

# Add libraries
#link_directories(${CMAKE_SOURCE_DIR}/deps/glfw/src)
#find_library(GLFW_LIBRARY libglfw /home/mike/Documents/github-repositories/plume/deps/glfw/src)
#add_subdirectory(${DEPS_DIR}/glfw)
find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/libs/vulkan" REQUIRED)
find_library(Glfw_LIBRARY NAMES libglfw.so PATHS ${CMAKE_SOURCE_DIR}/deps/glfw/src)

# Add the executable
file(GLOB sources src/*.cpp src/vk/misc/*.cpp src/vk/wrappers/*.cpp deps/spirv-cross/*.cpp)
add_executable(plume_app ${sources})

target_link_libraries(plume_app ${Vulkan_LIBRARY} ${Glfw_LIBRARY})

