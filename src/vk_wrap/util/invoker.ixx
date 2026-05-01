module;

#include <vulkan/vulkan.h>
#include <cassert>

export module mo_yanxi.vk.util:invoker;

import std;

namespace mo_yanxi::vk{
	template <::std::size_t index, typename... Ts>
	struct find;

	template <::std::size_t index, typename T, typename... Ts>
	struct find<index, T, Ts...>{
		using type = typename find<index - 1, Ts...>::type;
	};

	template <typename T, typename... Ts>
	struct find<0, T, Ts...>{
		using type = T;
	};

	template <::std::size_t index>
	struct find<index>{
		using type = ::std::nullptr_t;
	};

	template <typename T>
	struct InvokeResultPair{
		std::vector<T> vec{};
		VkResult rst{VK_SUCCESS};
	};

	template <typename... Ts>
	using get_last_t = typename find<sizeof...(Ts) - 1u, Ts...>::type;
	template <::std::size_t drop, typename... Ts>
	using get_last_n_t = typename find<sizeof...(Ts) - 1u - drop, Ts...>::type;

	export
	template <typename T, typename... Prms, typename... Args>
	[[nodiscard]] auto enumerate(T (*fn)(Prms...), Args... args) noexcept
		requires::std::invocable<T(*)(Prms...), Args..., ::std::uint32_t*, get_last_t<Prms...>>{
		assert(fn/*,"Invoke function cannot be empty."*/);

		if constexpr(!std::is_void_v<T>){
			InvokeResultPair<std::remove_pointer_t<get_last_t<Prms...>>> result{};

			::uint32_t nums;
			if((result.rst = fn(args..., &nums, nullptr)) != VK_SUCCESS) return result;
			result.vec.resize(nums);
			result.rst = fn(args..., &nums, result.vec.data());
			return result;
		} else{
			::std::vector<std::remove_pointer_t<get_last_t<Prms...>>> result{};

			::uint32_t nums;
			fn(args..., &nums, nullptr);

			result.resize(nums);
			fn(args..., &nums, result.data());

			return result;
		}
	}


	//WARNING: Use this method to invoke create pipeline, not the generic template.
	export
	auto Invoke(decltype(&::vkCreateGraphicsPipelines) fnCreate,
	            ::VkDevice device,
	            ::VkPipelineCache pipelineCache,
	            ::std::span<::VkGraphicsPipelineCreateInfo> createInfos,
	            const ::VkAllocationCallbacks* pAllocator){
		struct result_type{
			::std::vector<::VkPipeline> Result;
			::VkResult Code;
		};

		::std::vector<::VkPipeline> pipes{createInfos.size()};
		auto code = fnCreate(device, pipelineCache, static_cast<::uint32_t>(createInfos.size()), createInfos.data(),
		                     pAllocator, pipes.data());

		return result_type{::std::move(pipes), code};
	}

}
