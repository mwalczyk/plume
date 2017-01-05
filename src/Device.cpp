#include "Device.h"

namespace vk
{
	/// Notes:
	///
	/// The Vulkan implementation will generally group all of the queues with the same capabilities into a single queue
	/// family. However, this is not a strict requirement, and there may be multiple queue families with the same 
	/// capabilities.

	Device::Options::Options()
	{
		mRequiredQueueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
		mUseSwapchain = true;
	}

	Device::Device(VkPhysicalDevice tPhysicalDevice, const Options &tOptions) :
		mPhysicalDeviceHandle(tPhysicalDevice),
		mRequiredQueueFlags(tOptions.mRequiredQueueFlags),
		mRequiredDeviceExtensions(tOptions.mRequiredDeviceExtensions),
		mUseSwapchain(tOptions.mUseSwapchain)
	{		
		// Ensure that at least one suitable GPU was found.
		assert(mPhysicalDeviceHandle != VK_NULL_HANDLE);

		// Store the general properties, features, and memory properties of the chosen physical device.
		vkGetPhysicalDeviceProperties(tPhysicalDevice, &mPhysicalDeviceProperties);
		vkGetPhysicalDeviceFeatures(tPhysicalDevice, &mPhysicalDeviceFeatures);
		vkGetPhysicalDeviceMemoryProperties(tPhysicalDevice, &mPhysicalDeviceMemoryProperties);

		// Store the queue family properties of the chosen physical device.
		uint32_t physicalDeviceQueueFamilyPropertiesCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(tPhysicalDevice, &physicalDeviceQueueFamilyPropertiesCount, nullptr);
		
		mPhysicalDeviceQueueFamilyProperties.resize(physicalDeviceQueueFamilyPropertiesCount);
		vkGetPhysicalDeviceQueueFamilyProperties(tPhysicalDevice, &physicalDeviceQueueFamilyPropertiesCount, mPhysicalDeviceQueueFamilyProperties.data());

		// Store the device extensions of the chosen physical device.
		uint32_t deviceExtensionPropertiesCount = 0;
		vkEnumerateDeviceExtensionProperties(tPhysicalDevice, nullptr, &deviceExtensionPropertiesCount, nullptr);

		mPhysicalDeviceExtensionProperties.resize(deviceExtensionPropertiesCount);
		vkEnumerateDeviceExtensionProperties(tPhysicalDevice, nullptr, &deviceExtensionPropertiesCount, mPhysicalDeviceExtensionProperties.data());

		// Print some useful information about the chosen physical device.
		std::cout << "Found suitable physical device:\n";
		std::cout << "\tdevice ID: " << mPhysicalDeviceProperties.deviceID << "\n";
		std::cout << "\tdevice name: " << mPhysicalDeviceProperties.deviceName << "\n";
		std::cout << "\tvendor ID: " << mPhysicalDeviceProperties.vendorID << "\n";
		
		// Find the indicies of all of the requested queue families (inspired by Sascha Willems' codebase).
		const float defaultQueuePriority = 0.0f;
		const uint32_t defaultQueueCount = 1;
		std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
		if (mRequiredQueueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			mQueueFamilyIndices.mGraphicsIndex = findQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);

			VkDeviceQueueCreateInfo deviceQueueCreateInfo = {};
			deviceQueueCreateInfo.pQueuePriorities = &defaultQueuePriority;
			deviceQueueCreateInfo.queueCount = defaultQueueCount;
			deviceQueueCreateInfo.queueFamilyIndex = mQueueFamilyIndices.mGraphicsIndex;
			deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

			deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
		}
		if (mRequiredQueueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			mQueueFamilyIndices.mComputeIndex = findQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);

