#pragma once

#include <memory>
#include <vector>
#include <map>
#include <cassert>

#include "Platform.h"
#include "Noncopyable.h"
#include "Instance.h"
#include "Surface.h"

namespace graphics
{

	class Device;
	using DeviceRef = std::shared_ptr<Device>;

	class Device : public Noncopyable
	{
	public:

		struct SwapchainSupportDetails
		{
			vk::SurfaceCapabilitiesKHR mCapabilities;
			std::vector<vk::SurfaceFormatKHR> mFormats;
			std::vector<vk::PresentModeKHR> mPresentModes;
		};

		class QueueFamiliesMapping
		{
		public:
			QueueFamiliesMapping() = default;		

			std::pair<vk::Queue, uint32_t> graphics() const { return mGraphicsQueue; }
			std::pair<vk::Queue, uint32_t> compute() const { return mComputeQueue; }
			std::pair<vk::Queue, uint32_t> transfer() const { return mTransferQueue; }
			std::pair<vk::Queue, uint32_t> sparseBinding() const { return mSparseBindingQueue; }
			std::pair<vk::Queue, uint32_t> presentation() const { return mPresentationQueue; }

		private:

			std::pair<vk::Queue, uint32_t> mGraphicsQueue = { VK_NULL_HANDLE, 0 };
			std::pair<vk::Queue, uint32_t> mComputeQueue = { VK_NULL_HANDLE, 0 };
			std::pair<vk::Queue, uint32_t> mTransferQueue = { VK_NULL_HANDLE, 0 };
			std::pair<vk::Queue, uint32_t> mSparseBindingQueue = { VK_NULL_HANDLE, 0 };
			std::pair<vk::Queue, uint32_t> mPresentationQueue = { VK_NULL_HANDLE, 0 };

			friend class Device;
		};

		class Options
		{
		public:

			Options();

			Options& requiredQueueFlags(vk::QueueFlags tRequiredQueueFlags) { mRequiredQueueFlags = tRequiredQueueFlags; return *this; }
			Options& requiredDeviceExtensions(const std::vector<const char*> &tRequiredDeviceExtensions) { mRequiredDeviceExtensions = tRequiredDeviceExtensions; return *this; }
			Options& useSwapchain(bool tUseSwapchain) { mUseSwapchain = tUseSwapchain; return *this; }

		private:

			vk::QueueFlags mRequiredQueueFlags;
			std::vector<const char*> mRequiredDeviceExtensions;
			bool mUseSwapchain;

			friend class Device;
		};

		//! Factory method for returning a new DeviceRef.
		static DeviceRef create(vk::PhysicalDevice tPhysicalDevice, const Options &tOptions = Options())
		{
			return std::make_shared<Device>(tPhysicalDevice, tOptions);
		}

		//! Construct a logical device around a physical device (GPU).
		Device(vk::PhysicalDevice tPhysicalDevice, const Options &tOptions = Options());
		~Device();

		inline vk::Device getHandle() const { return mDeviceHandle; };
		inline vk::PhysicalDevice getPhysicalDeviceHandle() const { return mPhysicalDeviceHandle; }
		inline vk::PhysicalDeviceProperties getPhysicalDeviceProperties() const { return mPhysicalDeviceProperties; }
		inline vk::PhysicalDeviceFeatures getPhysicalDeviceFeatures() const { return mPhysicalDeviceFeatures;  }
		inline vk::PhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties() const { return mPhysicalDeviceMemoryProperties; }
		inline const QueueFamiliesMapping& getQueueFamiliesMapping() const { return mQueueFamiliesMapping; }
		inline const std::vector<vk::QueueFamilyProperties>& getPhysicalDeviceQueueFamilyProperties() const { return mPhysicalDeviceQueueFamilyProperties; }
		inline const std::vector<vk::ExtensionProperties>& getPhysicalDeviceExtensionProperties() const { return mPhysicalDeviceExtensionProperties; }
		
		//! Format features are properties of the physical device.
		inline vk::FormatProperties getPhysicalDeviceFormatProperties(vk::Format tFormat) const { return mPhysicalDeviceHandle.getFormatProperties(tFormat); }
		
		//! Depth formats are not necessarily supported by the system. Retrieve the highest precision format available.
		vk::Format getSupportedDepthFormat() const;

		//! Returns a structure that contains information related to the chosen physical device's swapchain support.
		SwapchainSupportDetails getSwapchainSupportDetails(const SurfaceRef &tSurface) const;

		friend std::ostream& operator<<(std::ostream &tStream, const DeviceRef &tDevice);

	private:

		uint32_t findQueueFamilyIndex(vk::QueueFlagBits tQueueFlagBits) const;

		vk::Device mDeviceHandle;
		vk::PhysicalDevice mPhysicalDeviceHandle;
		vk::PhysicalDeviceProperties mPhysicalDeviceProperties;
		vk::PhysicalDeviceFeatures mPhysicalDeviceFeatures;
		vk::PhysicalDeviceMemoryProperties mPhysicalDeviceMemoryProperties;
		std::vector<vk::QueueFamilyProperties> mPhysicalDeviceQueueFamilyProperties;
		std::vector<vk::ExtensionProperties> mPhysicalDeviceExtensionProperties;
		QueueFamiliesMapping mQueueFamiliesMapping;
		std::vector<const char*> mRequiredDeviceExtensions;
	};

} // namespace graphics