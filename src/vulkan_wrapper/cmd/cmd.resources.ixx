module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.cmd:resources;

import std;
import mo_yanxi.vk.util;
import mo_yanxi.meta_programming;

namespace mo_yanxi::vk::cmd{
	export
	template <typename T>
		requires requires(T& t){
			t.srcAccessMask;
			t.dstAccessMask;
			t.srcStageMask;
			t.dstStageMask;

			std::swap(t.srcAccessMask, t.dstAccessMask);
			std::swap(t.srcStageMask, t.dstStageMask);
		}
	void swap_stage(T& t){
		std::swap(t.srcAccessMask, t.dstAccessMask);
		std::swap(t.srcStageMask, t.dstStageMask);
	}

	export
	template <>
	void swap_stage(VkImageMemoryBarrier2& t){
		std::swap(t.srcAccessMask, t.dstAccessMask);
		std::swap(t.srcStageMask, t.dstStageMask);
		std::swap(t.srcQueueFamilyIndex, t.dstQueueFamilyIndex);
		std::swap(t.oldLayout, t.newLayout);
	}

	export
	template <>
	void swap_stage(VkBufferMemoryBarrier2& t){
		std::swap(t.srcAccessMask, t.dstAccessMask);
		std::swap(t.srcStageMask, t.dstStageMask);
		std::swap(t.srcQueueFamilyIndex, t.dstQueueFamilyIndex);
	}


	export
	template <std::ranges::range Rng>
	void swap_stage(Rng&& arr){
		for(auto& t : arr){
			cmd::swap_stage(t);
		}
	}


	export
	template <contigious_range_of<VkImageBlit> Rng = std::initializer_list<VkImageBlit>>
	void image_blit(VkCommandBuffer command_buffer,
	                VkImage src, VkImageLayout srcExpectedLayout,
	                VkImage dst, VkImageLayout dstExpectedLayout,
	                VkFilter filter_type,
	                const Rng& rng
	){
		// VkBlitImageInfo
		::vkCmdBlitImage(command_buffer, src, srcExpectedLayout, dst, dstExpectedLayout,
		                 static_cast<std::uint32_t>(std::ranges::size(rng)),
		                 std::ranges::cdata(rng),
		                 filter_type
		);
	}

	export
	template <contigious_range_of<VkImageSubresourceRange> Rng = std::initializer_list<VkImageSubresourceRange>>
	void clear_color_image(VkCommandBuffer command_buffer,
	                       VkImage image, VkImageLayout expectedLayout,
	                       const VkClearColorValue& clearValue,
	                       const Rng& rng
	){
		::vkCmdClearColorImage(command_buffer, image, expectedLayout,
		                       &clearValue,
		                       static_cast<std::uint32_t>(std::ranges::size(rng)),
		                       std::ranges::cdata(rng)
		);
	}

	export
	template <contigious_range_of<VkImageCopy> Rng = std::initializer_list<VkImageCopy>>
	void copy_image_to_image(VkCommandBuffer command_buffer,
	                         VkImage src, VkImageLayout srcExpectedLayout,
	                         VkImage dst, VkImageLayout dstExpectedLayout,
	                         const Rng& rng
	){
		::vkCmdCopyImage(command_buffer, src, srcExpectedLayout, dst, dstExpectedLayout,
		                 static_cast<std::uint32_t>(std::ranges::size(rng)),
		                 std::ranges::cdata(rng)
		);
	}

	export
	template <contigious_range_of<VkBufferImageCopy> Rng>
	void copy_buffer_to_image(
		VkCommandBuffer command_buffer, VkBuffer src, VkImage dst, VkImageLayout expectedLayout, const Rng& rng){
		::vkCmdCopyBufferToImage(command_buffer, src, dst, expectedLayout,
		                         static_cast<std::uint32_t>(std::ranges::size(rng)),
		                         std::ranges::cdata(rng)
		);
	}

