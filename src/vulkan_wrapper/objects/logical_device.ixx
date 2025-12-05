module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk:logical_deivce;

export import mo_yanxi.handle_wrapper;
import :physical_device;
import mo_yanxi.vk.util;
import std;

namespace mo_yanxi::vk{
export
template <typename... T>
struct extension_chain{
private:
	using Chain = std::tuple<T...>;
	Chain chain{};

public:
	template <typename... Args>
	[[nodiscard]] explicit extension_chain(Args&&... args) : chain{args...}{
		create();
	}

private:
	template <typename T1, typename T2>
	static void set(T1& t1, T2& t2){
		t1.pNext = &t2;
	}

	template <typename T1, typename T2>
	static void check(T1& t1, T2& t2){
		if(t1.pNext != &t2){
			throw std::runtime_error("Chain Failed");
		}
	}

	void create(){
		[&]<std::size_t ... I>(std::index_sequence<I...>){
			((std::get<I>(chain).pNext = nullptr), ...);
		}(std::make_index_sequence<sizeof...(T)>{});

		[&]<std::size_t ... I>(std::index_sequence<I...>){
			(this->set(std::get<I>(chain), std::get<I + 1>(chain)), ...);
		}(std::make_index_sequence<sizeof...(T) - 1>{});
	}

public:
	[[nodiscard]] void* get_first() const noexcept{
		if constexpr (sizeof...(T) > 0){
			return const_cast<std::tuple_element_t<0, Chain>*>(&std::get<0>(chain));
		}else{
			return nullptr;
		}
	}
};

template <typename... Args>
extension_chain(Args&&...) -> extension_chain<std::decay_t<Args>...>;

export
class logical_device : public exclusive_handle<VkDevice>{
	std::vector<VkQueue> graphicQueues{};
	std::vector<VkQueue> computeQueues{};
	VkQueue presentQueue{};

public:
	[[nodiscard]] logical_device() = default;

	~logical_device(){
		vkDestroyDevice(handle, nullptr);
	}

	[[nodiscard]] VkQueue graphic_queue(const std::uint32_t index) const{
		return graphicQueues[std::min(static_cast<const std::uint32_t>(graphicQueues.size() - 1), index)];
	}

	[[nodiscard]] VkQueue compute_queue(const std::uint32_t index) const{
		return computeQueues[std::min(static_cast<const std::uint32_t>(computeQueues.size() - 1), index)];
	}

	[[nodiscard]] VkQueue primary_graphics_queue() const noexcept{ return graphicQueues.front(); }

	[[nodiscard]] VkQueue present_queue() const noexcept{ return presentQueue; }

	[[nodiscard]] VkQueue primary_compute_queue() const noexcept{ return computeQueues.front(); }


	logical_device(const logical_device& other) = delete;
	logical_device(logical_device&& other) noexcept = default;
	logical_device& operator=(const logical_device& other) = delete;
	logical_device& operator=(logical_device&& other) noexcept = default;


	template <typename ...Ts>
	logical_device(
		VkPhysicalDevice physicalDevice,
		const queue_family_indices& indices,
		std::span<const char* const> device_extensions,
		const VkPhysicalDeviceFeatures& features,
		const extension_chain<Ts...>& extension_chain
	){
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
		std::vector<std::vector<float>> queueCreatePriorityInfos{};

		const std::unordered_set uniqueQueueFamilies{indices.graphic, indices.compute, indices.present};

		for(const auto [index, count] : uniqueQueueFamilies){
			auto& info = queueCreateInfos.emplace_back();
			auto& priority = queueCreatePriorityInfos.emplace_back();

			priority = std::views::iota(0u, count) | std::views::reverse | std::views::transform(
				[count](auto i) -> float{
					return static_cast<float>(i + 1) / static_cast<float>(count);
				}) | std::ranges::to<std::vector>();

			info = VkDeviceQueueCreateInfo{
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.queueFamilyIndex = index,
					.queueCount = count,
					.pQueuePriorities = priority.data()
				};
		}

		VkPhysicalDeviceFeatures2 deviceFeatures2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
		deviceFeatures2.pNext = extension_chain.get_first();
		deviceFeatures2.features = features;


		VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
		createInfo.pNext = &deviceFeatures2;

		createInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.enabledExtensionCount = static_cast<std::uint32_t>(device_extensions.size());
		createInfo.ppEnabledExtensionNames = device_extensions.data();

		if(enable_validation_layers){
			createInfo.enabledLayerCount = static_cast<std::uint32_t>(used_validation_layers.size());
			createInfo.ppEnabledLayerNames = used_validation_layers.data();
		} else{
			createInfo.enabledLayerCount = 0;
		}

		if(auto rst = vkCreateDevice(physicalDevice, &createInfo, nullptr, &handle); rst != VK_SUCCESS){
			std::println(std::cerr, "[Vulkan] Failed To Create Device: {}", static_cast<int>(rst));
			throw vk_error(rst, "Failed to create logical device!");
		} else{
			std::println("[Vulkan] Device Created: {:#0X}", reinterpret_cast<std::uintptr_t>(handle));
		}

		indices.graphic.create_queues(handle, graphicQueues);
		indices.compute.create_queues(handle, computeQueues);
		vkGetDeviceQueue(handle, indices.present.index, 0, &presentQueue);
	}
};
}
