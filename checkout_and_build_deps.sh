# initialize and update all submodules
git submodule update --init --recursive
git submodule update --recursive --remote

# clone necessary sub-repos for shaderc
cd deps/shaderc/third_party
git clone https://github.com/google/googletest.git
git clone https://github.com/google/glslang.git
git clone https://github.com/KhronosGroup/SPIRV-Tools.git spirv-tools
git clone https://github.com/KhronosGroup/SPIRV-Headers.git spirv-tools/external/spirv-headers

# copy header/source files from spirv-cross into new folders that
# will be included in the build
cd ../../..
mkdir include/vk/spirv-cross
mkdir src/vk/spirv-cross
mv deps/spirv-cross/*.hpp include/vk/spirv-cross
mv deps/spirv-cross/*.cpp src/vk/spirv-cross
rm src/vk/spirv-cross/main.cpp

