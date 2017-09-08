# initialize and update all submodules
git submodule update --init --recursive
git submodule update --recursive --remote

# clone necessary sub-repos for shaderc
cd deps/shaderc/third_party
git clone https://github.com/google/googletest.git
git clone https://github.com/google/glslang.git
git clone https://github.com/KhronosGroup/SPIRV-Tools.git spirv-tools
git clone https://github.com/KhronosGroup/SPIRV-Headers.git spirv-tools/external/spirv-headers

# build shaderc
cd ..
mkdir build
cd build
cmake ..
cmake --build . --config Debug
cmake --build . --config Release
