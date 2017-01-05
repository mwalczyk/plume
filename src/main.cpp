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
	/// The Window class creates a Surface object
	auto windowOptions = vk::Window::Options();
	windowOptions.width(900);
	windowOptions.height(300);
	windowOptions.title("Test Application");
	auto window = vk::Window::create(instance, windowOptions);

	/// vk::Device
	auto device = vk::Device::create(physicalDevices[0]);

	while (!window->shouldWindowClose())
	{
		window->pollEvents();
	}

	return 0;
}