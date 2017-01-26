#include "Device.h"

namespace graphics
{

	Device::Options::Options()
	{
		mRequiredQueueFlags = vk::QueueFlagBits::eGraphics;
		mUseSwapchain = true;
	}

	Device::Device(vk::PhysicalDevice tPhysicalDevice, const Options &tOptions) :
		mPhysicalDeviceHandle(tPhysicalDevice),
		mRequiredDeviceExtensions(tOptions.mRequiredDeviceExtensions)
	{		
		// Store the general properties, features, and memory properties of the chosen physical device.
		mPhysicalDeviceProperties = mPhysicalDeviceHandle.getProperties();
		mPhysicalDeviceFeatures = mPhysicalDeviceHandle.getFeatures();
		mPhysicalDeviceMemoryProperties = mPhysicalDeviceHandle.getMemoryProperties();

		// Store the queue family properties of the chosen physical device.
		mPhysicalDeviceQueueFamilyProperties = mPhysicalDeviceHandle.getQueueFamilyProperties();

		// Store the device extensions of the chosen physical device.
		mPhysicalDeviceExtensionProperties = mPhysicalDeviceHandle.enumerateDeviceExtensionProperties();
		
		// Find the indicies of all of the requested queue families (inspired by Sascha Willems' codebase).
		const float defaultQueuePriority = 0.0f;
		const uint32_t defaultQueueCount = 1;
		std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos;
		if (tOptions.mRequiredQueueFlags & vk::QueueFlagBits::eGraphics)
		{
			mQueueFamiliesMapping.mGraphicsQueue.second = findQueueFamilyIndex(vk::QueueFlagBits::eGraphics);

			vk::DeviceQueueCreateInfo deviceQueueCreateInfo = {};
			deviceQueueCreateInfo.pQueuePriorities = &defaultQueuePriority;
			deviceQueueCreateInfo.queueCount = defaultQueueCount;
			deviceQueueCreateInfo.queueFamilyIndex = mQueueFamiliesMapping.mGraphicsQueue.second;

			deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);

			// For now, perform presentation with the same queue as graphics operations.
			if (tOptions.mUseSwapchain)
			{
				mQueueFamiliesMapping.mPresentationQueue.second = mQueueFamiliesMapping.mGraphicsQueue.second;
			}
		}
		if (tOptions.mRequiredQueueFlags & vk::QueueFlagBits::eCompute)
		{
			mQueueFamiliesMapping.mComputeQueue.second = findQueueFamilyIndex(vk::QueueFlagBits::eCompute);

			if (mQueueFamiliesMapping.mComputeQueue.second != mQueueFamiliesMapping.mGraphicsQueue.second)
			{
				// Create a dedicated queue for compute operations.
				vk::DeviceQueueCreateInfo deviceQueueCreateInfo = {};
				deviceQueueCreateInfo.pQueuePriorities = &defaultQueuePriority;
				deviceQueueCreateInfo.queueCount = defaultQueueCount;
				deviceQueueCreateInfo.queueFamilyIndex = mQueueFamiliesMapping.mComputeQueue.second;

				deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
			}
			else
			{
				// Reuse the graphics queue for compute operations.
				mQueueFamiliesMapping.mComputeQueue.second = mQueueFamiliesMapping.mGraphicsQueue.second;
			}
		}
		if (tOptions.mRequiredQueueFlags & vk::QueueFlagBits::eTransfer)
		{
			mQueueFamiliesMapping.mTransferQueue.second = findQueueFamilyIndex(vk::QueueFlagBits::eTransfer);

			if (mQueueFamiliesMapping.mTransferQueue.second != mQueueFamiliesMapping.mGraphicsQueue.second &&
				mQueueFamiliesMapping.mTransferQueue.second != mQueueFamiliesMapping.mComputeQueue.second)
			{
				// Create a dedicated queue for transfer operations.
				vk::DeviceQueueCreateInfo deviceQueueCreateInfo = {};
				deviceQueueCreateInfo.pQueuePriorities = &defaultQueuePriority;
				deviceQueueCreateInfo.queueCount = defaultQueueCount;
				deviceQueueCreateInfo.queueFamilyIndex = mQueueFamiliesMapping.mTransferQueue.second;

				deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
			}
			else
			{
				// Reuse the graphics queue for transfer operations.
				mQueueFamiliesMapping.mTransferQueue.second = mQueueFamiliesMapping.mGraphicsQueue.second;
			}
		}
		if (tOptions.mRequiredQueueFlags & vk::QueueFlagBits::eSparseBinding)
		{
			mQueueFamiliesMapping.mSparseBindingQueue.second = findQueueFamilyIndex(vk::QueueFlagBits::eSparseBinding);

			// TODO
		}

		// Automatically add the swapchain extension if needed.
		if (tOptions.mUseSwapchain)
		{
			mRequiredDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		// Create the logical device: note that device layers were deprecated in Vulkan 1.0.13, and device layer 
		// requests should be ignored by the driver.
		vk::DeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(mRequiredDeviceExtensions.size());
		deviceCreateInfo.pEnabledFeatures = &mPhysicalDeviceFeatures;
		deviceCreateInfo.ppEnabledExtensionNames = mRequiredDeviceExtensions.data();
		deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size());

