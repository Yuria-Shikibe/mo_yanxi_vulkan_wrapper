module;

#include <cassert>

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
export import :uniform_buffer;

export import :sampler;
export import :shader;

namespace mo_yanxi::vk{
	export
	template <std::unsigned_integral T>
	constexpr T align_up(T size, T alignment) noexcept {
		assert(std::has_single_bit(alignment));
		return (size + alignment - 1) & ~(alignment - 1);
	}
}
