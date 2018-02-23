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
set(SHADERC_SKIP_TESTS ON)

# Compile shaderc, as per: https://github.com/google/shaderc/blob/master/libshaderc/README.md
#set(CMAKE_GENERATOR "Ninja")
#message("generator is set to ${CMAKE_GENERATOR}")
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/shaderc)

