#pragma once

#define NOMINMAX

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	#define SPECTRA_MSW
#elif defined(linux) || defined(__linux) || defined(__linux__)
	#define SPECTRA_LINUX
#endif

#if defined(SPECTRA_MSW)
	#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(SPECTRA_LINUX)
	#define VK_USE_PLATFORM_XCB_KHR
#endif

#include "vulkan/vulkan.hpp"
