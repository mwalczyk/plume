#include "Sampler.h"

namespace graphics
{

	Sampler::Options::Options()
	{
		m_address_mode_u = m_address_mode_v = m_address_mode_w = vk::SamplerAddressMode::eRepeat;
		m_min_filter = m_mag_filter = vk::Filter::eLinear;
		m_min_lod = m_max_lod = m_mip_lod_bias = 0.0f;
		m_anistropy_enabled = VK_FALSE;
		m_max_anistropy = 1.0f;
		m_border_color = vk::BorderColor::eIntOpaqueBlack;
		m_mipmap_mode = vk::SamplerMipmapMode::eLinear;
		m_unnormalized_coordinates = VK_FALSE;
		m_compare_op_enable = VK_FALSE;
		m_compare_op = vk::CompareOp::eAlways;
	}

	Sampler::Sampler(DeviceWeakRef device, const Options& options) :
		
		m_device(device)
	{
		DeviceRef device_shared = m_device.lock();

		vk::SamplerCreateInfo sampler_create_info;
		sampler_create_info.addressModeU = options.m_address_mode_u;
		sampler_create_info.addressModeV = options.m_address_mode_v;
		sampler_create_info.addressModeW = options.m_address_mode_w;
		sampler_create_info.anisotropyEnable = options.m_anistropy_enabled;
		sampler_create_info.borderColor = options.m_border_color;
		sampler_create_info.compareEnable = options.m_compare_op_enable;
		sampler_create_info.compareOp = options.m_compare_op;
		sampler_create_info.magFilter = options.m_mag_filter;
		sampler_create_info.maxAnisotropy = options.m_max_anistropy;
		sampler_create_info.maxLod = options.m_max_lod;
		sampler_create_info.minFilter = options.m_min_filter;
		sampler_create_info.minLod = options.m_min_lod;
		sampler_create_info.mipLodBias = options.m_mip_lod_bias;
		sampler_create_info.mipmapMode = options.m_mipmap_mode;
		sampler_create_info.unnormalizedCoordinates = options.m_unnormalized_coordinates;

		m_sampler_handle = device_shared->get_handle().createSampler(sampler_create_info);
	}

	Sampler::~Sampler()
	{
		DeviceRef device_shared = m_device.lock();

		device_shared->get_handle().destroySampler(m_sampler_handle);
	}

} // namespace graphics