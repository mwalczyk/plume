# Spectrum

![IcoSphere](https://github.com/mwalczyk/VulkanToolkit/blob/master/icosphere.PNG)

A WIP framework and abstraction layer around the Vulkan graphics API. Currently being developed with
version 1.0.39.1.

## Cloning

VulkanToolkit uses several submodules (GLM, SPIRV-Cross, STB, etc.). After cloning the main
repository, make sure to initialize submodules by executing the following command from the top-level
directory:

`git submodule update --init --recursive`

More information on working with submodules can be found [here](https://github.com/blog/2104-working-with-submodules).

## Inspiration

VulkanToolkit's syntax, structure, and design patterns were greatly influenced by several major
open source projects:

- [Cinder](https://github.com/cinder/Cinder)
- [Nyorain's VPP Toolkit](https://github.com/nyorain/vpp)
- [Dynamic Static's Vulkan Toolkit](https://github.com/DynamicStatic/Dynamic_Static_Graphics)
- [Alexander Overvoorde's Vulkan Tutorial](https://vulkan-tutorial.com/)

## Roadmap:

- [ ] Add support for headless rendering and multiple windows
- [ ] Add a "notes" section to each header file, explaining each Vulkan object and its usage
- [ ] Implement a standard material model for physically based shading with IBL
- [ ] Implement forward / deferred rendering modes
