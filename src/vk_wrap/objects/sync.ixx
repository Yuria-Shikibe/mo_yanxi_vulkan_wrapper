module;

#include <cassert>
#include <vulkan/vulkan.h>

export module mo_yanxi.vk:sync;

import std;
import mo_yanxi.handle_wrapper;
import mo_yanxi.vk.util;

namespace mo_yanxi::vk{
export
struct fence : public exclusive_handle<VkFence>{
private:
	exclusive_handle_member<VkDevice> device{};

public:
	[[nodiscard]] fence() = default;

	[[nodiscard]] fence(VkDevice device, const bool create_signal) : device{device}{
		const VkFenceCreateInfo fenceInfo{
				.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
				.pNext = nullptr,
				.flags = create_signal ? VK_FENCE_CREATE_SIGNALED_BIT : VkFenceCreateFlags{}
			};

		if(vkCreateFence(device, &fenceInfo, nullptr, &handle) != VK_SUCCESS){
			throw std::runtime_error("Failed to create fence!");
		}
	}

	[[nodiscard]] VkResult get_status() const noexcept{
		assert(handle && device);
		return vkGetFenceStatus(device, handle);
	}

	void reset() const{
		if(const auto rst = vkResetFences(device, 1, &handle)){
			throw vk_error{rst, "Failed to reset fence"};
		}
	}

	void wait(const std::uint64_t timeout = std::numeric_limits<std::uint64_t>::max()) const{
		if(const auto rst = vkWaitForFences(device, 1, &handle, true, timeout)){
			throw vk_error{rst, "Failed to wait fence"};
		}
	}

	void wait_and_reset(const std::uint64_t timeout = std::numeric_limits<std::uint64_t>::max()) const{
		wait(timeout);
		reset();
	}

	[[nodiscard]] VkDevice get_device() const noexcept{
		return device;
	}

	fence(const fence& other) = delete;
	fence(fence&& other) noexcept = default;
	fence& operator=(const fence& other) = delete;
	fence& operator=(fence&& other) noexcept = default;

	~fence(){
		if(device) vkDestroyFence(device, handle, nullptr);
	}
};

export
struct semaphore : exclusive_handle<VkSemaphore>{
private:
	exclusive_handle_member<VkDevice> device{};

public:
	[[nodiscard]] semaphore() = default;

	[[nodiscard]] explicit(false) semaphore(VkDevice device) : device{device}{
		VkSemaphoreCreateInfo semaphoreInfo{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0
			};

		if(const auto rst = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &handle)){
			throw vk_error(rst, "Failed to create Semaphore!");
		}
	}

	[[nodiscard]] explicit(false) semaphore(VkDevice device, const std::uint64_t initValue) : device{device}{
		VkSemaphoreTypeCreateInfo semaphoreInfo2{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
				.pNext = nullptr,
				.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
				.initialValue = initValue
			};

		VkSemaphoreCreateInfo semaphoreInfo{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
				.pNext = &semaphoreInfo2,
				.flags = 0
			};

		if(const auto rst = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &handle)){
			throw vk_error(rst, "Failed to create Semaphore!");
		}
	}


	semaphore(const semaphore& other) = delete;
	semaphore(semaphore&& other) noexcept = default;
	semaphore& operator=(const semaphore& other) = delete;
	semaphore& operator=(semaphore&& other) = default;

	~semaphore(){
		if(device) vkDestroySemaphore(device, handle, nullptr);
	}

	[[nodiscard]] VkDevice get_device() const noexcept{
		return device;
	}

	[[nodiscard]] VkSemaphoreSubmitInfo get_submit_info(const VkPipelineStageFlags2 flags) const{
		return {
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.pNext = nullptr,
				.semaphore = handle,
				.value = 0,
				.stageMask = flags,
				.deviceIndex = 0
			};
	}

	void signal(const std::uint64_t value = 0) const{
		const VkSemaphoreSignalInfo signalInfo{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
				.pNext = nullptr,
				.semaphore = handle,
				.value = value
			};

		if(const auto rst = vkSignalSemaphore(device, &signalInfo)){
			throw vk_error(rst, "Failed to signal semaphore value");
		}
	}

	[[nodiscard]] std::uint64_t get_counter_value() const{
		std::uint64_t value;
		if(const auto rst = vkGetSemaphoreCounterValue(device, handle, &value)){
			throw vk_error(rst, "Failed to get semaphore value");
		}
		return value;
	}

	void wait(const std::uint64_t value, const std::uint64_t timeout = std::numeric_limits<std::uint64_t>::max()) const{
		const VkSemaphoreWaitInfo info{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
				.pNext = nullptr,
				.flags = 0,
				.semaphoreCount = 1,
				.pSemaphores = &handle,
				.pValues = &value
			};

		vkWaitSemaphores(device, &info, timeout);
	}
};

