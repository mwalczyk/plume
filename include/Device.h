#pragma once

#include <memory>
#include <vector>
#include <cassert>

#include "Platform.h"
#include "Instance.h"
#include "Surface.h"

namespace vk
{

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR mCapabilities;		
		std::vector<VkSurfaceFormatKHR> mFormats;		
		std::vector<VkPresentModeKHR> mPresentModes;	
	};

	class Device;
	using DeviceRef = std::shared_ptr<Device>;

	class Device
	{

	public:

		struct Options
		{
			Options();

			Options& requiredQueueFlagBits(const std::vector<VkQueueFlagBits> &tRequiredQueueFlagBits) { mRequiredQueueFlagBits = tRequiredQueueFlagBits; return *this; }
			Options& requiredDeviceExtensions(const std::vector<const char*> &tRequiredDeviceExtensions) { mRequiredDeviceExtensions = tRequiredDeviceExtensions; return *this; }
			Options& requiredPhysicalDeviceType(VkPhysicalDeviceType tRequiredPhysicalDeviceType) { mRequiredPhysicalDeviceType = tRequiredPhysicalDeviceType; return *this; }
			Options& requiredPhysicalDeviceFeatures(VkPhysicalDeviceFeatures tRequiredPhysicalDeviceFeatures) { mRequiredPhysicalDeviceFeatures = tRequiredPhysicalDeviceFeatures; return *this; }

			std::vector<VkQueueFlagBits> mRequiredQueueFlagBits;
			std::vector<const char*> mRequiredDeviceExtensions;
			VkPhysicalDeviceType mRequiredPhysicalDeviceType;
			VkPhysicalDeviceFeatures mRequiredPhysicalDeviceFeatures;
		};

		static DeviceRef create(const InstanceRef &tInstance, const SurfaceRef &tSurface, const Options &tOptions = Options())
		{
			return std::make_shared<Device>(tInstance, tSurface, tOptions);
		}

		Device(const InstanceRef &tInstance, const SurfaceRef &tSurface, const Options &tOptions = Options());
		~Device();

		inline VkDevice getHandle() const { return mDeviceHandle; };
		VkPhysicalDeviceProperties getPhysicalDeviceProperties(VkPhysicalDevice tPhysicalDevice) const;
		VkPhysicalDeviceFeatures getPhysicalDeviceFeatures(VkPhysicalDevice tPhysicalDevice) const;
		std::vector<VkQueueFamilyProperties> getPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice tPhysicalDevice) const;
		std::vector<VkExtensionProperties> getDeviceExtensionProperties(VkPhysicalDevice tPhysicalDevice) const;
		VkPhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties(VkPhysicalDevice tPhysicalDevice) const;
		SwapchainSupportDetails Device::getSwapchainSupportDetails(VkPhysicalDevice tPhysicalDevice) const;

	private:

		bool isPhysicalDeviceSuitable(VkPhysicalDevice tPhysicalDevice);
		bool checkDeviceExtensionSupport(VkPhysicalDevice tPhysicalDevice) const;
		void initLogicalDevice();

		VkDevice mDeviceHandle;
		VkPhysicalDevice mPhysicalDeviceHandle;
		VkQueue mQueueHandle;

		InstanceRef mInstance;
		SurfaceRef mSurface;

		std::vector<VkQueueFlagBits> mRequiredQueueFlagBits;
		std::vector<const char*> mRequiredDeviceExtensions;
		VkPhysicalDeviceType mRequiredPhysicalDeviceType;

		size_t mQueueFamilyIndex;
	};

} // namespace vk