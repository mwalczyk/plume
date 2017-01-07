#include "RenderPass.h"

namespace vk
{

	RenderPass::Options::Options()
	{

	}

	RenderPass::RenderPass(const DeviceRef &tDevice, const Options &tOptions) :
		mDevice(tDevice)
	{

	}

	RenderPass::~RenderPass()
	{

	}

} // namespace vk