	export
	template <contigious_range_of<VkBufferImageCopy> Rng>
	void copy_image_to_buffer(
		VkCommandBuffer command_buffer, VkImage src, VkBuffer dst, VkImageLayout expectedLayout, const Rng& rng){
		::vkCmdCopyImageToBuffer(command_buffer, src, expectedLayout, dst,
		                         static_cast<std::uint32_t>(std::ranges::size(rng)),
		                         std::ranges::cdata(rng)
		);
	}

	export
	void copy_buffer_to_image(
		VkCommandBuffer command_buffer, VkBuffer src, VkImage dst, VkImageLayout expectedLayout,
		std::initializer_list<VkBufferImageCopy> rng){
		::vkCmdCopyBufferToImage(command_buffer, src, dst, expectedLayout,
		                         static_cast<std::uint32_t>(std::ranges::size(rng)),
		                         std::ranges::cdata(rng)
		);
	}

	export
	void copy_image_to_buffer(
		VkCommandBuffer command_buffer, VkImage src, VkBuffer dst, VkImageLayout expectedLayout,
		std::initializer_list<VkBufferImageCopy>& rng){
		::vkCmdCopyImageToBuffer(command_buffer, src, expectedLayout, dst,
		                         static_cast<std::uint32_t>(std::ranges::size(rng)),
		                         std::ranges::cdata(rng)
		);
	}

	export
	template <contigious_range_of<VkBufferCopy> Rng = std::initializer_list<VkBufferCopy>>
	void copy_buffer(VkCommandBuffer command_buffer, VkBuffer src, VkBuffer dst, Rng&& rng) noexcept{
		::vkCmdCopyBuffer(
			command_buffer, src, dst,
			static_cast<std::uint32_t>(std::ranges::size(rng)),
			std::ranges::cdata(rng));
	}

	export
	[[nodiscard]] VkImageLayout deduce_image_dst_layout(
		VkPipelineStageFlags2 stage,
		VkAccessFlags2 access){

		switch(stage){
		case VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT:{
			switch(access){
			case VK_ACCESS_2_SHADER_READ_BIT:
				return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			default:
				return VK_IMAGE_LAYOUT_GENERAL;
			}
			break;
		}
		case VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT:{
			switch(access){
				case VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT:
					return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				default: throw vk_error{VK_ERROR_FEATURE_NOT_PRESENT, "Deduce Failed"};
			}
		}
		default: throw vk_error{VK_ERROR_FEATURE_NOT_PRESENT, "Deduce Failed"};

		}
		std::unreachable();
	}

	export
	struct dependency_gen{
		std::vector<VkMemoryBarrier2> memory_barriers;
		std::vector<VkBufferMemoryBarrier2> buffer_memory_barriers;
		std::vector<VkImageMemoryBarrier2> image_memory_barriers;

		void swap_stages(){
			cmd::swap_stage(memory_barriers);
			cmd::swap_stage(buffer_memory_barriers);
			cmd::swap_stage(image_memory_barriers);
		}

		void clear() noexcept{
			memory_barriers.clear();
			buffer_memory_barriers.clear();
			image_memory_barriers.clear();
		}

		bool empty() const noexcept{
			return memory_barriers.empty() &&
			buffer_memory_barriers.empty() &&
			image_memory_barriers.empty();
		}

