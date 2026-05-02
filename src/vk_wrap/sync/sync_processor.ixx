module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.sync_processor;

import std;
// NOLINTBEGIN(*-misplaced-const)

namespace mo_yanxi::vk{
namespace sync{

constexpr inline std::uint32_t invalid = std::numeric_limits<std::uint32_t>::max();

export struct barrier_reserve_info{
	std::size_t memory_barrier_count{};
	std::size_t buffer_barrier_count{};
	std::size_t image_barrier_count{};
};

export struct execution_state{
	VkPipelineStageFlags2 stage_mask{VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT};
	VkAccessFlags2 access_mask{VK_ACCESS_2_NONE};
};

export struct image_state : execution_state{
	VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};
};

export struct buffer_state : execution_state{
};

export struct image_slot{
	std::uint32_t value{invalid};

	[[nodiscard]] explicit operator bool() const noexcept{
		return value != invalid;
	}
	[[nodiscard]] auto operator<=>(const image_slot&) const noexcept = default;
	};

export struct buffer_slot{
	std::uint32_t value{invalid};

	[[nodiscard]] explicit operator bool() const noexcept{
		return value != invalid;
	}
	[[nodiscard]] auto operator<=>(const buffer_slot&) const noexcept = default;
	};

export struct transition_options{
	bool force{};
	bool skip_read_to_read{true};
	bool skip_if_identical{true};
	};

export class sync_barrier_batch{
public:
	void reserve(const barrier_reserve_info info){
		memory_barriers_.reserve(info.memory_barrier_count);
		buffer_barriers_.reserve(info.buffer_barrier_count);
		image_barriers_.reserve(info.image_barrier_count);
	}

	void clear() noexcept{
		memory_barriers_.clear();
		buffer_barriers_.clear();
		image_barriers_.clear();
		combined_stage_mask_ = 0;
	}

	[[nodiscard]] bool empty() const noexcept{
		return memory_barriers_.empty() && buffer_barriers_.empty() && image_barriers_.empty();
	}

	[[nodiscard]] VkDependencyInfo create_info(const VkDependencyFlags flags = 0) const noexcept{
		return {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext = nullptr,
			.dependencyFlags = flags,
			.memoryBarrierCount = static_cast<std::uint32_t>(memory_barriers_.size()),
			.pMemoryBarriers = memory_barriers_.empty() ? nullptr : memory_barriers_.data(),
			.bufferMemoryBarrierCount = static_cast<std::uint32_t>(buffer_barriers_.size()),
			.pBufferMemoryBarriers = buffer_barriers_.empty() ? nullptr : buffer_barriers_.data(),
			.imageMemoryBarrierCount = static_cast<std::uint32_t>(image_barriers_.size()),
			.pImageMemoryBarriers = image_barriers_.empty() ? nullptr : image_barriers_.data(),
		};
	}

	void apply(
		const VkCommandBuffer command_buffer,
		const bool keep = false,
		const VkDependencyFlags flags = 0) noexcept{
		if(empty())return;
		auto info = create_info(flags);
		vkCmdPipelineBarrier2(command_buffer, &info);
		if(!keep){
			clear();
		}
	}

	VkMemoryBarrier2& push_execution(
		const VkPipelineStageFlags2 src_stage_mask,
		const VkAccessFlags2 src_access_mask,
		const VkPipelineStageFlags2 dst_stage_mask,
		const VkAccessFlags2 dst_access_mask){
		combined_stage_mask_ |= src_stage_mask | dst_stage_mask;
		return memory_barriers_.emplace_back(VkMemoryBarrier2{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
			.pNext = nullptr,
			.srcStageMask = src_stage_mask,
			.srcAccessMask = src_access_mask,
			.dstStageMask = dst_stage_mask,
			.dstAccessMask = dst_access_mask,
		});
	}

