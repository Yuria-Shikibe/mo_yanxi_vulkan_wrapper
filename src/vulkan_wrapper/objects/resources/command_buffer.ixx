module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk:command_buffer;

import mo_yanxi.handle_wrapper;
import mo_yanxi.vk.util;
import std;

namespace mo_yanxi::vk{
namespace cmd{
export void begin(
	VkCommandBuffer buffer,
	const VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	const VkCommandBufferInheritanceInfo& inheritance = {}){
	VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

	beginInfo.flags = flags;
	beginInfo.pInheritanceInfo = &inheritance; // Optional

	if(const auto rst = vkBeginCommandBuffer(buffer, &beginInfo)){
		throw vk_error(rst, "Failed to begin recording command buffer!");
	}
}

export void end(VkCommandBuffer buffer){
	if(const auto rst = vkEndCommandBuffer(buffer)){
		throw vk_error(rst, "Failed to record command buffer!");
	}
}
}


export
struct command_pool;

export
struct command_buffer : exclusive_handle<VkCommandBuffer>{
protected:
	exclusive_handle_member<VkDevice> device{};
	exclusive_handle_member<VkCommandPool> pool{};

public:
	[[nodiscard]] command_buffer() = default;

	command_buffer(const command_pool& command_pool, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	command_buffer(
		VkDevice device,
		VkCommandPool commandPool,
		const VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
	) : device{device}, pool{commandPool}{
		const VkCommandBufferAllocateInfo allocInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.pNext = nullptr,
				.commandPool = commandPool,
				.level = level,
				.commandBufferCount = 1
			};

		if(const auto rst = vkAllocateCommandBuffers(device, &allocInfo, &handle)){
			throw vk_error(rst, "Failed to allocate command buffers!");
		}
	}

	[[nodiscard]] VkDevice get_device() const noexcept{
		return device;
	}

	[[nodiscard]] VkCommandPool get_pool() const noexcept{
		return pool;
	}

	~command_buffer(){
		if(device && pool && handle) vkFreeCommandBuffers(device, pool, 1, &handle);
	}

	command_buffer(const command_buffer& other) = delete;

	command_buffer& operator=(const command_buffer& other) = delete;

	command_buffer(command_buffer&& other) noexcept = default;
	// command_buffer& operator=(command_buffer&& other) noexcept = default;
	command_buffer& operator=(command_buffer&& other) noexcept{
		if(this == &other) return *this;
		if(device && pool && handle) vkFreeCommandBuffers(device, pool, 1, &handle);

		handle = nullptr;
		pool = nullptr;
		device = nullptr;
		wrapper::operator=(std::move(other));
		device = std::move(other.device);
		pool = std::move(other.pool);
		return *this;
	}

	[[nodiscard]] VkCommandBufferSubmitInfo submit_info() const noexcept{
		return {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
				.pNext = nullptr,
				.commandBuffer = handle,
				.deviceMask = 0
			};
	}

	void begin(
		const VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		const VkCommandBufferInheritanceInfo& inheritance = {}) const{
		cmd::begin(get(), flags, inheritance);
	}

	void end() const{
		cmd::end(get());
	}

	template <typename T, typename... Args>
		requires std::invocable<T, VkCommandBuffer, const Args&...>
	void push(T fn, const Args&... args) const{
		std::invoke(fn, handle, args...);
	}

	[[deprecated]] void blitDraw() const{
		vkCmdDraw(handle, 3, 1, 0, 0);
	}

	void reset(VkCommandBufferResetFlagBits flagBits = static_cast<VkCommandBufferResetFlagBits>(0)) const{
		vkResetCommandBuffer(handle, flagBits);
	}

	[[deprecated]] void setViewport(const VkViewport& viewport) const{
		vkCmdSetViewport(handle, 0, 1, &viewport);
	}

	[[deprecated]] void setScissor(const VkRect2D& scissor) const{
		vkCmdSetScissor(handle, 0, 1, &scissor);
	}
};

export
struct scoped_recorder{
private:
	VkCommandBuffer handler;

public:
	[[nodiscard]] explicit(false) scoped_recorder(VkCommandBuffer handler,
		const VkCommandBufferUsageFlags flags =
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		const VkCommandBufferInheritanceInfo& inheritance = {})
	: handler{handler}{
		cmd::begin(handler, flags, inheritance);
	}

	~scoped_recorder(){
		cmd::end(handler);
	}

	explicit(false) operator VkCommandBuffer() const noexcept{
		return handler;
	}

	[[nodiscard]] VkCommandBuffer get() const noexcept{
		return handler;
	}

