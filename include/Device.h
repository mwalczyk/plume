#pragma once

#include <memory>
#include <vector>
#include <cassert>

#include "Platform.h"
#include "Instance.h"
#include "Surface.h"

namespace vk
{

	class Device;
	using DeviceRef = std::shared_ptr<Device>;

	class Device
	{
	public:

		struct SwapchainSupportDetails
		{
			VkSurfaceCapabilitiesKHR mCapabilities;
			std::vector<VkSurfaceFormatKHR> mFormats;
			std::vector<VkPresentModeKHR> mPresentModes;
		};

		struct QueueFamilyIndices
		{
			uint32_t mGraphicsIndex = 0;
			uint32_t mComputeIndex = 0;
			uint32_t mTransferIndex = 0;
			uint32_t mSparseBindingIndex = 0;
			uint32_t mPresentationIndex = 0;
		};

		struct QueueFamilyHandles
		{
			VkQueue mGraphicsQueue = VK_NULL_HANDLE;
			VkQueue mComputeQueue = VK_NULL_HANDLE;
			VkQueue mTransferQueue = VK_NULL_HANDLE;
			VkQueue mSparseBindingQueue = VK_NULL_HANDLE;
			VkQueue mPresentationQueue = VK_NULL_HANDLE;
		};

		class Options
		{
		public:

			Options();

			Options& requiredQueueFlags(VkQueueFlags tRequiredQueueFlags) { mRequiredQueueFlags = tRequiredQueueFlags; return *this; }
			Options& requiredDeviceExtensions(const std::vector<const char*> &tRequiredDeviceExtensions) { mRequiredDeviceExtensions = tRequiredDeviceExtensions; return *this; }
			Options& useSwapchain(bool tUseSwapchain) { mUseSwapchain = tUseSwapchain; return *this; }

		private:

			VkQueueFlags mRequiredQueueFlags;
			std::vector<const char*> mRequiredDeviceExtensions;
			bool mUseSwapchain;

			friend class Device;
		};

		//! Factory method for returning a new DeviceRef.
		static DeviceRef create(VkPhysicalDevice tPhysicalDevice, const Options &tOptions = Options())
		{
			return std::make_shared<Device>(tPhysicalDevice, tOptions);
		}

		//! Construct a logical device around a physical device (GPU).
		Device(VkPhysicalDevice tPhysicalDevice, const Options &tOptions = Options());
		~Device();

		inline VkDevice getHandle() const { return mDeviceHandle; };
		inline VkPhysicalDevice getPhysicalDeviceHandle() const { return mPhysicalDeviceHandle; }
		inline QueueFamilyIndices getQueueFamilyIndices() const { return mQueueFamilyIndices; }
		inline QueueFamilyHandles getQueueHandles() const { return mQueuesHandles; }
		inline VkPhysicalDeviceProperties getPhysicalDeviceProperties() const { return mPhysicalDeviceProperties; }
		inline VkPhysicalDeviceFeatures getPhysicalDeviceFeatures() const { return mPhysicalDeviceFeatures;  }
		inline VkPhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties() const { return mPhysicalDeviceMemoryProperties; }
		inline const std::vector<VkQueueFamilyProperties>& getPhysicalDeviceQueueFamilyProperties() const { return mPhysicalDeviceQueueFamilyProperties; }
		inline const std::vector<VkExtensionProperties>& getPhysicalDeviceExtensionProperties() const { return mPhysicalDeviceExtensionProperties; }
		
		//! Returns a structure that contains information related to the chosen physical device's swapchain support.
		SwapchainSupportDetails getSwapchainSupportDetails(const SurfaceRef &tSurface) const;

		friend std::ostream& operator<<(std::ostream &tStream, const DeviceRef &tDevice);

	private:

		uint32_t findQueueFamilyIndex(VkQueueFlagBits tQueueFlagBits) const;

		VkDevice mDeviceHandle;
		VkPhysicalDevice mPhysicalDeviceHandle;
		QueueFamilyIndices mQueueFamilyIndices;
		QueueFamilyHandles mQueuesHandles;

		VkPhysicalDeviceProperties mPhysicalDeviceProperties;
		VkPhysicalDeviceFeatures mPhysicalDeviceFeatures;
		VkPhysicalDeviceMemoryProperties mPhysicalDeviceMemoryProperties;
		std::vector<VkQueueFamilyProperties> mPhysicalDeviceQueueFamilyProperties;
		std::vector<VkExtensionProperties> mPhysicalDeviceExtensionProperties;

		std::vector<const char*> mRequiredDeviceExtensions;
	};

} // namespace vk