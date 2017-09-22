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
#include "Log.h"

namespace graphics
{

	namespace utils
	{

		namespace app
		{

			//! Retrieve the number of seconds that have elapsed since the application started.
			float get_elapsed_milliseconds();

			//! Retrieve the number of seconds that have elapsed since the application started.
			float get_elapsed_seconds();

		} // namespace app

		//! Determine whether or not an image format contains a depth component.
		bool is_depth_format(vk::Format format);

		//! Determine whether or not an image format contains a stencil component.
		bool is_stencil_format(vk::Format format);

		//! Translate an image format into the appropriate aspect mask flags.
		vk::ImageAspectFlags format_to_aspect_mask(vk::Format format);

		//! Translates a sample count (integer) into the correspond vk::SampleCountFlagBits. 
		//! A `count` of 4 would return vk::SampleCountFlagBits::e4, for example.
		vk::SampleCountFlagBits sample_count_to_flags(uint32_t count);
		
		namespace flags
		{

			inline vk::BufferUsageFlags buffer_usage_all() 
			{
				return vk::BufferUsageFlagBits::eIndexBuffer |
					   vk::BufferUsageFlagBits::eIndirectBuffer |
					   vk::BufferUsageFlagBits::eStorageBuffer |
					   vk::BufferUsageFlagBits::eStorageTexelBuffer |
					   vk::BufferUsageFlagBits::eTransferDst |
					   vk::BufferUsageFlagBits::eTransferSrc |
				 	   vk::BufferUsageFlagBits::eUniformBuffer |
					   vk::BufferUsageFlagBits::eUniformTexelBuffer |
					   vk::BufferUsageFlagBits::eVertexBuffer;
			}

			inline vk::ImageUsageFlags image_usage_all()
			{
				return vk::ImageUsageFlagBits::eColorAttachment |
					   vk::ImageUsageFlagBits::eDepthStencilAttachment |
					   vk::ImageUsageFlagBits::eInputAttachment |
					   vk::ImageUsageFlagBits::eSampled |
					   vk::ImageUsageFlagBits::eStorage |
					   vk::ImageUsageFlagBits::eTransferDst |
					   vk::ImageUsageFlagBits::eTransferSrc |
					   vk::ImageUsageFlagBits::eTransientAttachment;	
			}

		} // namespace flags

		namespace clear_color
		{

			inline vk::ClearColorValue red(float alpha = 1.0f)				{ return std::array<float, 4>{ 1.0f, 0.0f, 0.0f, alpha }; }
			inline vk::ClearColorValue green(float alpha = 1.0f)			{ return std::array<float, 4>{ 0.0f, 1.0f, 0.0f, alpha }; }
			inline vk::ClearColorValue blue(float alpha = 1.0f)				{ return std::array<float, 4>{ 0.0f, 0.0f, 1.0f, alpha }; }
			inline vk::ClearColorValue white(float alpha = 1.0f)			{ return std::array<float, 4>{ 1.0f, 1.0f, 1.0f, alpha }; }
			inline vk::ClearColorValue black(float alpha = 1.0f)			{ return std::array<float, 4>{ 0.0f, 0.0f, 0.0f, alpha }; }
			inline vk::ClearColorValue gray(float v, float alpha = 1.0f)	{ return std::array<float, 4>{ v, v, v, alpha }; }
		
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