	VkBufferMemoryBarrier2& push_buffer(
		const VkBuffer buffer,
		const VkPipelineStageFlags2 src_stage_mask,
		const VkAccessFlags2 src_access_mask,
		const VkPipelineStageFlags2 dst_stage_mask,
		const VkAccessFlags2 dst_access_mask,
		const VkDeviceSize offset = 0,
		const VkDeviceSize size = VK_WHOLE_SIZE){
		combined_stage_mask_ |= src_stage_mask | dst_stage_mask;
		return buffer_barriers_.emplace_back(VkBufferMemoryBarrier2{
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
			.pNext = nullptr,
			.srcStageMask = src_stage_mask,
			.srcAccessMask = src_access_mask,
			.dstStageMask = dst_stage_mask,
			.dstAccessMask = dst_access_mask,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.buffer = buffer,
			.offset = offset,
			.size = size,
		});
	}

	VkImageMemoryBarrier2& push_image(
		const VkImage image,
		const VkPipelineStageFlags2 src_stage_mask,
		const VkAccessFlags2 src_access_mask,
		const VkPipelineStageFlags2 dst_stage_mask,
		const VkAccessFlags2 dst_access_mask,
		const VkImageLayout old_layout,
		const VkImageLayout new_layout,
		const VkImageSubresourceRange& subresource_range){
		combined_stage_mask_ |= src_stage_mask | dst_stage_mask;
		return image_barriers_.emplace_back(VkImageMemoryBarrier2{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.pNext = nullptr,
			.srcStageMask = src_stage_mask,
			.srcAccessMask = src_access_mask,
			.dstStageMask = dst_stage_mask,
			.dstAccessMask = dst_access_mask,
			.oldLayout = old_layout,
			.newLayout = new_layout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange = subresource_range,
		});
	}

	[[nodiscard]] const std::vector<VkMemoryBarrier2>& memory_barriers() const noexcept{
		return memory_barriers_;
	}

	[[nodiscard]] const std::vector<VkBufferMemoryBarrier2>& buffer_barriers() const noexcept{
		return buffer_barriers_;
	}

	[[nodiscard]] const std::vector<VkImageMemoryBarrier2>& image_barriers() const noexcept{
		return image_barriers_;
	}

	[[nodiscard]] VkPipelineStageFlags2 combined_stage_mask() const noexcept{
		return combined_stage_mask_;
	}

private:
	std::vector<VkMemoryBarrier2> memory_barriers_{};
	std::vector<VkBufferMemoryBarrier2> buffer_barriers_{};
	std::vector<VkImageMemoryBarrier2> image_barriers_{};
	VkPipelineStageFlags2 combined_stage_mask_{};
	};

export class sync_processor{
private:
	struct reset_event_record{
		VkEvent event{VK_NULL_HANDLE};
		VkPipelineStageFlags2 stage_mask{VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
	};

	struct event_record{
		VkEvent event{VK_NULL_HANDLE};
		VkDependencyFlags flags{0};
		sync_barrier_batch barriers{};

		void clear() noexcept{
			barriers.clear();
		}
	};

	static constexpr VkAccessFlags2 write_access_mask_ =
		VK_ACCESS_2_MEMORY_WRITE_BIT |
		VK_ACCESS_2_SHADER_WRITE_BIT |
		VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT |
		VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
		VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
		VK_ACCESS_2_TRANSFER_WRITE_BIT |
		VK_ACCESS_2_HOST_WRITE_BIT;

	[[nodiscard]] static bool has_write_access_(const VkAccessFlags2 access) noexcept{
		return (access & write_access_mask_) != 0;
	}

	[[nodiscard]] static bool same_image_state_(const image_state& lhs, const image_state& rhs) noexcept{
		return lhs.stage_mask == rhs.stage_mask && lhs.access_mask == rhs.access_mask &&
			lhs.layout == rhs.layout;
	}

	[[nodiscard]] static bool same_buffer_state_(const buffer_state& lhs, const buffer_state& rhs) noexcept{
		return lhs.stage_mask == rhs.stage_mask && lhs.access_mask == rhs.access_mask;
	}

	[[nodiscard]] static bool can_skip_image_transition_(
		const image_state& current,
		const image_state& next,
		const transition_options& options) noexcept{
		if(options.force){
			return false;
		}

		if(options.skip_if_identical && same_image_state_(current, next)){
			return true;
		}

		if(!options.skip_read_to_read){
			return false;
		}

		const bool same_layout = current.layout == next.layout;
		return same_layout &&
			!has_write_access_(current.access_mask) && !has_write_access_(next.access_mask);
	}

