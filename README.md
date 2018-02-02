<p align="center">
  <img src="https://github.com/mwalczyk/plume/blob/master/logo.svg" alt="plume logo" width="400" height="auto"/>
</p>

A work-in-progress framework and abstraction layer around the Vulkan graphics API. Currently being developed with version 1.0.65.0.

## Cloning

Plume uses several submodules:
- [shaderc](https://github.com/google/shaderc) for runtime shader compilation (from GLSL to SPIR-V)
- [glm](https://github.com/g-truc/glm) for mathematics
- [spirv-cross](https://github.com/KhronosGroup/SPIRV-Cross) for shader reflection
- [stb](https://github.com/nothings/stb) for image loading
- [glfw](https://github.com/glfw/glfw) for cross-platform windowing

After cloning the main repository, run `checkout_and_build_deps.sh` which will initialize the aforementioned submodules and setup a few other directories that will be used during the build process. Next, run CMake:

```
mkdir build
cd build
cmake ..
make
```

This should create the executable `plume_app` in the `build` directory.

More information on working with submodules can be found [here](https://github.com/blog/2104-working-with-submodules).

## Inspiration

Plume's syntax, structure, and design patterns were greatly influenced by several major
open source projects:

- [Cinder](https://github.com/cinder/Cinder)
- [VPP](https://github.com/nyorain/vpp)
- [Dynamic Static Graphics](https://github.com/DynamicStatic/Dynamic_Static_Graphics)
- [Yave](https://github.com/gan74/Yave)
- [Laugh](https://github.com/jian-ru/laugh_engine)
- [Anvil](https://github.com/GPUOpen-LibrariesAndSDKs/Anvil)
- [Vookoo](https://github.com/andy-thomason/Vookoo)

See the [Plume C++ style guide](https://github.com/mwalczyk/plume_cpp_style) for more information.

## Roadmap

Many parts of Vulkan have not been implemented in Plume. As such, this project should be considered
experimental and breaking changes will happen often.
