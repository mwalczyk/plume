#pragma once

#include <memory>

#include "Platform.h"
#include "Device.h"

namespace vk
{

	class RenderPass;
	using RenderPassRef = std::shared_ptr<RenderPass>;

	class RenderPass
	{

	public:

		struct Options
		{
			Options();

			float a;
			float b;
		};

		//! Factory method for returning a new RenderPassRef.
		static RenderPassRef create(const DeviceRef &tDevice, const Options &tOptions = Options())
		{
			return std::make_shared<RenderPass>(tDevice, tOptions);
		}

		RenderPass(const DeviceRef &tDevice, const Options &tOptions = Options());
		~RenderPass();

		inline VkRenderPass getHandle() const { return mRenderPassHandle; }

	private:

		VkRenderPass mRenderPassHandle;

		DeviceRef mDevice;

	};

} // namespace vk