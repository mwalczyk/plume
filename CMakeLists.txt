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

# Set variables
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(DEPS_DIR ${CMAKE_SOURCE_DIR}/deps)

# Add include directories
include_directories($ENV{VULKAN_SDK}/include)
include_directories($ENV{VULKAN_SDK}/include/vulkan)
include_directories(${INCLUDE_DIR}/vk/misc)
include_directories(${INCLUDE_DIR}/vk/wrappers)
include_directories(${INCLUDE_DIR}/vk/spirv-cross)
include_directories(${DEPS_DIR}/shaderc/libshaderc/include)
include_directories(${DEPS_DIR}/shaderc/libshaderc_util/include)
include_directories(${DEPS_DIR}/glfw/include/GLFW)
include_directories(${DEPS_DIR}/glm/glm)
include_directories(${DEPS_DIR}/stb)

# Compile glfw, as per: http://www.glfw.org/docs/latest/build_guide.html
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/glfw)

# Compile shaderc, as per: https://github.com/google/shaderc/blob/master/libshaderc/README.md
#add_subdirectory(deps/shaderc)

# Add Vulkan library
find_library(VULKAN_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/libs/vulkan" REQUIRED)
find_library(SHADERC_LIBRARY NAMES shaderc_combined HINTS "${CMAKE_SOURCE_DIR}/deps/shaderc/build/libshaderc" REQUIRED)

# Grab all of the source files
file(GLOB SOURCES src/*.cpp src/vk/misc/*.cpp src/vk/wrappers/*.cpp src/vk/spirv-cross/*.cpp)

# Add the executable and link libraries
add_executable(plume_app ${SOURCES})
target_link_libraries(plume_app ${VULKAN_LIBRARY} ${SHADERC_LIBRARY})
target_link_libraries(plume_app glfw)