	[[nodiscard]] static bool can_skip_buffer_transition_(
		const buffer_state& current,
		const buffer_state& next,
		const transition_options& options) noexcept{
		if(options.force){
			return false;
		}

		if(options.skip_if_identical && same_buffer_state_(current, next)){
			return true;
		}

		if(!options.skip_read_to_read){
			return false;
		}

		return !has_write_access_(current.access_mask) && !has_write_access_(next.access_mask);
	}

	[[nodiscard]] event_record& acquire_wait_record_(
		const VkEvent event,
		const VkDependencyFlags flags,
		const barrier_reserve_info reserve_hint){
		if(wait_event_count_ == wait_events_.size()){
			wait_events_.emplace_back();
		}
		auto& record = wait_events_[wait_event_count_++];
		record.event = event;
		record.flags = flags;
		record.clear();
		record.barriers.reserve(reserve_hint);
		return record;
	}

	[[nodiscard]] event_record& acquire_signal_record_(
		const VkEvent event,
		const VkDependencyFlags flags,
		const barrier_reserve_info reserve_hint){
		if(signal_event_count_ == signal_events_.size()){
			signal_events_.emplace_back();
		}
		auto& record = signal_events_[signal_event_count_++];
		record.event = event;
		record.flags = flags;
		record.clear();
		record.barriers.reserve(reserve_hint);
		return record;
	}

	[[nodiscard]] static const VkImageSubresourceRange& default_image_subresource_range_() noexcept{
		static constexpr VkImageSubresourceRange range{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		};
		return range;
	}

public:
	void reserve_resources(const std::size_t image_count, const std::size_t buffer_count){
		tracked_images_.reserve(image_count);
		tracked_buffers_.reserve(buffer_count);
	}

	void reserve_immediate(const barrier_reserve_info info){
		immediate_barriers_.reserve(info);
	}

	void reserve_wait_events(
		const std::size_t count,
		const barrier_reserve_info per_event_reserve = {}){
		if(wait_events_.size() < count){
			wait_events_.resize(count);
		}
		wait_event_handles_cache_.reserve(count);
		wait_dependency_infos_cache_.reserve(count);
		for(std::size_t i = 0; i < count; ++i){
			wait_events_[i].barriers.reserve(per_event_reserve);
		}
	}

	void reserve_signal_events(
		const std::size_t count,
		const barrier_reserve_info per_event_reserve = {}){
		if(signal_events_.size() < count){
			signal_events_.resize(count);
		}
		for(std::size_t i = 0; i < count; ++i){
			signal_events_[i].barriers.reserve(per_event_reserve);
		}
	}

	void reserve_reset_events(const std::size_t count){
		reset_events_.reserve(count);
	}

	[[nodiscard]] image_slot register_image(const image_state initial_state = {}){
		tracked_images_.push_back(initial_state);
		return image_slot{.value = static_cast<std::uint32_t>(tracked_images_.size() - 1)};
	}

	[[nodiscard]] buffer_slot register_buffer(const buffer_state initial_state = {}){
		tracked_buffers_.push_back(initial_state);
		return buffer_slot{.value = static_cast<std::uint32_t>(tracked_buffers_.size() - 1)};
	}

	void clear_resources() noexcept{
		tracked_images_.clear();
		tracked_buffers_.clear();
	}

	void reset_transient() noexcept{
		immediate_barriers_.clear();
		reset_event_count_ = 0;
		wait_event_count_ = 0;
		signal_event_count_ = 0;
	}

	void reset_all_states(
		const image_state image_initial_state = {},
		const buffer_state buffer_initial_state = {}) noexcept{
		std::ranges::fill(tracked_images_, image_initial_state);
		std::ranges::fill(tracked_buffers_, buffer_initial_state);
	}

	void set_image_state(const image_slot slot, const image_state state){
		tracked_images_[slot.value] = state;
	}

