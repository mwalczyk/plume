#include "App.h"

int main()
{
	VkPhysicalDeviceFeatures requiredFeatures = {};

	App myApp{ "test application", requiredFeatures };
	myApp.run();

	return 0;
}