			if (mQueueFamilyIndices.mComputeIndex != mQueueFamilyIndices.mGraphicsIndex)
			{
				// Create a dedicated queue for compute operations.
				VkDeviceQueueCreateInfo deviceQueueCreateInfo = {};
				deviceQueueCreateInfo.pQueuePriorities = &defaultQueuePriority;
				deviceQueueCreateInfo.queueCount = defaultQueueCount;
				deviceQueueCreateInfo.queueFamilyIndex = mQueueFamilyIndices.mComputeIndex;
				deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

				deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
			}
			else
			{
				// Reuse the graphics queue for compute operations.
				mQueueFamilyIndices.mComputeIndex = mQueueFamilyIndices.mGraphicsIndex;
			}
		}
		if (mRequiredQueueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			mQueueFamilyIndices.mTransferIndex = findQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);

			if (mQueueFamilyIndices.mTransferIndex != mQueueFamilyIndices.mGraphicsIndex &&
				mQueueFamilyIndices.mTransferIndex != mQueueFamilyIndices.mComputeIndex)
			{
				// Create a dedicated queue for transfer operations.
				VkDeviceQueueCreateInfo deviceQueueCreateInfo = {};
				deviceQueueCreateInfo.pQueuePriorities = &defaultQueuePriority;
				deviceQueueCreateInfo.queueCount = defaultQueueCount;
				deviceQueueCreateInfo.queueFamilyIndex = mQueueFamilyIndices.mTransferIndex;
				deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

				deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
			}
			else
			{
				// Reuse the graphics queue for transfer operations.
				mQueueFamilyIndices.mTransferIndex = mQueueFamilyIndices.mGraphicsIndex;
			}
		}
		if (mRequiredQueueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
		{
			mQueueFamilyIndices.mSparseBindingIndex = findQueueFamilyIndex(VK_QUEUE_SPARSE_BINDING_BIT);

			// TODO
		}

		std::cout << "Queue family - graphcis index: " << mQueueFamilyIndices.mGraphicsIndex << std::endl;
		std::cout << "Queue family - compute index: " << mQueueFamilyIndices.mComputeIndex << std::endl;
		std::cout << "Queue family - transfer index: " << mQueueFamilyIndices.mTransferIndex << std::endl;
		std::cout << "Queue family - sparse binding index: " << mQueueFamilyIndices.mSparseBindingIndex << std::endl;
		std::cout << "Number of VkDeviceQueueCreateInfo structures: " << deviceQueueCreateInfos.size() << std::endl;

		// Automatically add the swapchain extension if needed.
		if (mUseSwapchain)
		{
			mRequiredDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		/*
		// For now, we simply create a single queue from the first queue family
		VkDeviceQueueCreateInfo deviceQueueCreateInfo;
		deviceQueueCreateInfo.flags = 0;
		deviceQueueCreateInfo.pNext = nullptr;
		deviceQueueCreateInfo.pQueuePriorities = &defaultQueuePriority;
		deviceQueueCreateInfo.queueCount = 1;
		deviceQueueCreateInfo.queueFamilyIndex = 0;
		deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

		VkDeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.enabledExtensionCount = mRequiredDeviceExtensions.size();
		deviceCreateInfo.enabledLayerCount = mInstance->mRequiredLayers.size();
		deviceCreateInfo.flags = 0;
		deviceCreateInfo.pEnabledFeatures = &mPhysicalDeviceFeatures;
		deviceCreateInfo.pNext = nullptr;
		deviceCreateInfo.ppEnabledExtensionNames = mRequiredDeviceExtensions.data();
		deviceCreateInfo.ppEnabledLayerNames = mInstance->mRequiredLayers.data();
		deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		auto result = vkCreateDevice(mPhysicalDeviceHandle, &deviceCreateInfo, nullptr, &mDeviceHandle);
		assert(result == VK_SUCCESS);

		// Store a handle to the queue
		vkGetDeviceQueue(mDeviceHandle, mQueueFamilyIndex, 0, &mQueueHandle);
		*/

		// HERE we should try to find a queue for each of the requested queue families - individual queues are better... but 
		// if a unique queue is not found, find a queue family that supports multiple operations (see Sascha Willems' example)

		std::cout << "Sucessfully created logical and physical devices\n";
	}

	Device::~Device()
	{
		// The logical device is likely to be the last object created (aside from objects used at
		// runtime). Before destroying the device, ensure that it is not executing any work.
		vkDeviceWaitIdle(mDeviceHandle);

		vkDestroyDevice(mDeviceHandle, nullptr);
	}

	uint32_t Device::findQueueFamilyIndex(VkQueueFlagBits tQueueFlagBits) const
	{
		// Try to find a dedicated queue for compute operations (without graphics).
		if (tQueueFlagBits & VK_QUEUE_COMPUTE_BIT)
		{
			for (size_t i = 0; i < mPhysicalDeviceQueueFamilyProperties.size(); ++i)
			{
				if (mPhysicalDeviceQueueFamilyProperties[i].queueCount > 0 && 
					mPhysicalDeviceQueueFamilyProperties[i].queueFlags & tQueueFlagBits &&
					(mPhysicalDeviceQueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
				{
					return static_cast<uint32_t>(i);
				}
			}
		}
		// Try to find a dedicated queue for transfer operations (without compute and graphics).
		else if (tQueueFlagBits & VK_QUEUE_TRANSFER_BIT)
		{
			for (size_t i = 0; i < mPhysicalDeviceQueueFamilyProperties.size(); ++i)
			{
				if (mPhysicalDeviceQueueFamilyProperties[i].queueCount > 0 &&
					mPhysicalDeviceQueueFamilyProperties[i].queueFlags & tQueueFlagBits &&
					(mPhysicalDeviceQueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 &&
					(mPhysicalDeviceQueueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)
				{
					return static_cast<uint32_t>(i);
				}
			}
		}
		
		// For all other queue families (or if a dedicated queue was not found above), simply return the 
		// index of the first queue family that supports the requested operations.
		for (size_t i = 0; i < mPhysicalDeviceQueueFamilyProperties.size(); ++i)
		{
			if (mPhysicalDeviceQueueFamilyProperties[i].queueCount > 0 && mPhysicalDeviceQueueFamilyProperties[i].queueFlags & tQueueFlagBits)
			{
				return static_cast<uint32_t>(i);
			}
		}
		
		throw std::runtime_error("Could not find a matching queue family");
	}

	/*
	SwapchainSupportDetails Device::getSwapchainSupportDetails(VkPhysicalDevice tPhysicalDevice) const
	{
		SwapchainSupportDetails details;

		// General surface capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(tPhysicalDevice, mSurface->getHandle(), &details.mCapabilities);

		// Surface formats
		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(tPhysicalDevice, mSurface->getHandle(), &formatCount, nullptr);
		if (formatCount != 0)
		{
			details.mFormats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(tPhysicalDevice, mSurface->getHandle(), &formatCount, details.mFormats.data());
		}

		// Surface presentation modes
		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(tPhysicalDevice, mSurface->getHandle(), &presentModeCount, nullptr);

		if (presentModeCount != 0) 
		{
			details.mPresentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(tPhysicalDevice, mSurface->getHandle(), &presentModeCount, details.mPresentModes.data());
		}

		return details;
	}*/

} // namespace vk