export
struct event : exclusive_handle<VkEvent>{
private:
	exclusive_handle_member<VkDevice> device{};

public:
	~event(){
		if(device) vkDestroyEvent(device, handle, nullptr);
	}

	[[nodiscard]] event() = default;

	[[nodiscard]] explicit event(VkDevice device, const bool isDeviceOnly = true) : device(device){
		const VkEventCreateInfo info{
				.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
				.pNext = nullptr,
				.flags = isDeviceOnly ? VK_EVENT_CREATE_DEVICE_ONLY_BIT : 0u
			};

		if(const auto rst = vkCreateEvent(device, &info, nullptr, &handle)){
			throw vk_error(rst, "Failed to create vk::event!");
		}
	}

	void set() const{
		if(const auto rst = vkSetEvent(device, handle)){
			throw vk_error(rst, "Failed to set vk::event!");
		}
	}

	void cmd_set(VkCommandBuffer commandBuffer, const VkDependencyInfo& dependencyInfo) const noexcept{
		vkCmdSetEvent2(commandBuffer, handle, &dependencyInfo);
	}

	void cmd_reset(VkCommandBuffer commandBuffer, const VkPipelineStageFlags2 flags) const noexcept{
		vkCmdResetEvent2(commandBuffer, handle, flags);
	}

	void cmdWait(VkCommandBuffer commandBuffer, const VkDependencyInfo& dependencyInfo) const noexcept{
		vkCmdWaitEvents2(commandBuffer, 1, &handle, &dependencyInfo);
	}

	void reset() const{
		if(const auto rst = vkResetEvent(device, handle)){
			throw vk_error(rst, "Failed to reset vk::event!");
		}
	}

	[[nodiscard]] VkResult get_status() const{
		switch(auto rst = get_status_noexcept()){
		case VK_ERROR_OUT_OF_HOST_MEMORY :
		case VK_ERROR_OUT_OF_DEVICE_MEMORY :
		case VK_ERROR_DEVICE_LOST : throw vk_error(rst, "Failed to get vk::event status!");
		default : return rst;
		}
	}

	[[nodiscard]] VkResult get_status_noexcept() const noexcept{
		return vkGetEventStatus(device, handle);
	}

	[[nodiscard]] bool is_set() const{
		return get_status() == VK_EVENT_SET;
	}

	[[nodiscard]] bool is_reset() const{
		return get_status() == VK_EVENT_RESET;
	}

	[[nodiscard]] VkDevice get_device() const noexcept{
		return device;
	}

	template <typename P>
	bool wait_set(std::chrono::duration<std::chrono::microseconds::rep, P> timeout) const noexcept{
		const auto src = std::chrono::high_resolution_clock::now();
		while(!is_set()){
			if(std::chrono::high_resolution_clock::now() - src > timeout){
				return false;
			}
			std::this_thread::sleep_for(std::chrono::microseconds(10));
		}
		return true;
	}

	void wait_set() const noexcept(true){
#if MO_YANXI_VULKAN_WRAPPER_ENABLE_CHECK
		const auto src = std::chrono::high_resolution_clock::now();
#endif
		while(!is_set()){
			// std::cerr << "abc";
			std::this_thread::sleep_for(std::chrono::microseconds(10));
#if MO_YANXI_VULKAN_WRAPPER_ENABLE_CHECK
			if(std::chrono::high_resolution_clock::now() - src > std::chrono::seconds{1}){
				std::terminate();
			}
#endif
		}
	}

	event(const event& other) = delete;

	event(event&& other) noexcept = default;

	event& operator=(const event& other) = delete;

