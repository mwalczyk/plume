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

#include <chrono>
#include <iostream>

#include "Platform.h"

namespace graphics
{

	namespace utils
	{

		//! Determine whether or not an image format contains a depth component.
		bool is_depth_format(vk::Format format);

		//! Determine whether or not an image format contains a stencil component.
		bool is_stencil_format(vk::Format format);

		//! Translate an image format into the appropriate aspect mask flags.
		vk::ImageAspectFlags format_to_aspect_mask(vk::Format format);

		//! Translates a sample count (integer) into the correspond vk::SampleCountFlagBits. 
		//! A `count` of 4 would return vk::SampleCountFlagBits::e4, for example.
		vk::SampleCountFlagBits sample_count_to_flags(uint32_t count);
		
		namespace clear_color
		{

			inline vk::ClearColorValue red()	{ return std::array<float, 4>{ 1.0f, 0.0f, 0.0f, 1.0f }; }
			inline vk::ClearColorValue green()	{ return std::array<float, 4>{ 0.0f, 1.0f, 0.0f, 1.0f }; }
			inline vk::ClearColorValue blue()	{ return std::array<float, 4>{ 0.0f, 0.0f, 1.0f, 1.0f }; }
			inline vk::ClearColorValue white()	{ return std::array<float, 4>{ 1.0f, 1.0f, 1.0f, 1.0f }; }
			inline vk::ClearColorValue black()	{ return std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f }; }
		
		} // namespace clear_color

		namespace clear_depth
		{

			inline vk::ClearDepthStencilValue depth_zero()				{ return { 0.0f, 0 }; }
			inline vk::ClearDepthStencilValue depth_one()				{ return { 1.0f, 0 }; }
			inline vk::ClearDepthStencilValue depth_zero_stencil_one()	{ return { 0.0f, 1 }; }
			inline vk::ClearDepthStencilValue depth_one_stencil_one()	{ return { 1.0f, 1 }; }

		} // namespace clear_depth

	} // namespace utils

} // namespace graphics