	scoped_recorder(const scoped_recorder& other) = delete;
	scoped_recorder(scoped_recorder&& other) noexcept = delete;
	scoped_recorder& operator=(const scoped_recorder& other) = delete;
	scoped_recorder& operator=(scoped_recorder&& other) noexcept = delete;
};

export
struct transient_command : command_buffer{
	static constexpr VkCommandBufferBeginInfo BeginInfo{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};

private:
	VkQueue targetQueue{};
	VkFence fence{};

public:
	[[nodiscard]] transient_command() = default;

	[[nodiscard]] transient_command(command_buffer&& commandBuffer, VkQueue targetQueue, VkFence fence = nullptr) :
	command_buffer{
		std::move(commandBuffer)
	}, targetQueue{targetQueue}, fence(fence){
		vkBeginCommandBuffer(handle, &BeginInfo);
	}

	[[nodiscard]] transient_command(
		VkDevice device,
		VkCommandPool commandPool,
		VkQueue targetQueue, VkFence fence = nullptr)
	: command_buffer{device, commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY},
	targetQueue{targetQueue}, fence(fence){
		vkBeginCommandBuffer(handle, &BeginInfo);
	}

	[[nodiscard]] transient_command(
		const command_pool& command_pool,
		VkQueue targetQueue, VkFence fence = nullptr);

	~transient_command() noexcept(false){
		submit();
	}

	transient_command(const transient_command& other) = delete;

	transient_command& operator=(const transient_command& other) = delete;

	transient_command(transient_command&& other) noexcept = default;

	transient_command& operator=(transient_command&& other) noexcept{
		if(this == &other) return *this;
		std::swap(static_cast<command_buffer&>(*this), static_cast<command_buffer&>(other));
		std::swap(fence, other.fence);
		std::swap(targetQueue, other.targetQueue);
		return *this;
	}

private:
	void submit(){
		if(!handle) return;

		vkEndCommandBuffer(handle);

		const VkSubmitInfo submitInfo{
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				.pNext = nullptr,
				.waitSemaphoreCount = 0,
				.pWaitSemaphores = nullptr,
				.pWaitDstStageMask = nullptr,
				.commandBufferCount = 1,
				.pCommandBuffers = &handle,
				.signalSemaphoreCount = 0,
				.pSignalSemaphores = nullptr
			};

		vkQueueSubmit(targetQueue, 1, &submitInfo, fence);
		if(fence){
			if(const auto rst = vkWaitForFences(device, 1, &fence, true, std::numeric_limits<uint64_t>::max())){
				throw vk_error{rst, "Failed To Wait Fence On Transient Command"};
			}
			if(const auto rst = vkResetFences(device, 1, &fence)){
				throw vk_error{rst, "Failed To Reset Fence On Transient Command"};
			}
		} else{
			vkQueueWaitIdle(targetQueue);
		}
	}
};

export
template <typename Alloc>
struct command_chunk;

struct unit{
	template <typename Alloc>
	friend struct command_chunk;

	using span_type = std::span<const VkCommandBuffer>;
private:
	span_type command_buffers{};
	VkSemaphore semaphore{};
	std::uint64_t counting{};

public:
	span_type::const_iterator begin() const noexcept{
		return command_buffers.begin();
	}

	span_type::const_iterator end() const noexcept{
		return command_buffers.end();
	}

	[[nodiscard]] VkCommandBuffer operator[](const std::size_t idx) const noexcept{
		return command_buffers[idx];
	}

	[[nodiscard]] VkSemaphore get_semaphore() const noexcept{
		return semaphore;
	}

	[[nodiscard]] VkSemaphoreSubmitInfo get_next_semaphore_submit_info(VkPipelineStageFlags2 signal_stage) noexcept{
		return VkSemaphoreSubmitInfo{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = semaphore,
				.value = ++counting,
				.stageMask = signal_stage
			};
	}

	[[nodiscard]] VkCommandBufferSubmitInfo get_command_submit_info(const std::size_t index = 0) const noexcept{
		return VkCommandBufferSubmitInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
				.commandBuffer = command_buffers[index],
			};
	}

	void submit(
		VkQueue queue,
		VkPipelineStageFlags2 signal_stage,
		std::span<const VkSemaphoreSubmitInfo> waiting_semaphores = {}
	) & noexcept{
		const VkSemaphoreSubmitInfo to_signal = get_next_semaphore_submit_info(signal_stage);

		const VkCommandBufferSubmitInfo cmd_info = get_command_submit_info();

		VkSubmitInfo2 submit_info{
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
				.waitSemaphoreInfoCount = static_cast<std::uint32_t>(waiting_semaphores.size()),
				.pWaitSemaphoreInfos = waiting_semaphores.size() == 0 ? nullptr : waiting_semaphores.data(),
				.commandBufferInfoCount = 1,
				.pCommandBufferInfos = &cmd_info,
				.signalSemaphoreInfoCount = 1,
				.pSignalSemaphoreInfos = &to_signal
			};

		vkQueueSubmit2(queue, 1, &submit_info, nullptr);
	}