		[[nodiscard]] VkDependencyInfo create() const noexcept{
			return {
					.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
					.pNext = nullptr,
					.dependencyFlags = 0,
					.memoryBarrierCount = static_cast<std::uint32_t>(memory_barriers.size()),
					.pMemoryBarriers = memory_barriers.empty() ? nullptr : memory_barriers.data(),
					.bufferMemoryBarrierCount = static_cast<std::uint32_t>(buffer_memory_barriers.size()),
					.pBufferMemoryBarriers = buffer_memory_barriers.empty()
						                         ? nullptr
						                         : buffer_memory_barriers.data(),
					.imageMemoryBarrierCount = static_cast<std::uint32_t>(image_memory_barriers.size()),
					.pImageMemoryBarriers = image_memory_barriers.empty() ? nullptr : image_memory_barriers.data()
				};
		}
		//
		// void merge(){
		// 	auto itr = image_memory_barriers.begin();
		// 	auto end = image_memory_barriers.end();
		//
		// 	while(itr != end){
		// 		auto next = std::ranges::next(itr);
		// 		while(next != end){
		// 			if(next->image != itr->image)continue;
		// 			assert(next->newLayout == itr->newLayout);
		// 			assert(next->oldLayout == itr->oldLayout);
		//
		//
		// 			itr->srcStageMask |= next->srcStageMask;
		// 			itr->srcAccessMask |= next->srcAccessMask;
		//
		// 			itr->dstStageMask |= next->dstStageMask;
		// 			itr->dstAccessMask |= next->dstAccessMask;
		// 		}
		// 	}
		// }

		void apply(VkCommandBuffer command_buffer, bool reserve = false) noexcept{
			const auto d = create();
			vkCmdPipelineBarrier2(command_buffer, &d);
			if(!reserve){
				clear();
			}
		}

		void push_staging(
			const VkBuffer staging_buffer,
			const std::uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const std::uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const VkDeviceSize offset = 0,
			const VkDeviceSize size = VK_WHOLE_SIZE){
			push(staging_buffer,
			     VK_PIPELINE_STAGE_2_HOST_BIT,
			     VK_ACCESS_2_HOST_WRITE_BIT,
			     VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			     VK_ACCESS_2_TRANSFER_READ_BIT,
			     srcQueueFamilyIndex,
			     dstQueueFamilyIndex,
			     offset,
			     size
			);
		}

		void push_on_initialization(
			const VkBuffer buffer_first_init,
			const std::uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const std::uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const VkDeviceSize offset = 0,
			const VkDeviceSize size = VK_WHOLE_SIZE){
			push(buffer_first_init,
			     VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
			     VK_ACCESS_2_NONE,
			     VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			     VK_ACCESS_2_TRANSFER_WRITE_BIT,
			     srcQueueFamilyIndex,
			     dstQueueFamilyIndex,
			     offset,
			     size
			);
		}

		void push_post_write(
			const VkBuffer buffer_after_write,
			const VkPipelineStageFlags2 dstStageMask,
			const VkAccessFlags2 dstAccessMask,
			const std::uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const std::uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const VkDeviceSize offset = 0,
			const VkDeviceSize size = VK_WHOLE_SIZE){
			push(buffer_after_write,
			     VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			     VK_ACCESS_2_TRANSFER_WRITE_BIT,
			     dstStageMask,
			     dstAccessMask,
			     srcQueueFamilyIndex,
			     dstQueueFamilyIndex,
			     offset,
			     size
			);
		}

		void push_post_write(
			const VkImage buffer_after_write,
			const VkPipelineStageFlags2 dstStageMask,
			const VkAccessFlags2 dstAccessMask,
			const VkImageLayout newLayout,
			const VkImageSubresourceRange& range,
			const std::uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const std::uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED){
			push(buffer_after_write,
			     VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			     VK_ACCESS_2_TRANSFER_WRITE_BIT,
			     dstStageMask,
			     dstAccessMask,
			     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			     newLayout,
			     range,
			     srcQueueFamilyIndex,
			     dstQueueFamilyIndex
			);
		}

		VkBufferMemoryBarrier2& push(
			const VkBuffer buffer,
			const VkPipelineStageFlags2 srcStageMask,
			const VkAccessFlags2 srcAccessMask,
			const VkPipelineStageFlags2 dstStageMask,
			const VkAccessFlags2 dstAccessMask,
			const std::uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const std::uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const VkDeviceSize offset = 0,
			const VkDeviceSize size = VK_WHOLE_SIZE
		){
			return buffer_memory_barriers.emplace_back(VkBufferMemoryBarrier2{
					.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
					.pNext = nullptr,
					.srcStageMask = srcStageMask,
					.srcAccessMask = srcAccessMask,
					.dstStageMask = dstStageMask,
					.dstAccessMask = dstAccessMask,
					.srcQueueFamilyIndex = srcQueueFamilyIndex,
					.dstQueueFamilyIndex = dstQueueFamilyIndex,
					.buffer = buffer,
					.offset = offset,
					.size = size
				});
		}

