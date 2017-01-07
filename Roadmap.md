### To do list:

- [ ] Research `VK_QUEUE_SPARSE_BINDING_BIT` for queue creation in `Device.cpp`
- [ ] Implement physical device selection / candidacy
- [ ] Window resizing / swapchain recreation
- [ ] Handle queue family index selection for surface presentation operations in `Device.cpp`
- [ ] Add options for the `VkSwapchainCreateInfoKHR` structure inside of the swapchain class constructor
- [ ] Remove hardcoded `VkViewport` width and height and `VkRect2D` scissor extent from the pipeline class constructor

### Long term:

- [ ] Create a separate class for resource management (i.e. shader files, etc.)
- [ ] Create an event handling system
- [ ] Add support for headless rendering and multiple windows
- [ ] Add proper exception subclasses
- [ ] Add a "notes" section to each header file, explaining each Vulkan object + usage
- [ ] Implement a standard material model for physically based shading with IBL
- [ ] Implement forward / deferred rendering modes
