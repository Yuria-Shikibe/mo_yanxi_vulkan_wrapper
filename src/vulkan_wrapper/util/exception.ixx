module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.util:exception;

import std;

namespace mo_yanxi::vk{
	export struct vk_error final : std::exception{
		[[nodiscard]] vk_error(const VkResult result, const std::string_view msg)
			: std::exception(msg.data()), result{result}{
		}

		VkResult result{};

		[[nodiscard]] const char* what() const override{
			return std::exception::what();
		}
	};

	export struct unqualified_error final : std::exception{
		[[nodiscard]] unqualified_error() : std::exception("Env/Device Disabled"){

		}

		[[nodiscard]] unqualified_error(const std::string_view msg)
			: std::exception(msg.data()){
		}

		[[nodiscard]] const char* what() const override{
			return std::exception::what();
		}
	};

	struct ambiguous_error final : std::exception{
		[[nodiscard]] ambiguous_error() : std::exception("Ambiguous Setting"){

		}

		[[nodiscard]] ambiguous_error(const std::string_view msg)
			: std::exception(msg.data()){
		}

		[[nodiscard]] const char* what() const override{
			return std::exception::what();
		}
	};
}
