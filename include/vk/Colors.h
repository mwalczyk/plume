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

#pragma once

#include "Platform.h"

namespace graphics
{
	namespace clear
	{
		vk::ClearColorValue RED =	std::array<float, 4>{ 1.0f, 0.0f, 0.0f, 1.0f };
		vk::ClearColorValue GREEN = std::array<float, 4>{ 0.0f, 0.0f, 1.0f, 1.0f };
		vk::ClearColorValue BLUE =	std::array<float, 4>{ 0.0f, 1.0f, 0.0f, 1.0f };
		vk::ClearColorValue WHITE = std::array<float, 4>{ 1.0f, 1.0f, 1.0f, 1.0f };
		vk::ClearColorValue BLACK = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f };

		vk::ClearDepthStencilValue DEPTH_ZERO = { 0.0f, 0 };
		vk::ClearDepthStencilValue DEPTH_ONE =	{ 1.0f, 0 };
		vk::ClearDepthStencilValue DEPTH_ZERO_STENCIL_ONE = { 0.0f, 1 };
		vk::ClearDepthStencilValue DEPTH_ONE_STENCIL_ONE =	{ 1.0f, 1 };
	}
}