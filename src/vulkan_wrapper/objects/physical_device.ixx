module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk:physical_device;

import std;

import mo_yanxi.vk.util;
import :swap_chain_info;

import mo_yanxi.meta_programming;

namespace mo_yanxi::vk{

	export
	struct queue_family_indices{
		struct family_data{
			static constexpr auto invalid_family = std::numeric_limits<std::uint32_t>::max();

			std::uint32_t index{invalid_family};
			std::uint32_t count{};

			void create_queues(VkDevice device, std::vector<VkQueue>& queues) const{
				queues.resize(count);

				for (auto && [i, _] : queues | std::views::enumerate){
					vkGetDeviceQueue(device, index, static_cast<std::uint32_t>(i), queues.data() + i);
				}
			}

			[[nodiscard]] constexpr explicit operator bool() const noexcept{
				return index != invalid_family;
			}

			constexpr friend bool operator==(const family_data& lhs, const family_data& rhs){
				return lhs.index == rhs.index;
			}
		};

		family_data graphic{};
		family_data present{};
		family_data compute{};

		[[nodiscard]] constexpr bool valid() const noexcept{
			return graphic && present && compute;
		}

		[[nodiscard]] explicit operator bool() const noexcept{
			return valid();
		}

		[[nodiscard]] queue_family_indices() = default;

		[[nodiscard]] queue_family_indices(VkPhysicalDevice device, VkSurfaceKHR surface){
			std::vector<VkQueueFamilyProperties> queueFamilies = enumerate(vkGetPhysicalDeviceQueueFamilyProperties, device);

			for(const auto& [index, queueFamily] : queueFamilies | std::ranges::views::enumerate){
				if(!queueFamily.queueCount) continue;

				if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT){
					graphic = {static_cast<std::uint32_t>(index), queueFamily.queueCount};
				}

				// if(index != 0){
					if(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT){
						compute = {static_cast<std::uint32_t>(index), queueFamily.queueCount};
					}
				// }


				//Obtain present queue
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, static_cast<std::uint32_t>(index), surface, &presentSupport);
				if(presentSupport){
					present = {static_cast<std::uint32_t>(index), 1};
				}

				if(valid()){
					break;
				}
			}

			if(!valid()){
				std::println(std::cerr, "[Vulkan] Queue Family Incomplete");
				throw unqualified_error{"Queue Family Incomplete"};
			}
		}
	};

	export
	struct physical_device{
		VkPhysicalDevice device{};

		// Properties
		queue_family_indices queues{};
		VkPhysicalDeviceMemoryProperties deviceMemoryProperties{};

		[[nodiscard]] constexpr std::uint32_t find_memory_type(
			const std::uint32_t typeFilter,
			const VkMemoryPropertyFlags properties
		) const{
			for(std::uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++){
				if(typeFilter & (1u << i) && (deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties){
					return i;
				}
			}

			throw unqualified_error("failed to find suitable memory type!");
		}

		[[nodiscard]] explicit operator bool() const noexcept{
			return device != nullptr;
		}

		[[nodiscard]] VkPhysicalDeviceProperties get_device_properties() const noexcept{
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);
			return deviceProperties;
		}

		[[nodiscard]] std::uint32_t rate_device() const noexcept{
			VkPhysicalDeviceProperties deviceProperties = get_device_properties();

			std::uint32_t score = 0;

			// Discrete GPUs have a significant performance advantage
			if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
				score += 1000;
			}

			// Maximum possible size of textures affects graphics quality
			score += deviceProperties.limits.maxImageDimension2D;

			return score;
		}

		[[nodiscard]] bool check_extension_support(const auto& requiredExtensions) const{
			if(auto [availableExtensions, rst] = enumerate(vkEnumerateDeviceExtensionProperties, device, nullptr); rst){
				throw vk_error{rst, "Failed to get device extensions"};
			}else{
				return vk::check_support(requiredExtensions, availableExtensions, &VkExtensionProperties::extensionName);
			}

		}

		[[nodiscard]] std::string get_name() const{
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);

			return std::string{deviceProperties.deviceName};
		}

		template <typename... Args>
			requires requires(VkPhysicalDeviceFeatures features, Args... args){
				requires (std::same_as<VkBool32, typename mptr_info<Args>::value_type> && ...);
				requires (std::is_member_object_pointer_v<Args> && ...);
			}
		[[nodiscard]] bool meet_features(Args... args) const{
			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

			return (static_cast<bool>(deviceFeatures.*args) && ...);
		}

		[[nodiscard]] bool meet_features(const VkPhysicalDeviceFeatures& required) const noexcept{
			static constexpr std::size_t FeatureListSize = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);

			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

			auto& availableSrc = reinterpret_cast<const std::array<VkBool32, FeatureListSize>&>(deviceFeatures);
			auto& requiredSrc = reinterpret_cast<const std::array<VkBool32, FeatureListSize>&>(required);

			for(std::size_t i = 0; i < FeatureListSize; ++i){
				if(!availableSrc[i] && requiredSrc[i]) return false;
			}

			return true;
		}

		[[nodiscard]] explicit(false) constexpr operator VkPhysicalDevice() const noexcept{
			return device;
		}

		bool valid(VkSurfaceKHR surface = nullptr, std::span<const char* const> device_extensions = {}) const{
			const bool extensionsSupported = check_extension_support(device_extensions);
			const bool featuresMeet = meet_features(&VkPhysicalDeviceFeatures::samplerAnisotropy);

			if(!featuresMeet || !extensionsSupported)return false;

			if(!surface)return true;

			const queue_family_indices indices(device, surface);
			const swap_chain_info swapChainSupport(device, surface);

			return indices.valid() && swapChainSupport.valid();
		}

		void cache_properties(VkSurfaceKHR surface){
			vkGetPhysicalDeviceMemoryProperties(device, &deviceMemoryProperties);
			queues = queue_family_indices{device, surface};
		}
	};
}

export
template <>
struct std::hash<mo_yanxi::vk::queue_family_indices::family_data>{
	constexpr std::size_t operator()(const mo_yanxi::vk::queue_family_indices::family_data& index) const noexcept{
		return index.index;
	}
};