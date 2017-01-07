#include "App.h"
#include "Vk.h"

int main()
{
	/*
	App myApp{ "test application", {} };
	myApp.run();
	*/

	/// vk::Instance
	auto instance = vk::Instance::create();
	auto physicalDevices = instance->getPhysicalDevices();
	assert(physicalDevices.size() > 0);

	/// vk::Window
	auto windowOptions = vk::Window::Options()
		.width(900)
		.height(300)
		.title("Test Application");
	auto window = vk::Window::create(instance, windowOptions);

	/// vk::Surface
	auto surface = window->createSurface();

	/// vk::Device
	auto deviceOptions = vk::Device::Options()
		.requiredQueueFlags(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
	auto device = vk::Device::create(physicalDevices[0], deviceOptions);

	auto queueFamilyProperties = device->getPhysicalDeviceQueueFamilyProperties();
	for (size_t i = 0; i < queueFamilyProperties.size(); ++i)
	{
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device->getPhysicalDeviceHandle(), i, surface->getHandle(), &presentSupport);

		if (presentSupport)
		{
			std::cout << "Queue Family at index " << i << " supports presentation\n";
		}
	}

	/// vk::Swapchain
	auto swapchain = vk::Swapchain::create(device, surface);

	/// vk::Pipeline
	auto pipeline = vk::Pipeline::create(device);

	while (!window->shouldWindowClose())
	{
		window->pollEvents();
	}

	return 0;
}