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
		mRequiredQueueFlags = VK_QUEUE_GRAPHICS_BIT;
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
		vkGetPhysicalDeviceProperties(mPhysicalDeviceHandle, &mPhysicalDeviceProperties);
		vkGetPhysicalDeviceFeatures(mPhysicalDeviceHandle, &mPhysicalDeviceFeatures);
		vkGetPhysicalDeviceMemoryProperties(mPhysicalDeviceHandle, &mPhysicalDeviceMemoryProperties);

		// Store the queue family properties of the chosen physical device.
		uint32_t physicalDeviceQueueFamilyPropertiesCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDeviceHandle, &physicalDeviceQueueFamilyPropertiesCount, nullptr);
		
		mPhysicalDeviceQueueFamilyProperties.resize(physicalDeviceQueueFamilyPropertiesCount);
		vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDeviceHandle, &physicalDeviceQueueFamilyPropertiesCount, mPhysicalDeviceQueueFamilyProperties.data());

		// Store the device extensions of the chosen physical device.
		uint32_t deviceExtensionPropertiesCount = 0;
		vkEnumerateDeviceExtensionProperties(mPhysicalDeviceHandle, nullptr, &deviceExtensionPropertiesCount, nullptr);

		mPhysicalDeviceExtensionProperties.resize(deviceExtensionPropertiesCount);
		vkEnumerateDeviceExtensionProperties(mPhysicalDeviceHandle, nullptr, &deviceExtensionPropertiesCount, mPhysicalDeviceExtensionProperties.data());

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

			// For now, perform presentation with the same queue as graphics operations.
			if (mUseSwapchain)
			{
				mQueueFamilyIndices.mPresentationIndex = mQueueFamilyIndices.mGraphicsIndex;
			}
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

		std::cout << "Queue family - graphics index: " << mQueueFamilyIndices.mGraphicsIndex << std::endl;
		std::cout << "Queue family - compute index: " << mQueueFamilyIndices.mComputeIndex << std::endl;
		std::cout << "Queue family - transfer index: " << mQueueFamilyIndices.mTransferIndex << std::endl;
		std::cout << "Queue family - sparse binding index: " << mQueueFamilyIndices.mSparseBindingIndex << std::endl;
		std::cout << "Number of VkDeviceQueueCreateInfo structures: " << deviceQueueCreateInfos.size() << std::endl;

		// Automatically add the swapchain extension if needed.
		if (mUseSwapchain)
		{
			mRequiredDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		// Create the logical device: note that device layers were deprecated in Vulkan 1.0.13, and device layer 
		// requests should be ignored by the driver.
		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(mRequiredDeviceExtensions.size());
		deviceCreateInfo.pEnabledFeatures = &mPhysicalDeviceFeatures;
		deviceCreateInfo.ppEnabledExtensionNames = mRequiredDeviceExtensions.data();
		deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size());
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		auto result = vkCreateDevice(mPhysicalDeviceHandle, &deviceCreateInfo, nullptr, &mDeviceHandle);
		assert(result == VK_SUCCESS);

		// Store handles to each of the newly created queues.
		vkGetDeviceQueue(mDeviceHandle, mQueueFamilyIndices.mGraphicsIndex, 0, &mQueuesHandles.mGraphicsQueue);
		vkGetDeviceQueue(mDeviceHandle, mQueueFamilyIndices.mComputeIndex, 0, &mQueuesHandles.mComputeQueue);
		vkGetDeviceQueue(mDeviceHandle, mQueueFamilyIndices.mTransferIndex, 0, &mQueuesHandles.mTransferQueue);
		vkGetDeviceQueue(mDeviceHandle, mQueueFamilyIndices.mSparseBindingIndex, 0, &mQueuesHandles.mSparseBindingQueue);

		std::cout << "Sucessfully created logical and physical devices\n";
	}

	Device::~Device()
	{
		// The logical device is likely to be the last object created (aside from objects used at
		// runtime). Before destroying the device, ensure that it is not executing any work.
		vkDeviceWaitIdle(mDeviceHandle);
		
		// Note that queues are created along with the logical device. All queues associated with 
		// this device will automatically be destroyed when vkDestroyDevice is called.

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

	Device::SwapchainSupportDetails Device::getSwapchainSupportDetails(const SurfaceRef &tSurface) const
	{
		SwapchainSupportDetails swapchainSupportDetails;

		// Return the basic surface capabilities, i.e. min/max number of images, min/max width and height, etc.
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysicalDeviceHandle, tSurface->getHandle(), &swapchainSupportDetails.mCapabilities);

		// Retrieve the available surface formats, i.e. pixel formats and color spaces.
		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDeviceHandle, tSurface->getHandle(), &formatCount, nullptr);

		if (formatCount != 0)
		{
			swapchainSupportDetails.mFormats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDeviceHandle, tSurface->getHandle(), &formatCount, swapchainSupportDetails.mFormats.data());
		}

		// Retrieve the surface presentation modes, i.e. VK_PRESENT_MODE_MAILBOX_KHR.
		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDeviceHandle, tSurface->getHandle(), &presentModeCount, nullptr);

		if (presentModeCount != 0) 
		{
			swapchainSupportDetails.mPresentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDeviceHandle, tSurface->getHandle(), &presentModeCount, swapchainSupportDetails.mPresentModes.data());
		}

		return swapchainSupportDetails;
	}

} // namespace vk