		mDeviceHandle = mPhysicalDeviceHandle.createDevice(deviceCreateInfo);

		// Store handles to each of the newly created queues.
		mQueueFamiliesMapping.mGraphicsQueue.first = mDeviceHandle.getQueue(mQueueFamiliesMapping.mGraphicsQueue.second, 0);
		mQueueFamiliesMapping.mComputeQueue.first = mDeviceHandle.getQueue(mQueueFamiliesMapping.mComputeQueue.second, 0);
		mQueueFamiliesMapping.mTransferQueue.first = mDeviceHandle.getQueue(mQueueFamiliesMapping.mTransferQueue.second, 0);
		mQueueFamiliesMapping.mSparseBindingQueue.first = mDeviceHandle.getQueue(mQueueFamiliesMapping.mSparseBindingQueue.second, 0);
		mQueueFamiliesMapping.mPresentationQueue.first = mDeviceHandle.getQueue(mQueueFamiliesMapping.mPresentationQueue.second, 0);
	}

	Device::~Device()
	{
		// The logical device is likely to be the last object created (aside from objects used at
		// runtime). Before destroying the device, ensure that it is not executing any work.
		mDeviceHandle.waitIdle();
		
		// Note that queues are created along with the logical device. All queues associated with 
		// this device will automatically be destroyed when vkDestroyDevice is called.
		mDeviceHandle.destroy();
	}

	uint32_t Device::findQueueFamilyIndex(vk::QueueFlagBits tQueueFlagBits) const
	{
		// Try to find a dedicated queue for compute operations (without graphics).
		if (tQueueFlagBits == vk::QueueFlagBits::eCompute)
		{
			for (size_t i = 0; i < mPhysicalDeviceQueueFamilyProperties.size(); ++i)
			{
				if (mPhysicalDeviceQueueFamilyProperties[i].queueCount > 0 && 
					mPhysicalDeviceQueueFamilyProperties[i].queueFlags & tQueueFlagBits &&
					(mPhysicalDeviceQueueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlagBits::eGraphics)
				{
					return static_cast<uint32_t>(i);
				}
			}
		}
		// Try to find a dedicated queue for transfer operations (without compute and graphics).
		else if (tQueueFlagBits == vk::QueueFlagBits::eTransfer)
		{
			for (size_t i = 0; i < mPhysicalDeviceQueueFamilyProperties.size(); ++i)
			{
				if (mPhysicalDeviceQueueFamilyProperties[i].queueCount > 0 &&
					mPhysicalDeviceQueueFamilyProperties[i].queueFlags & tQueueFlagBits &&
					(mPhysicalDeviceQueueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlagBits::eGraphics &&
					(mPhysicalDeviceQueueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute) != vk::QueueFlagBits::eCompute)
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
		swapchainSupportDetails.mCapabilities = mPhysicalDeviceHandle.getSurfaceCapabilitiesKHR(tSurface->getHandle());

		// Retrieve the available surface formats, i.e. pixel formats and color spaces.
		swapchainSupportDetails.mFormats = mPhysicalDeviceHandle.getSurfaceFormatsKHR(tSurface->getHandle());

		// Retrieve the surface presentation modes, i.e. VK_PRESENT_MODE_MAILBOX_KHR.
		swapchainSupportDetails.mPresentModes = mPhysicalDeviceHandle.getSurfacePresentModesKHR(tSurface->getHandle());

		if (swapchainSupportDetails.mFormats.size() == 0 || swapchainSupportDetails.mPresentModes.size() == 0)
		{
			throw std::runtime_error("No available surface formats or present modes found");
		}

		return swapchainSupportDetails;
	}

	std::ostream& operator<<(std::ostream &tStream, const DeviceRef &tDevice)
	{
		tStream << "Device object: " << tDevice->mDeviceHandle << std::endl;
		
		tStream << "Chosen physical device object: " << tDevice->mPhysicalDeviceHandle << std::endl;
		std::cout << "\tDevice ID: " << tDevice->mPhysicalDeviceProperties.deviceID << std::endl;
		std::cout << "\tDevice name: " << tDevice->mPhysicalDeviceProperties.deviceName << std::endl;
		std::cout << "\tVendor ID: " << tDevice->mPhysicalDeviceProperties.vendorID << std::endl;
		
		tStream << "Queue family details:" << std::endl;
		tStream << "\tQueue family - graphics index: " << tDevice->mQueueFamiliesMapping.graphics().second << std::endl;
		tStream << "\tQueue family - compute index: " << tDevice->mQueueFamiliesMapping.compute().second << std::endl;
		tStream << "\tQueue family - transfer index: " << tDevice->mQueueFamiliesMapping.transfer().second << std::endl;
		tStream << "\tQueue family - sparse binding index: " << tDevice->mQueueFamiliesMapping.sparseBinding().second << std::endl;
		tStream << "\tQueue family - present index: " << tDevice->mQueueFamiliesMapping.presentation().second << std::endl;

		return tStream;
	}

} // namespace graphics