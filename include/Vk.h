#pragma once

/// Naming conventions used throughout this toolkit:
///
/// Class names are the same as they appear in the Vulkan API, minus the "Vk" prefix:
///		e.g. VkDevice -> Device
/// Shared pointers to classes are the same as the class name, plus the "Ref" suffix:
///		e.g. Device -> DeviceRef
/// Vulkan related variables should always use the full name, minus the "Vk" prefix and
///		without extension specific suffixes like "EXT" and "KHR":
///		e.g. VkExtensionProperties -> extensionProperties
///		e.g. VkSwapchainKHR -> swapchain;
///	All member variables should be prefixed with an "m":
///		e.g. uint32_t mNumberOfObjects;
/// All method parameters should be prefixed with a "t":
///		e.g. void multiply(uint32_t tValue);
///	Handles to Vulkan objects should be declared with the "Handle" suffix:
///		e.g. VkInstance -> mInstanceHandle
///	Furthermore, each Vulkan wrapper class should have an inline member function called
///		getHandle() that returns this value
/// "Getters" and "setters" should be inline in the header file
///
///	Note that all classes and structs exist in the "vk" namespace
/// 
/// In general, class items should appear in the following order:
///		Forward declarations and typedefs
///		Internal structs
///		Static variables and functions (e.g. factory "create" methods)
///		Constructors
///		Deconstructor
///		Public methods
///		Public member variables
///		Private methods
///		Private member variables

#include "Buffer.h"
#include "CommandBuffer.h"
#include "CommandPool.h"
#include "DescriptorPool.h"
#include "Device.h"
#include "Framebuffer.h"
#include "Image.h"
#include "Instance.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "Semaphore.h"
#include "ShaderModule.h"
#include "Surface.h"
#include "Swapchain.h"
#include "Window.h"