	void set_buffer_state(const buffer_slot slot, const buffer_state state){
		tracked_buffers_[slot.value] = state;
	}

	[[nodiscard]] image_state get_image_state(const image_slot slot) const{
		return tracked_images_[slot.value];
	}

	[[nodiscard]] buffer_state get_buffer_state(const buffer_slot slot) const{
		return tracked_buffers_[slot.value];
	}

	[[nodiscard]] sync_barrier_batch& immediate_barriers() noexcept{
		return immediate_barriers_;
	}

	bool transition_image(
		sync_barrier_batch& batch,
		const image_slot slot,
		const VkImage image,
		const image_state next_state,
		const VkImageSubresourceRange& subresource_range = default_image_subresource_range_(),
		const transition_options options = {}){
		auto& current = tracked_images_[slot.value];
		if(can_skip_image_transition_(current, next_state, options)){
			current = next_state;
			return false;
		}

		if(current.layout == VK_IMAGE_LAYOUT_UNDEFINED
			&& next_state.access_mask != VK_ACCESS_2_NONE
			&& !has_write_access_(next_state.access_mask)){
			batch.push_image(
				image,
				current.stage_mask,
				current.access_mask,
				next_state.stage_mask,
				VK_ACCESS_2_NONE,
				current.layout,
				VK_IMAGE_LAYOUT_GENERAL,
				subresource_range
			);
			current.stage_mask = next_state.stage_mask;
			current.access_mask = VK_ACCESS_2_NONE;
			current.layout = VK_IMAGE_LAYOUT_GENERAL;
		}

		batch.push_image(
			image,
			current.stage_mask,
			current.access_mask,
			next_state.stage_mask,
			next_state.access_mask,
			current.layout,
			next_state.layout,
			subresource_range
		);
		current = next_state;
		return true;
	}

	bool transition_image(
		const image_slot slot,
		const VkImage image,
		const image_state next_state,
		const VkImageSubresourceRange& subresource_range = default_image_subresource_range_(),
		const transition_options options = {}){
		return transition_image(immediate_barriers_, slot, image, next_state, subresource_range, options);
	}

	bool transition_buffer(
		sync_barrier_batch& batch,
		const buffer_slot slot,
		const VkBuffer buffer,
		const buffer_state next_state,
		const VkDeviceSize offset = 0,
		const VkDeviceSize size = VK_WHOLE_SIZE,
		const transition_options options = {}){
		auto& current = tracked_buffers_[slot.value];
		if(can_skip_buffer_transition_(current, next_state, options)){
			current = next_state;
			return false;
		}

		batch.push_buffer(
			buffer,
			current.stage_mask,
			current.access_mask,
			next_state.stage_mask,
			next_state.access_mask,
			offset,
			size
		);
		current = next_state;
		return true;
	}

	bool transition_buffer(
		const buffer_slot slot,
		const VkBuffer buffer,
		const buffer_state next_state,
		const VkDeviceSize offset = 0,
		const VkDeviceSize size = VK_WHOLE_SIZE,
		const transition_options options = {}){
		return transition_buffer(immediate_barriers_, slot, buffer, next_state, offset, size, options);
	}

	sync_barrier_batch& add_wait_event(
		const VkEvent event,
		const VkDependencyFlags flags = VK_DEPENDENCY_ASYMMETRIC_EVENT_BIT_KHR,
		const barrier_reserve_info reserve_hint = {}){
		return acquire_wait_record_(event, flags, reserve_hint).barriers;
	}

	sync_barrier_batch& add_signal_event(
		const VkEvent event,
		const VkDependencyFlags flags = VK_DEPENDENCY_ASYMMETRIC_EVENT_BIT_KHR,
		const barrier_reserve_info reserve_hint = {}){
		return acquire_signal_record_(event, flags, reserve_hint).barriers;
	}

	void add_signal_event_execution(
		const VkEvent event,
		const VkPipelineStageFlags2 src_stage_mask,
		const VkAccessFlags2 src_access_mask = VK_ACCESS_2_NONE,
		const VkDependencyFlags flags = VK_DEPENDENCY_ASYMMETRIC_EVENT_BIT_KHR){
		auto& batch = add_signal_event(event, flags, {.memory_barrier_count = 1});
		batch.push_execution(src_stage_mask, src_access_mask, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE);
	}