	void wait(VkDevice device, std::uint64_t timeout = std::numeric_limits<std::uint64_t>::max()) const noexcept{
		const VkSemaphoreWaitInfo info{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
				.pNext = nullptr,
				.flags = 0,
				.semaphoreCount = 1,
				.pSemaphores = &semaphore,
				.pValues = &counting
			};

		vkWaitSemaphores(device, &info, timeout);
	}

	[[nodiscard]] unit() = default;

	unit(const unit& other) = delete;
	unit(unit&& other) noexcept = default;
	unit& operator=(const unit& other) = delete;
	unit& operator=(unit&& other) noexcept = default;
};

template <typename Alloc = std::allocator<VkCommandBuffer>>
struct command_chunk{
private:
	VkDevice device{};
	VkCommandPool pool{};

public:
	using unit_allocator_type = std::allocator_traits<Alloc>::template rebind_alloc<unit>;
	using buffer_allocator_type = std::allocator_traits<Alloc>::template rebind_alloc<VkCommandBuffer>;

private:
	std::vector<VkCommandBuffer, buffer_allocator_type> command_buffers{};
	std::vector<unit, unit_allocator_type> units{};

public:
	using container_type = decltype(units);

	[[nodiscard]] command_chunk() = default;

	[[nodiscard]] command_chunk(const buffer_allocator_type& allocator, VkDevice device, VkCommandPool pool,
		const std::size_t chunk_count,
		const std::size_t commands_per_chunk = 1)
	: device(device)
	, pool(pool)
	, command_buffers(chunk_count * commands_per_chunk, allocator)
	, units(chunk_count, unit_allocator_type{allocator}){
		const VkCommandBufferAllocateInfo allocInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.pNext = nullptr,
				.commandPool = pool,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = static_cast<std::uint32_t>(command_buffers.size())
			};

		if(const auto rst = ::vkAllocateCommandBuffers(device, &allocInfo, command_buffers.data())){
			throw vk_error(rst, "Failed to allocate command buffers!");
		}

		static constexpr VkSemaphoreTypeCreateInfo semaphore_create_info{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
				.pNext = nullptr,
				.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
				.initialValue = 0
			};

		static constexpr VkSemaphoreCreateInfo semaphoreInfo{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
				.pNext = &semaphore_create_info,
				.flags = 0
			};

		for(auto&& [idx, value] : units | std::views::enumerate){
			value.command_buffers = std::span{command_buffers.begin() + idx * commands_per_chunk, command_buffers.begin() + idx * commands_per_chunk + commands_per_chunk};
			if(const auto rst = ::vkCreateSemaphore(device, &semaphoreInfo, nullptr, &value.semaphore)){
				throw vk_error(rst, "Failed to create semaphore!");
			}
		}
	}

	[[nodiscard]] command_chunk(VkDevice device, VkCommandPool pool,
		const std::size_t chunk_count,
		const std::size_t commands_per_chunk = 1) : command_chunk(buffer_allocator_type{}, device, pool, chunk_count, commands_per_chunk){}

private:
	void free() const noexcept{
		if(!device || !pool) return;

		::vkFreeCommandBuffers(device, pool, command_buffers.size(), command_buffers.data());

		for(const unit& value : units){
			::vkDestroySemaphore(device, value.semaphore, nullptr);
		}
	}

public:
	~command_chunk(){
		free();
	}

	command_chunk(const command_chunk& other) = delete;
	command_chunk& operator=(const command_chunk& other) = delete;

	command_chunk(command_chunk&& other) noexcept
	: device{std::exchange(other.device, {})},
	pool{std::exchange(other.pool, {})},
	command_buffers{std::exchange(other.command_buffers, {})},
	units{std::move(other.units)}{
	}

	command_chunk& operator=(command_chunk&& other) noexcept{
		if(this == &other) return *this;
		free();
		device = std::exchange(other.device, {});
		pool = std::exchange(other.pool, {});
		command_buffers = std::exchange(other.command_buffers, {});
		units = std::move(other.units);
		return *this;
	}

	[[nodiscard]] container_type::const_iterator begin() const noexcept{
		return units.begin();
	}

	[[nodiscard]] container_type::const_iterator end() const noexcept{
		return units.end();
	}

	[[nodiscard]] unit& operator[](const std::size_t index) noexcept{
		return units[index];
	}

	[[nodiscard]] VkCommandBuffer operator[](const std::size_t chunkIdx, const std::size_t localIdx) const noexcept{
		return units[chunkIdx].command_buffers[localIdx];
	}

	[[nodiscard]] container_type::size_type size() const noexcept{
		return units.size();
	}

	[[nodiscard]] VkDevice get_device() const noexcept{
		return device;
	}

	[[nodiscard]] VkCommandPool get_pool() const noexcept{
		return pool;
	}
};

