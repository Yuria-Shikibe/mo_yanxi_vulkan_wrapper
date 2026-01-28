module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.util;

import mo_yanxi.tuple_manipulate;


export import :concepts;
export import :invoker;
export import :exception;
export import :vma;
export import :validation;
import std;

namespace mo_yanxi::vk{
export std::vector<VkExtensionProperties> get_valid_extensions(){
	auto [extensions, rst] = enumerate(vkEnumerateInstanceExtensionProperties, nullptr);

	return extensions;
}


export
template <std::ranges::range Range, std::ranges::range ValidRange, typename Proj = std::identity>
	requires requires(
		std::unordered_set<std::string>& set,
		std::indirect_result_t<Proj, std::ranges::const_iterator_t<ValidRange>> value
	){
	requires std::constructible_from<std::string, std::ranges::range_value_t<Range>>;
	// set.erase(value);
	}
[[nodiscard]] bool check_support(const Range& required, const ValidRange& valids, Proj proj = {}){
	std::unordered_set<std::string> requiredExtensions(std::ranges::begin(required), std::ranges::end(required));

	for(const auto& extension : valids){
		requiredExtensions.erase(std::invoke(proj, extension));
	}

	return requiredExtensions.empty();
}
}
