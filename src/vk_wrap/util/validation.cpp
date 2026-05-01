module;

#include <vulkan/vulkan.h>
#include "../vk_loader.hpp"

module mo_yanxi.vk.util;

import mo_yanxi.stack_trace;

VkBool32 mo_yanxi::vk::validationCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData){

	std::string str{pCallbackData->pMessage};

	const auto back = std::ranges::find_last(str, '(').begin();
	std::ranges::replace(std::ranges::subrange{str.begin(), back}, ':', '\n');
	str.insert(back, '\n');

	std::println(std::cerr, "[Vulkan] ID:{} | {}", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName);

	std::println(std::cerr, "Current Thread: {}", std::this_thread::get_id());
	std::println(std::cerr, "{}", str);
	print_stack_trace(std::cerr, 1);

// #ifdef _MSC_VER
// 	__debugbreak();
// #endif

	return false;
}

VkResult mo_yanxi::vk::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback){

	if(auto* func = LoadFuncPtr(instance, vkCreateDebugUtilsMessengerEXT)){
		return func(instance, pCreateInfo, pAllocator, pCallback);
	} else{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void mo_yanxi::vk::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback,
	const VkAllocationCallbacks* pAllocator){
	if(auto* func = LoadFuncPtr(instance, vkDestroyDebugUtilsMessengerEXT)){
		func(instance, callback, pAllocator);
	}else{
		std::terminate();
	}
}
