### To do list:

- [ ] Research `VK_QUEUE_SPARSE_BINDING_BIT` for queue creation in `Device.cpp`
- [ ] Implement physical device selection / candidacy
- [ ] Window resizing / swapchain recreation
- [ ] Handle queue family index selection for surface presentation operations in `Device.cpp`
- [ ] Add options for the `VkSwapchainCreateInfoKHR` structure inside of the swapchain class constructor
- [ ] Remove hardcoded `VkViewport` width and height and `VkRect2D` scissor extent from the pipeline class constructor
- [ ] Create a pipeline base class and two derived classes: graphics pipeline and compute pipeline
- [ ] Create a simple structure / method for changing the blend mode of a graphics pipeline (i.e. additive blending)
- [ ] Add checks for framebuffer / render pass compatibility (same number / type of attachments)
- [ ] Remove the hardcoded clear color and image extent in the `beginRenderPass` method of the command buffer class
- [ ] Remove the hardcoded pipeline bind point (graphics / compute) in the `bindPipeline` method of the command buffer class
- [ ] Create a wrapper around VkFence objects
- [ ] Add a fence parameter to the `acquireNextSwapchainImage` method of the swapchain class
- [ ] Move the shader module class to a separate file
- [ ] Add reflection support for ubos, textures, etc.
- [ ] Remove the hardcoded entry point in the `VkPipelineShaderStageCreateInfo` structure
- [ ] Add error handling to `updatePushConstantRanges` method of the command buffer class to prevent the user from passing data that is larger than the push constants member

### Long term:

- [ ] Change structure member variable naming semantics (remove the "m" prefix)
- [ ] Create a separate class for resource management (i.e. shader files, etc.)
- [ ] Create an event handling system
- [ ] Add support for headless rendering and multiple windows
- [ ] Add proper exception subclasses
- [ ] Add a "notes" section to each header file, explaining each Vulkan object + usage
- [ ] Implement a standard material model for physically based shading with IBL
- [ ] Implement forward / deferred rendering modes
