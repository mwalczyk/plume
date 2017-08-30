# Spectrum

![Logo](https://github.com/mwalczyk/VulkanToolkit/blob/master/logo.png)

A work-in-progress framework and abstraction layer around the Vulkan graphics API. Currently being developed with version 1.0.39.1.

## Cloning

Spectrum uses several submodules:
- [Shaderc](https://github.com/google/shaderc) for runtime shader compilation (from GLSL to SPIR-V)
- [GLM](https://github.com/g-truc/glm) for mathematics
- [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross) for shader reflection
- [stb](https://github.com/nothings/stb) for image loading
After cloning the main repository, make sure to initialize submodules by executing the following command from the top-level
directory:

`git submodule update --init --recursive`

Existing submodules can be updated recursively by executing the following command from the top-level directory:

`git submodule update --recursive --remote`

More information on working with submodules can be found [here](https://github.com/blog/2104-working-with-submodules).

## Inspiration

Spectrum's syntax, structure, and design patterns were greatly influenced by several major
open source projects:

- [Cinder](https://github.com/cinder/Cinder)
- [Nyorain's VPP Toolkit](https://github.com/nyorain/vpp)
- [Dynamic Static's Vulkan Toolkit](https://github.com/DynamicStatic/Dynamic_Static_Graphics)
- [Alexander Overvoorde's Vulkan Tutorial](https://vulkan-tutorial.com/)
- [gan74's Yave Engine](https://github.com/gan74/Yave)
- [Jian Ru's Laugh Engine](https://github.com/jian-ru/laugh_engine)
- [AMD's Anvil](https://github.com/GPUOpen-LibrariesAndSDKs/Anvil)
- [Andy Thomson's Vookoo Toolkit](https://github.com/andy-thomason/Vookoo)

See the [Spectrum C++ style guide](https://github.com/mwalczyk/spectrum_cpp_style) for more information.

## Roadmap:

Many parts of Vulkan have not been implemented in Spectrum. As such, this project should be considered
experimental and breaking changes will happen often.