	void add_reset_event(
		const VkEvent event,
		const VkPipelineStageFlags2 stage_mask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT){
		if(reset_event_count_ == reset_events_.size()){
			reset_events_.emplace_back();
		}
		reset_events_[reset_event_count_++] = {event, stage_mask};
	}

	void record_pre(const VkCommandBuffer command_buffer) noexcept{
		record_resets(command_buffer);
		record_waits(command_buffer);
		immediate_barriers_.apply(command_buffer);
	}

	void record_post(const VkCommandBuffer command_buffer) noexcept{
		record_signals(command_buffer);
	}

	void record_resets(const VkCommandBuffer command_buffer) noexcept{
		if(reset_event_count_ == 0){
			return;
		}

		VkPipelineStageFlags2 combined_stage_mask = 0;
		for(std::size_t i = 0; i < reset_event_count_; ++i){
			const auto& reset = reset_events_[i];
			combined_stage_mask |= reset.stage_mask;
			vkCmdResetEvent2(command_buffer, reset.event, reset.stage_mask);
		}

		VkPipelineStageFlags2 wait_stage_mask = 0;
		for(std::size_t i = 0; i < wait_event_count_; ++i){
			wait_stage_mask |= wait_events_[i].barriers.combined_stage_mask();
		}

		const VkMemoryBarrier2 barrier{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
			.pNext = nullptr,
			.srcStageMask = combined_stage_mask,
			.srcAccessMask = VK_ACCESS_2_NONE,
			.dstStageMask = combined_stage_mask | wait_stage_mask,
			.dstAccessMask = VK_ACCESS_2_NONE,
		};
		const VkDependencyInfo info{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext = nullptr,
			.dependencyFlags = 0,
			.memoryBarrierCount = 1,
			.pMemoryBarriers = &barrier,
			.bufferMemoryBarrierCount = 0,
			.pBufferMemoryBarriers = nullptr,
			.imageMemoryBarrierCount = 0,
			.pImageMemoryBarriers = nullptr,
		};
		vkCmdPipelineBarrier2(command_buffer, &info);
	}

	void record_waits(const VkCommandBuffer command_buffer) noexcept{
		if(wait_event_count_ == 0){
			return;
		}

		wait_event_handles_cache_.clear();
		wait_dependency_infos_cache_.clear();
		wait_event_handles_cache_.reserve(wait_event_count_);
		wait_dependency_infos_cache_.reserve(wait_event_count_);

		for(std::size_t i = 0; i < wait_event_count_; ++i){
			const auto& wait = wait_events_[i];
			wait_event_handles_cache_.push_back(wait.event);
			wait_dependency_infos_cache_.push_back(wait.barriers.create_info(wait.flags));
		}

		vkCmdWaitEvents2(
			command_buffer,
			static_cast<std::uint32_t>(wait_event_count_),
			wait_event_handles_cache_.data(),
			wait_dependency_infos_cache_.data()
		);
	}

	void record_signals(const VkCommandBuffer command_buffer) noexcept{
		for(std::size_t i = 0; i < signal_event_count_; ++i){
			const auto& signal = signal_events_[i];
			auto info = signal.barriers.create_info(signal.flags);
			vkCmdSetEvent2(command_buffer, signal.event, &info);
		}
	}

private:
	std::vector<image_state> tracked_images_{};
	std::vector<buffer_state> tracked_buffers_{};

	sync_barrier_batch immediate_barriers_{};

	std::vector<reset_event_record> reset_events_{};
	std::size_t reset_event_count_{};

	std::vector<event_record> wait_events_{};
	std::size_t wait_event_count_{};
	std::vector<VkEvent> wait_event_handles_cache_{};
	std::vector<VkDependencyInfo> wait_dependency_infos_cache_{};

	std::vector<event_record> signal_events_{};
	std::size_t signal_event_count_{};
	};
} // namespace sync
} // namespace mo_yanxi::vk
// NOLINTEND(*-misplaced-const)
