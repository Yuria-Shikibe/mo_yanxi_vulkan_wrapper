module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.util:validation;

import :exception;
import mo_yanxi.handle_wrapper;
import std;

namespace mo_yanxi::vk{
	export inline bool enable_validation_layers{false};

	export constexpr std::array used_validation_layers{
		(const char*)"VK_LAYER_KHRONOS_validation",
	};

	export constexpr std::array disabled_validation_features{
		VK_VALIDATION_FEATURE_DISABLE_UNIQUE_HANDLES_EXT
	};

	VKAPI_ATTR VkBool32 VKAPI_CALL validationCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pCallback);

	void DestroyDebugUtilsMessengerEXT(
		VkInstance instance,
		VkDebugUtilsMessengerEXT callback,
		const VkAllocationCallbacks* pAllocator
	);


	export
	class validation_entry{
		exclusive_handle_member<VkInstance> instance{};
		exclusive_handle_member<VkDebugUtilsMessengerEXT> callback{};

	public:
		[[nodiscard]] constexpr validation_entry() noexcept = default;

		[[nodiscard]] explicit validation_entry(
			VkInstance instance,

			PFN_vkDebugUtilsMessengerCallbackEXT callbackFuncPtr = validationCallback,

			const VkDebugUtilsMessengerCreateFlagsEXT flags = 0,

			const VkDebugUtilsMessageSeverityFlagsEXT messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,

			const VkDebugUtilsMessageTypeFlagsEXT messageType =
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
			) : instance(instance){

			if (enable_validation_layers){
				const VkDebugUtilsMessengerCreateInfoEXT createInfo{
					.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
					.pNext = nullptr,
					.flags = flags,
					.messageSeverity = messageSeverity,
					.messageType = messageType,
					.pfnUserCallback = callbackFuncPtr,
					.pUserData = nullptr
				};

				if(auto rst = CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, callback.as_data())){
					throw vk_error(rst, "Failed to set up debug callback!");
				}

				std::println("[Vulkan] Validation Callback Setup Succeed: {:#X}", std::bit_cast<std::uintptr_t>(callbackFuncPtr));
			}
		}

		~validation_entry(){
			if (enable_validation_layers){
				if(instance)DestroyDebugUtilsMessengerEXT(instance, callback, nullptr);
			}
		}

		validation_entry(const validation_entry& other) = delete;
		validation_entry(validation_entry&& other) noexcept = default;
		validation_entry& operator=(const validation_entry& other) = delete;
		validation_entry& operator=(validation_entry&& other) noexcept = default;
	};
}
