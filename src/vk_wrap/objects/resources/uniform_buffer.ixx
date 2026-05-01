module;

#include <vulkan/vulkan.h>
#include <cassert>
#include <vk_mem_alloc.h>

export module mo_yanxi.vk:uniform_buffer;

import :resources;
import mo_yanxi.vk.util;


namespace mo_yanxi::vk{
	export
	struct uniform_buffer : buffer_cpu_to_gpu{
		using buffer_cpu_to_gpu::buffer_cpu_to_gpu;

		[[nodiscard]] uniform_buffer(
			const allocator_usage allocator,
			const VkDeviceSize size,
			const VkBufferUsageFlags append_usage = 0)
			: buffer_cpu_to_gpu(allocator, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | append_usage){
		}

		[[nodiscard]] VkDescriptorBufferInfo get_descriptor_info() const noexcept{
			return {
				.buffer = handle,
				.offset = 0,
				.range = get_size()
			};
		}

		[[nodiscard]] VkDescriptorBufferInfo get_descriptor_info(
			const VkDeviceSize offset,
			const VkDeviceSize range
		) const noexcept{
			assert(offset + range <= get_size());
			return {
				.buffer = handle,
				.offset = offset,
				.range = range
			};
		}
	};
}
