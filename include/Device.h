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

	struct QueueFamilyIndices
	{
		uint32_t mGraphicsIndex;
		uint32_t mComputeIndex;
		uint32_t mTransferIndex;
		uint32_t mSparseBindingIndex;
	};

	class Device;
	using DeviceRef = std::shared_ptr<Device>;

	class Device
	{

	public:

		struct Options
		{
			Options();

			Options& requiredQueueFlags(VkQueueFlags tRequiredQueueFlags) { mRequiredQueueFlags = tRequiredQueueFlags; return *this; }
			Options& requiredDeviceExtensions(const std::vector<const char*> &tRequiredDeviceExtensions) { mRequiredDeviceExtensions = tRequiredDeviceExtensions; return *this; }
			Options& useSwapchain(bool tUseSwapchain) { mUseSwapchain = tUseSwapchain; return *this; }

			VkQueueFlags mRequiredQueueFlags;
			std::vector<const char*> mRequiredDeviceExtensions;
			bool mUseSwapchain;
		};

		static DeviceRef create(VkPhysicalDevice tPhysicalDevice, const Options &tOptions = Options())
		{
			return std::make_shared<Device>(tPhysicalDevice, tOptions);
		}

		Device(VkPhysicalDevice tPhysicalDevice, const Options &tOptions = Options());
		~Device();

		inline VkDevice getHandle() const { return mDeviceHandle; };
		inline VkPhysicalDevice getPhysicalDeviceHandle() const { return mPhysicalDeviceHandle; }
		inline VkPhysicalDeviceProperties getPhysicalDeviceProperties() const { return mPhysicalDeviceProperties; }
		inline VkPhysicalDeviceFeatures getPhysicalDeviceFeatures() const { return mPhysicalDeviceFeatures;  }
		inline VkPhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties() const { return mPhysicalDeviceMemoryProperties; }
		inline const std::vector<VkQueueFamilyProperties>& getPhysicalDeviceQueueFamilyProperties() const { return mPhysicalDeviceQueueFamilyProperties; }
		inline const std::vector<VkExtensionProperties>& getPhysicalDeviceExtensionProperties() const { return mPhysicalDeviceExtensionProperties; }

	private:

		uint32_t findQueueFamilyIndex(VkQueueFlagBits tQueueFlagBits) const;

		VkDevice mDeviceHandle;
		VkPhysicalDevice mPhysicalDeviceHandle;
		VkQueue mQueueHandle;
		VkPhysicalDeviceProperties mPhysicalDeviceProperties;
		VkPhysicalDeviceFeatures mPhysicalDeviceFeatures;
		VkPhysicalDeviceMemoryProperties mPhysicalDeviceMemoryProperties;
		std::vector<VkQueueFamilyProperties> mPhysicalDeviceQueueFamilyProperties;
		std::vector<VkExtensionProperties> mPhysicalDeviceExtensionProperties;

		VkQueueFlags mRequiredQueueFlags;
		std::vector<const char*> mRequiredDeviceExtensions;
		bool mUseSwapchain;
		QueueFamilyIndices mQueueFamilyIndices;

	};

} // namespace vk