		void push_graphic_to_compute(
			const VkImage image,
			const std::uint32_t srcQueueFamilyIndex,
			const std::uint32_t dstQueueFamilyIndex,
			const VkAccessFlags2 dstAccess = VK_ACCESS_2_SHADER_READ_BIT,
			const VkImageLayout srcLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			const VkImageSubresourceRange& range = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}

		){
			VkImageLayout dstLayout = deduce_image_dst_layout(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, dstAccess);

			image_memory_barriers.push_back({
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
					.pNext = nullptr,
					.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
					.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					.dstAccessMask = dstAccess,
					.oldLayout = srcLayout,
					.newLayout = dstLayout,
					.srcQueueFamilyIndex = srcQueueFamilyIndex,
					.dstQueueFamilyIndex = dstQueueFamilyIndex,
					.image = image,
					.subresourceRange = range
				});
		}

		void push_compute_to_graphic(
			const VkImage image,
			const std::uint32_t srcQueueFamilyIndex,
			const std::uint32_t dstQueueFamilyIndex,
			const VkAccessFlags2 srcAccess = VK_ACCESS_2_SHADER_READ_BIT,
			const VkImageLayout dstLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			const VkImageSubresourceRange& range = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}

		){
			VkImageLayout srcLayout = deduce_image_dst_layout(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, srcAccess);

			image_memory_barriers.push_back({
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
					.pNext = nullptr,
					.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					.srcAccessMask = srcAccess,
					.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
					.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
					.oldLayout = dstLayout,
					.newLayout = srcLayout,
					.srcQueueFamilyIndex = srcQueueFamilyIndex,
					.dstQueueFamilyIndex = dstQueueFamilyIndex,
					.image = image,
					.subresourceRange = range
				});
		}

		void push(
			const VkImage image,
			const VkPipelineStageFlags2 srcStageMask,
			const VkAccessFlags2 srcAccessMask,
			const VkPipelineStageFlags2 dstStageMask,
			const VkAccessFlags2 dstAccessMask,
			const VkImageLayout oldLayout,
			const VkImageLayout newLayout,
			const VkImageSubresourceRange& range,
			const std::uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const std::uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
		){
			image_memory_barriers.push_back({
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
					.pNext = nullptr,
					.srcStageMask = srcStageMask,
					.srcAccessMask = srcAccessMask,
					.dstStageMask = dstStageMask,
					.dstAccessMask = dstAccessMask,
					.oldLayout = oldLayout,
					.newLayout = newLayout,
					.srcQueueFamilyIndex = srcQueueFamilyIndex,
					.dstQueueFamilyIndex = dstQueueFamilyIndex,
					.image = image,
					.subresourceRange = range
				});
		}
	};

	export
	void memory_barrier(
		const VkCommandBuffer command_buffer,
		const VkBuffer buffer,
		const VkPipelineStageFlags2 srcStageMask,
		const VkAccessFlags2 srcAccessMask,
		const VkPipelineStageFlags2 dstStageMask,
		const VkAccessFlags2 dstAccessMask,
		const std::uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		const std::uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		const VkDeviceSize offset = 0,
		const VkDeviceSize size = VK_WHOLE_SIZE
	){
		VkBufferMemoryBarrier2 barrier2{
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
			.pNext = nullptr,
			.srcStageMask = srcStageMask,
			.srcAccessMask = srcAccessMask,
			.dstStageMask = dstStageMask,
			.dstAccessMask = dstAccessMask,
			.srcQueueFamilyIndex = srcQueueFamilyIndex,
			.dstQueueFamilyIndex = dstQueueFamilyIndex,
			.buffer = buffer,
			.offset = offset,
			.size = size
		};

		VkDependencyInfo dependency{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.bufferMemoryBarrierCount = 1,
			.pBufferMemoryBarriers = &barrier2,
		};

		vkCmdPipelineBarrier2(command_buffer, &dependency);
	}

	export
	void memory_barrier(
		const VkCommandBuffer command_buffer,
		const VkImage image,
		const VkPipelineStageFlags2 srcStageMask,
		const VkAccessFlags2 srcAccessMask,
		const VkPipelineStageFlags2 dstStageMask,
		const VkAccessFlags2 dstAccessMask,
		const VkImageLayout oldLayout,
		const VkImageLayout newLayout,
		const VkImageSubresourceRange& range = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
		const std::uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		const std::uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
	){
		VkImageMemoryBarrier2 barrier2{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.pNext = nullptr,
				.srcStageMask = srcStageMask,
				.srcAccessMask = srcAccessMask,
				.dstStageMask = dstStageMask,
				.dstAccessMask = dstAccessMask,
				.oldLayout = oldLayout,
				.newLayout = newLayout,
				.srcQueueFamilyIndex = srcQueueFamilyIndex,
				.dstQueueFamilyIndex = dstQueueFamilyIndex,
				.image = image,
				.subresourceRange = range
			};

		VkDependencyInfo dependency{
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.imageMemoryBarrierCount = 1,
				.pImageMemoryBarriers = &barrier2,
			};

		vkCmdPipelineBarrier2(command_buffer, &dependency);
	}


	export
	template <std::ranges::forward_range Rng>
		requires (!std::is_const_v<std::remove_reference_t<Rng>> && std::same_as<
			std::ranges::range_value_t<Rng>, VkRect2D>)
	void generate_mipmaps(
		VkCommandBuffer commandBuffer,
		VkImage image,
		Rng&& regions,
		const std::uint32_t mipLevels,
		const std::uint32_t base_mipLevel = 1,
		const std::uint32_t baseLayer = 0,
		const std::uint32_t layerCount = 1,

		const VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		const VkAccessFlags2 access = VK_ACCESS_2_SHADER_READ_BIT,
		const VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	){
		if(mipLevels <= 1) return;

		VkImageMemoryBarrier2 barrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.pNext = nullptr,
				.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				.srcAccessMask = access,
				.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.oldLayout = layout,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = image,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = baseLayer,
					.layerCount = layerCount
				}
			};

		VkDependencyInfo dependency{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &barrier,
		};


		for(std::uint32_t i = base_mipLevel; i < mipLevels; i++){
			barrier.subresourceRange.baseMipLevel = i - 1;

			barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;


			vkCmdPipelineBarrier2(commandBuffer, &dependency);

			for(VkRect2D& srcRegion : regions){
				const VkRect2D dstRegion{
						srcRegion.offset.x / 2, srcRegion.offset.y / 2,
						srcRegion.extent.width / 2, srcRegion.extent.height / 2
					};

				if(dstRegion.extent.width * dstRegion.extent.height > 0){
					image_blit(
						commandBuffer,
						image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						VK_FILTER_LINEAR,
						{
							VkImageBlit{
								.srcSubresource = {
									.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
									.mipLevel = i - 1,
									.baseArrayLayer = baseLayer,
									.layerCount = layerCount
								},
								.srcOffsets = {
									{srcRegion.offset.x, srcRegion.offset.y, 0},
									{
										srcRegion.offset.x + static_cast<std::int32_t>(srcRegion.extent.width),
										srcRegion.offset.y + static_cast<std::int32_t>(srcRegion.extent.height), 1
									}
								},
								.dstSubresource = {
									.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
									.mipLevel = i,
									.baseArrayLayer = baseLayer,
									.layerCount = layerCount
								},
								.dstOffsets = {
									{dstRegion.offset.x, dstRegion.offset.y, 0},
									{
										dstRegion.offset.x + static_cast<std::int32_t>(dstRegion.extent.width),
										dstRegion.offset.y + static_cast<std::int32_t>(dstRegion.extent.height), 1
									}
								}
							}
						});
				}

				srcRegion = dstRegion;
			}

			barrier.dstStageMask = stage;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = layout;
			barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
			barrier.dstAccessMask = access;

			vkCmdPipelineBarrier2(commandBuffer, &dependency);

		}

		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = layout;
		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = access;

		vkCmdPipelineBarrier2(commandBuffer, &dependency);
	}

	export
	void generate_mipmaps(
		VkCommandBuffer commandBuffer,
		VkImage image,
		VkBuffer buffer,
		VkRect2D region,
		const std::uint32_t mipLevels,
		const std::uint32_t provided_levels,
		const std::uint32_t baseLayer = 0,
		const std::uint32_t layerCount = 1,

		const VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		const VkAccessFlags2 access = VK_ACCESS_2_SHADER_READ_BIT,
		const VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		const VkDeviceSize bpi = 4
	){
		if(mipLevels <= 1) return;

		cmd::memory_barrier(commandBuffer, image,
			stage,
			access,
			VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			VK_ACCESS_2_TRANSFER_WRITE_BIT,
			layout,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = mipLevels,
				.baseArrayLayer = baseLayer,
				.layerCount = layerCount,
			}
		);

		VkDeviceSize offset = 0;
		for(std::uint32_t i = 0; i < provided_levels - 1; ++i){
			cmd::copy_buffer_to_image(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {VkBufferImageCopy{
				.bufferOffset = offset,
				.bufferRowLength = 0,
				.bufferImageHeight = 0,
				.imageSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = i,
					.baseArrayLayer = baseLayer,
					.layerCount = layerCount
				},
				.imageOffset = {
					region.offset.x, region.offset.y, 0
				},
				.imageExtent = {
					region.extent.width, region.extent.height, 1
				}
			}});

			offset += region.extent.width * region.extent.height * bpi;

			region.offset.x /= 2;
			region.offset.y /= 2;
			region.extent.width /= 2;
			region.extent.height /= 2;

			if(region.extent.width == 0 || region.extent.height == 0) break;
		}

		cmd::copy_buffer_to_image(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {VkBufferImageCopy{
			.bufferOffset = offset,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = provided_levels - 1,
				.baseArrayLayer = baseLayer,
				.layerCount = layerCount
			},
			.imageOffset = {
				region.offset.x, region.offset.y, 0
			},
			.imageExtent = {
				region.extent.width, region.extent.height, 1
			}
		}});

		if(provided_levels > 1)cmd::memory_barrier(
			commandBuffer, image,
			VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			VK_ACCESS_2_TRANSFER_WRITE_BIT,
			stage,
			access,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			layout,
			{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = provided_levels - 1,
				.baseArrayLayer = baseLayer,
				.layerCount = layerCount,
			}
		);

		if(provided_levels >= mipLevels) return;

		std::array arr{region};
		generate_mipmaps(commandBuffer, image, arr, mipLevels, provided_levels, baseLayer, layerCount, stage, access, layout);
	}


	template <typename Fn>
	using clear_func_color_t = std::add_lvalue_reference_t<std::remove_pointer_t<std::tuple_element_t<
		3, typename function_traits<std::remove_pointer_t<Fn>>::args_type>>>;

	template <typename Fn>
	void clear_image(
		Fn fn,
		VkCommandBuffer commandBuffer,
		VkImage image,
		clear_func_color_t<Fn> clearValue,
		const VkImageSubresourceRange& subrange,

		const VkPipelineStageFlags2 srcStage,
		const VkPipelineStageFlags2 dstStage,
		const VkAccessFlags2 srcAccess,
		const VkAccessFlags2 dstAccess,
		const VkImageLayout expectedLayout,
		const std::uint32_t runningQueue = VK_QUEUE_FAMILY_IGNORED,
		const std::uint32_t srcQueue = VK_QUEUE_FAMILY_IGNORED,
		const std::uint32_t dstQueue = VK_QUEUE_FAMILY_IGNORED
	) noexcept{
		VkImageMemoryBarrier2 imageMemoryBarrier{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				nullptr,
				srcStage,
				srcAccess,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				VK_ACCESS_2_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				srcQueue,
				runningQueue,
				image,
				subrange
			};

		VkDependencyInfo dependency_info{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &imageMemoryBarrier
		};

		vkCmdPipelineBarrier2(commandBuffer, &dependency_info);

		fn(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &subrange);

		imageMemoryBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstStageMask = dstStage;
		imageMemoryBarrier.dstAccessMask = dstAccess;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.newLayout = expectedLayout;
		imageMemoryBarrier.srcQueueFamilyIndex = runningQueue,
		imageMemoryBarrier.dstQueueFamilyIndex = dstQueue,

		vkCmdPipelineBarrier2(commandBuffer, &dependency_info);
	}

	export
	void clear_color(
		VkCommandBuffer commandBuffer,
		VkImage image,
		const VkClearColorValue& clearValue,
		const VkImageSubresourceRange& subrange,

		const VkPipelineStageFlags2 srcStage,
		const VkPipelineStageFlags2 dstStage,
		const VkAccessFlags2 srcAccess,
		const VkAccessFlags2 dstAccess,
		const VkImageLayout expectedLayout,
		const std::uint32_t runningQueue = VK_QUEUE_FAMILY_IGNORED,
		const std::uint32_t srcQueue = VK_QUEUE_FAMILY_IGNORED,
		const std::uint32_t dstQueue = VK_QUEUE_FAMILY_IGNORED
	) noexcept{
		clear_image(vkCmdClearColorImage, commandBuffer, image, clearValue, subrange,
			srcStage, dstStage, srcAccess, dstAccess, expectedLayout, runningQueue, srcQueue, dstQueue);
	}

	export
	void clear_depth_stencil(
		VkCommandBuffer commandBuffer,
		VkImage image,
		const VkClearDepthStencilValue& clearValue,
		const VkImageSubresourceRange& subrange,

		const VkPipelineStageFlags2 srcStage,
		const VkPipelineStageFlags2 dstStage,
		const VkAccessFlags srcAccess,
		const VkAccessFlags dstAccess,
		const VkImageLayout expectedLayout,
		const std::uint32_t runningQueue = VK_QUEUE_FAMILY_IGNORED,
		const std::uint32_t srcQueue = VK_QUEUE_FAMILY_IGNORED,
		const std::uint32_t dstQueue = VK_QUEUE_FAMILY_IGNORED
	) noexcept{
		clear_image(vkCmdClearDepthStencilImage, commandBuffer, image, clearValue, subrange,
			srcStage, dstStage, srcAccess, dstAccess, expectedLayout, runningQueue, srcQueue, dstQueue);
	}

	export
	void update(
		VkCommandBuffer commandBuffer,
		VkBuffer buffer,
		VkDeviceSize offset,
		VkDeviceSize size,
		const void* data
	){
		vkCmdUpdateBuffer(commandBuffer, buffer, offset, size, data);
	}

	export
	template <typename T>
	void update(
		VkCommandBuffer commandBuffer,
		VkBuffer buffer,
		VkDeviceSize offset,
		const T& data
	){
		::vkCmdUpdateBuffer(commandBuffer, buffer, offset, sizeof(T), &data);
	}

	export
	template <std::ranges::contiguous_range T>
	void update(
		VkCommandBuffer commandBuffer,
		VkBuffer buffer,
		VkDeviceSize offset,
		const T& range
	){
		::vkCmdUpdateBuffer(commandBuffer, buffer, offset, std::ranges::size(range) * sizeof(std::ranges::range_value_t<T>), std::ranges::cdata(range));
	}
}