export
template <typename Alloc = std::allocator<VkCommandBuffer>>
struct command_seq {
    using allocator_type = std::allocator_traits<Alloc>::template rebind_alloc<VkCommandBuffer>;
    using container_type = std::vector<VkCommandBuffer, allocator_type>;

private:
    VkDevice device{};
    VkCommandPool pool{};
    container_type buffers{};

    void free() const noexcept {
        if (!device || !pool || buffers.empty()) return;
        ::vkFreeCommandBuffers(device, pool, static_cast<std::uint32_t>(buffers.size()), buffers.data());
    }

public:
    [[nodiscard]] command_seq() = default;

    [[nodiscard]] command_seq(
        VkDevice device,
        VkCommandPool pool,
        const std::size_t count,
        const VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        const allocator_type& allocator = allocator_type{}
    ) : device(device), pool(pool), buffers(count, nullptr, allocator) {
        if (count == 0) return;

        const VkCommandBufferAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = pool,
            .level = level,
            .commandBufferCount = static_cast<std::uint32_t>(count)
        };

        if (const auto rst = ::vkAllocateCommandBuffers(device, &allocInfo, buffers.data())) {
            throw vk_error(rst, "Failed to allocate command sequence!");
        }
    }

    ~command_seq() {
        free();
    }

    command_seq(const command_seq& other) = delete;
    command_seq& operator=(const command_seq& other) = delete;

    command_seq(command_seq&& other) noexcept
        : device{std::exchange(other.device, {})},
          pool{std::exchange(other.pool, {})},
          buffers{std::move(other.buffers)} {
    }

    command_seq& operator=(command_seq&& other) noexcept {
        if (this == &other) return *this;
        free();
        device = std::exchange(other.device, {});
        pool = std::exchange(other.pool, {});
        buffers = std::move(other.buffers);
        return *this;
    }

    [[nodiscard]] container_type::const_iterator begin() const noexcept {
        return buffers.begin();
    }

    [[nodiscard]] container_type::const_iterator end() const noexcept {
        return buffers.end();
    }

    [[nodiscard]] VkCommandBuffer operator[](const std::size_t index) const noexcept {
        return buffers[index];
    }

    [[nodiscard]] VkCommandBuffer at(const std::size_t index) const {
        return buffers.at(index);
    }

    [[nodiscard]] container_type::size_type size() const noexcept {
        return buffers.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        return buffers.empty();
    }

    [[nodiscard]] const VkCommandBuffer* data() const noexcept {
        return buffers.data();
    }

    [[nodiscard]] VkDevice get_device() const noexcept {
        return device;
    }

    [[nodiscard]] VkCommandPool get_pool() const noexcept {
        return pool;
    }
};

struct command_pool : exclusive_handle<VkCommandPool>{
protected:
	exclusive_handle_member<VkDevice> device{};

public:
	[[nodiscard]] command_pool() = default;

	~command_pool(){
		if(device) vkDestroyCommandPool(device, handle, nullptr);
	}

	command_pool(const command_pool& other) = delete;
	command_pool(command_pool&& other) noexcept = default;
	command_pool& operator=(const command_pool& other) = delete;
	command_pool& operator=(command_pool&& other) = default;

	command_pool(
		VkDevice device,
		const std::uint32_t queueFamilyIndex,
		const VkCommandPoolCreateFlags flags
	) : device{device}{
		VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
		poolInfo.queueFamilyIndex = queueFamilyIndex;
		poolInfo.flags = flags; // Optional

		if(auto rst = vkCreateCommandPool(device, &poolInfo, nullptr, &handle)){
			throw vk_error(rst, "Failed to create command pool!");
		}
	}

	[[nodiscard]] VkDevice get_device() const noexcept{
		return device;
	}

	[[nodiscard]] command_buffer obtain(const VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const{
		return {device, handle, level};
	}

	[[nodiscard]] transient_command get_transient(VkQueue targetQueue, VkFence fence = nullptr) const{
		return transient_command{device, handle, targetQueue, fence};
	}

	void reset_all(const VkCommandPoolResetFlags resetFlags = 0) const{
		vkResetCommandPool(device, handle, resetFlags);
	}
};


command_buffer::command_buffer(const command_pool& command_pool, const VkCommandBufferLevel level)
: command_buffer(command_pool.get_device(), command_pool, level){
}

transient_command::transient_command(const command_pool& command_pool, VkQueue targetQueue, VkFence fence) :
transient_command(command_pool.get_device(), command_pool.get(), targetQueue, fence){
}
}
