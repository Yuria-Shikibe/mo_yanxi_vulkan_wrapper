module;

#include <cassert>
#include <vulkan/vulkan.h>

export module mo_yanxi.vk;

export import :ext;
export import :swap_chain_info;
export import :sync;
export import :logical_deivce;
export import :physical_device;
export import :instance;
export import :pipeline;
export import :pipeline_layout;

export import :command_buffer;
export import :resources;
export import :aliasing;
export import :image_derives;
export import :descriptor_buffer;
export import :descriptor_heap;
export import :uniform_buffer;

export import :sampler;
export import :shader;
import std;

namespace mo_yanxi::vk{
	export
	template <std::unsigned_integral T>
	constexpr T align_up(T size, T alignment) noexcept {
		assert(std::has_single_bit(alignment));
		return (size + alignment - 1) & ~(alignment - 1);
	}

struct physical_device_requirements {
	VkDeviceSize min_uniform_buffer_offset_alignment;
	VkDeviceSize min_storage_buffer_offset_alignment;
};

struct  device_requirement_kv{
	VkDevice device;
	physical_device_requirements prop;
};

extern std::vector<device_requirement_kv> global_device_requirements;


export
inline void register_requirement(VkDevice device, const physical_device_requirements& requirements) {
	if (const auto it = std::ranges::find(global_device_requirements, device, &device_requirement_kv::device); it != global_device_requirements.end()) {
		it->prop = requirements;
	} else {
		global_device_requirements.push_back({device, requirements});
	}
}

export
inline std::optional<physical_device_requirements> get_device_requirement(VkDevice device) noexcept {
	if (const auto it = std::ranges::find(global_device_requirements, device, &device_requirement_kv::device); it != global_device_requirements.end()) {
		return it->prop;
	}

	return std::nullopt;
}

export
inline void register_default_requirements(VkDevice device, VkPhysicalDevice physical_device) {
	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(physical_device, &device_properties);

	physical_device_requirements reqs{};
	reqs.min_uniform_buffer_offset_alignment = device_properties.limits.minUniformBufferOffsetAlignment;
	reqs.min_storage_buffer_offset_alignment = device_properties.limits.minStorageBufferOffsetAlignment;

	register_requirement(device, reqs);
}

}

