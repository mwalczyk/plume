/*
*
* MIT License
*
* Copyright(c) 2017 Michael Walczyk
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files(the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions :
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
*/

#include "Instance.h"

namespace graphics
{

	Instance::Options::Options()
	{
		m_application_info.apiVersion = VK_API_VERSION_1_0;
		m_application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		m_application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		m_application_info.pApplicationName = "Plume Application";
		m_application_info.pEngineName = "Plume Engine";

		m_debug_report_flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	}

	//! Proxy function for creating a debug callback object
	vk::Result create_debug_report_callback(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* create_info, const VkAllocationCallbacks* allocator, VkDebugReportCallbackEXT* call_back)
	{
		auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

		if (func != nullptr)
		{
			return static_cast<vk::Result>(func(instance, create_info, allocator, call_back));
		}
		else
		{
			return vk::Result::eErrorExtensionNotPresent;
		}
	}

	//! Proxy function for destroying a debug callback object
	void destroy_debug_report_callback(vk::Instance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* allocator)
	{
		auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");

		if (func != nullptr)
		{
			func(static_cast<VkInstance>(instance), callback, allocator);
		}
	}

	Instance::Instance(const Options& options) :
		m_required_layers(options.m_required_layers),
		m_required_extensions(options.m_required_extensions)
	{
		// Store the instance extension properties.
		m_instance_extension_properties = vk::enumerateInstanceExtensionProperties();

		// Store the instance layer properties.
		m_instance_layer_properties = vk::enumerateInstanceLayerProperties();

		if (!check_instance_layer_support())
		{
			throw std::runtime_error("One or more of the requested validation layers are not supported on this platform");
		}

		// Append the instance extensions required by the windowing system.
#if defined(PLUME_MSW)
		m_required_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(PLUME_LINUX)
		m_required_extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
		m_required_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

		// If building in debug mode, automatically enable the standard validation 
		// layer and the debug report callback extension.
#if defined(_DEBUG)
		m_required_layers.push_back("VK_LAYER_LUNARG_standard_validation");
		m_required_extensions.push_back("VK_EXT_debug_report");
#endif

		// Create the instance.
		vk::InstanceCreateInfo instance_create_info;
		instance_create_info.enabledExtensionCount = static_cast<uint32_t>(m_required_extensions.size());
		instance_create_info.enabledLayerCount = static_cast<uint32_t>(m_required_layers.size());
		instance_create_info.pApplicationInfo = &options.m_application_info;
		instance_create_info.ppEnabledExtensionNames = m_required_extensions.data();
		instance_create_info.ppEnabledLayerNames = m_required_layers.data();

		m_instance_handle = vk::createInstance(instance_create_info);

		// Only set up the debug callback object if the VK_EXT_debug_report extension is present 
		// (note that this needs to happen after initialization).
#if defined(_DEBUG)
		setup_debug_report_callback(options.m_debug_report_flags);
#endif

		// Store the handles to each of the present physical devices (note that this needs to happen after initialization).
		m_physical_devices = m_instance_handle.enumeratePhysicalDevices();
	}

	Instance::~Instance()
	{
		destroy_debug_report_callback(m_instance_handle, m_debug_report_callback, nullptr);
		
		m_instance_handle.destroy();
	}

	vk::PhysicalDevice Instance::pick_physical_device(const std::function<bool(vk::PhysicalDevice)>& func)
	{
		for (const auto& physical_device : m_physical_devices)
		{
			if (func(physical_device))
			{
				return physical_device;
			}
		}
		
		return{};
	}

	bool Instance::check_instance_layer_support()
	{
		for (const auto& required_layer_name : m_required_layers)
		{
			auto predicate = [&](const vk::LayerProperties &layerProperty) { return strcmp(required_layer_name, layerProperty.layerName) == 0; };
			if (std::find_if(m_instance_layer_properties.begin(), m_instance_layer_properties.end(), predicate) == m_instance_layer_properties.end())
			{
				std::cerr << "Required layer " << required_layer_name << " is not supported\n";
				return false;
			}
		}

		return true;
	}

	void Instance::setup_debug_report_callback(VkDebugReportFlagsEXT debug_report_flags)
	{
		VkDebugReportCallbackCreateInfoEXT debug_report_callback_create_info = {};
		debug_report_callback_create_info.flags = debug_report_flags;
		debug_report_callback_create_info.pfnCallback = debug_callback;
		debug_report_callback_create_info.pUserData = nullptr;
		debug_report_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		
		create_debug_report_callback(m_instance_handle, &debug_report_callback_create_info, nullptr, &m_debug_report_callback);
	}

} // namespace graphics