	event& operator=(event&& other) noexcept = default;
};

export
void wait_multiple(
	VkDevice device,
	const std::span<VkSemaphore> semaphores,
	const std::span<const std::uint64_t> value,
	const std::uint64_t timeout = std::numeric_limits<std::uint64_t>::max()){
	const VkSemaphoreWaitInfo info{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.pNext = nullptr,
			.flags = 0,
			.semaphoreCount = static_cast<std::uint32_t>(value.size()),
			.pSemaphores = semaphores.data(),
			.pValues = value.data()
		};

	vkWaitSemaphores(device, &info, timeout);
}


export
struct event_vector {
private:
    VkDevice device_{VK_NULL_HANDLE};
    std::vector<VkEvent> events_{};

    void cleanup() noexcept {
        if (device_ != VK_NULL_HANDLE) {
            for (const auto& event : events_) {
                if (event != VK_NULL_HANDLE) {
                    vkDestroyEvent(device_, event, nullptr);
                }
            }
        }
        events_.clear();
    }

public:
    event_vector() = default;

    explicit event_vector(VkDevice device) : device_(device) {}

    event_vector(VkDevice device, std::size_t count, bool device_only = true) : device_(device) {
        resize(count, device_only);
    }

    ~event_vector() {
        cleanup();
    }

    event_vector(const event_vector&) = delete;
    event_vector& operator=(const event_vector&) = delete;

    event_vector(event_vector&& other) noexcept
        : device_(std::exchange(other.device_, VK_NULL_HANDLE)), events_(std::exchange(other.events_, {})) {
    }

    event_vector& operator=(event_vector&& other) noexcept {
        if (this != &other) {
            cleanup(); // 1. 先销毁自己的资源

            device_ = other.device_;        // 2. 接管资源
            events_ = std::move(other.events_);

            other.device_ = VK_NULL_HANDLE; // 3. 置空源对象
        }
        return *this;
    }

    // --- 核心功能 ---

    void resize(std::size_t count, bool device_only = true) {
        if (device_ == VK_NULL_HANDLE) {
            throw std::logic_error("Attempting to resize event_vector with null device");
        }

        const auto prev = events_.size();

        if (count < prev) {
            for (std::size_t i = count; i < prev; ++i) {
                vkDestroyEvent(device_, events_[i], nullptr);
            }
            events_.resize(count);
        }
        else if (count > prev) {
            events_.resize(count, VK_NULL_HANDLE);

            for (std::size_t i = prev; i < count; ++i) {
            	VkEventCreateInfo createInfo{
                	.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
                	.flags = device_only ? VK_EVENT_CREATE_DEVICE_ONLY_BIT : VkEventCreateFlags{}
                };

                if (const auto result = vkCreateEvent(device_, &createInfo, nullptr, &events_[i])) {
                    // --- 异常处理修正 ---
                    // 销毁本次循环中已经创建成功的 Event (prev 到 i-1)
                    for (std::size_t j = prev; j < i; ++j) {
                        vkDestroyEvent(device_, events_[j], nullptr);
                    }
                    // 回退 vector 大小
                    events_.resize(prev);
                    throw vk::vk_error(result, "Failed to create vk::event during resize!");
                }
            }
        }
    }

    void clear() noexcept {
        resize(0);
    }

    [[nodiscard]] VkDevice device() const { return device_; }
    [[nodiscard]] std::size_t size() const { return events_.size(); }
    [[nodiscard]] bool empty() const { return events_.empty(); }

    [[nodiscard]] const VkEvent* data() const { return events_.data(); }
    [[nodiscard]] VkEvent* data() { return events_.data(); }

    VkEvent& operator[](std::size_t index) { return events_[index]; }
    const VkEvent& operator[](std::size_t index) const { return events_[index]; }

    using iterator = std::vector<VkEvent>::iterator;
    using const_iterator = std::vector<VkEvent>::const_iterator;

    iterator begin() { return events_.begin(); }
    iterator end() { return events_.end(); }

    const_iterator begin() const { return events_.begin(); }
    const_iterator end() const { return events_.end(); }

    const_iterator cbegin() const { return events_.cbegin(); }
    const_iterator cend() const { return events_.cend(); }
};

}
