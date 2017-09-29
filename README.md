<p align="center">
  <img src="https://github.com/mwalczyk/VulkanToolkit/blob/master/logo.svg" alt="plume logo" width="400" height="auto"/>
</p>

A work-in-progress framework and abstraction layer around the Vulkan graphics API. Currently being developed with version 1.0.54.0.

## Cloning

Plume uses several submodules:
- [shaderc](https://github.com/google/shaderc) for runtime shader compilation (from GLSL to SPIR-V)
- [glm](https://github.com/g-truc/glm) for mathematics
- [spirv-cross](https://github.com/KhronosGroup/SPIRV-Cross) for shader reflection
- [stb](https://github.com/nothings/stb) for image loading

After cloning the main repository, make sure to initialize submodules by executing the following command from the top-level directory:

`git submodule update --init --recursive`

Existing submodules can be updated recursively by executing the following command from the top-level directory:

`git submodule update --recursive --remote`

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
