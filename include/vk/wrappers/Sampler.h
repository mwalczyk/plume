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

#include "Device.h"

namespace plume
{

	namespace graphics
	{

		//! Image samplers are used by the implementation to read image data and apply filtering and other transformations
		//! inside of a shader. Note that a single sampler can be used with multiple attachments when constructing 
		//! descriptor sets. 
		class Sampler
		{
		public:

			class Options
			{
			public:
				Options();

				//! Sets all three sampler address modes (u, v, w) simultaneously.
				Options& address_modes(vk::SamplerAddressMode mode) { m_address_mode_u = m_address_mode_v = m_address_mode_w = mode; return *this; }

				//! Sets each sampler address mode (u, v, w) independently.
				Options& address_modes(vk::SamplerAddressMode u, vk::SamplerAddressMode v, vk::SamplerAddressMode w)
				{
					m_address_mode_u = u;
					m_address_mode_v = v;
					m_address_mode_w = w;
					return *this;
				}

				//! Sets the minification and magnification filter modes for the sampler.
				Options& min_mag_filters(vk::Filter min, vk::Filter mag)
				{
					m_min_filter = min;
					m_mag_filter = mag;
					return *this;
				}

				//! Sets both the minification and magnification filter modes for the sampler.
				Options& min_mag_filters(vk::Filter filter) { m_min_filter = m_mag_filter = filter; return *this; }

				//! Sets the anistropy value clamp and enables texel anistropic filtering. Note 
				//! that this value must be between 1.0f and vk::PhysicalDeviceLimits::maxSamplerAnisotropy,
				//! inclusive.
				Options& max_anistropy(float max_anistropy = 16.0f)
				{
					m_anistropy_enabled = VK_TRUE;
					m_max_anistropy = max_anistropy;
					return *this;
				}

				//! Sets the values that are used to clamp the computed level-of-detail value. Note
				//! that the maximum LOD must be greater than or equal to the minimum LOD. 
				Options& lod(float min, float max, float mip_bias)
				{
					m_min_lod = min;
					m_max_lod = (max < min) ? min : max;
					m_mip_lod_bias = mip_bias;
					return *this;
				}

				//! Sets the border color used by the sampler.
				Options& border_color(vk::BorderColor border_color = vk::BorderColor::eIntOpaqueWhite) { m_border_color = border_color; return *this; }

				//! Sets the mipmap mode used by the sampler. Possible values are vk::SamplerMipmapMode::eLinear
				//! (default) and vk::SamplerMipmapMode::eNearest.
				Options& mipmap_mode(vk::SamplerMipmapMode mipmap_mode) { m_mipmap_mode = mipmap_mode; return *this; }

				//! By default, the sampler assumes that the application is using normalized texture coordinates.
				//! Calling this function disables this behavior. Note that enabling this features places significant
				//! restrictions on the corresponding image view that will be sampled in the shader. See the spec for 
				//! details.
				Options& enable_unnormalized_coordinates() { m_unnormalized_coordinates = VK_TRUE;  return *this; }

				//! Enables and sets the comparison function that is applied to fetched data before filtering.
				Options& compare_op(vk::CompareOp compare_op)
				{
					m_compare_op_enable = VK_TRUE;
					m_compare_op = compare_op;
					return *this;
				}

			private:

				vk::SamplerAddressMode m_address_mode_u;
				vk::SamplerAddressMode m_address_mode_v;
				vk::SamplerAddressMode m_address_mode_w;
				vk::Filter m_min_filter;
				vk::Filter m_mag_filter;
				float m_min_lod;
				float m_max_lod;
				float m_mip_lod_bias;
				vk::Bool32 m_anistropy_enabled;
				float m_max_anistropy;
				vk::BorderColor m_border_color;
				vk::SamplerMipmapMode m_mipmap_mode;
				vk::Bool32 m_unnormalized_coordinates;
				vk::Bool32 m_compare_op_enable;
				vk::CompareOp m_compare_op;

				friend class Sampler;
			};

			Sampler(const Device& device, const Options& options = Options());

			vk::Sampler get_handle() const { return m_sampler_handle.get(); }

		private:

			const Device* m_device_ptr;
			vk::UniqueSampler m_sampler_handle;
		};

	} // namespace graphics

} // namespace plume