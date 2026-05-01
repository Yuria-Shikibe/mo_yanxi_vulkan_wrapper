module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk:instance;

import mo_yanxi.vk.util;
import mo_yanxi.handle_wrapper;

import std;

namespace mo_yanxi::vk{
	[[nodiscard]] bool checkValidationLayerSupport(const decltype(used_validation_layers)& validationLayers = used_validation_layers){
		auto [availableLayers, rst] = enumerate(vkEnumerateInstanceLayerProperties);
		return check_support(validationLayers, availableLayers, &VkLayerProperties::layerName);

	}

	export
	struct instance : exclusive_handle<VkInstance>{
	public:
		[[nodiscard]] constexpr instance() = default;

		[[nodiscard]] explicit instance(const VkApplicationInfo& app_info, std::span<const char* const> required_exts){
			init(app_info, required_exts);
		}

	private:
		void init(const VkApplicationInfo& app_info, const std::span<const char* const> required_exts){
			if(handle != nullptr){
				throw std::runtime_error("Instance is already initialized");
			}

			VkValidationFeaturesEXT validation_features_ext{
				.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
				.pNext = nullptr,
				.enabledValidationFeatureCount = 0,
				.pEnabledValidationFeatures = nullptr,
				.disabledValidationFeatureCount = static_cast<std::uint32_t>(disabled_validation_features.size()),
				.pDisabledValidationFeatures = disabled_validation_features.data()
			};

			VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
			// createInfo.pNext = &validation_features_ext;
			createInfo.pApplicationInfo = &app_info;

			if(enable_validation_layers){
				if(!checkValidationLayerSupport(used_validation_layers)){
					throw std::runtime_error("validation layers requested, but not available!");
				}

				std::println("[Vulkan] Using Validation Layer: ");

				for(const auto& validationLayer : used_validation_layers){
					std::println("\t{}", validationLayer);
				}

				createInfo.enabledLayerCount = static_cast<std::uint32_t>(used_validation_layers.size());
				createInfo.ppEnabledLayerNames = used_validation_layers.data();
			} else{
				createInfo.enabledLayerCount = 0;
			}

			createInfo.enabledExtensionCount = static_cast<std::uint32_t>(required_exts.size());
			createInfo.ppEnabledExtensionNames = required_exts.data();

			auto cur = std::chrono::high_resolution_clock::now();
			if(const auto rst = vkCreateInstance(&createInfo, nullptr, &handle)){
				throw vk_error(rst, "failed to create vulkan instance!");
			}
			auto end = std::chrono::high_resolution_clock::now();

			std::println("[Vulkan] Instance Create Succeed: {:#X} - {}ms", std::bit_cast<std::uintptr_t>(handle), std::chrono::duration_cast<std::chrono::milliseconds>(end - cur).count());
		}

	public:
		~instance(){
			vkDestroyInstance(handle, nullptr);
		}

		instance(const instance& other) = delete;
		instance(instance&& other) noexcept = default;
		instance& operator=(const instance& other) = delete;
		instance& operator=(instance&& other) noexcept = default;

		[[nodiscard]] operator VkInstance() const noexcept{ return handle; }
	};
}