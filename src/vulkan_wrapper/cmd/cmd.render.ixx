module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.cmd:render;

import mo_yanxi.vk.util;
import std;

namespace mo_yanxi::vk::cmd{

	export
	template <std::ranges::range Rng>
		requires (std::convertible_to<std::ranges::range_value_t<Rng>, VkImageView>)
	[[nodiscard]] std::vector<VkDescriptorImageInfo> getDescriptorInfoRange_ShaderRead(VkSampler handle, const Rng& imageViews) noexcept{
		std::vector<VkDescriptorImageInfo> rst{};

		for (const auto& view : imageViews){
			rst.emplace_back(handle, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		return rst;
	}

	export
	template <std::size_t size>
	void submit_command(VkQueue queue, const std::array<VkCommandBuffer, size>& commandBuffer, VkFence fence = nullptr,
			VkSemaphore toWait = nullptr, VkPipelineStageFlags2 waitStage = VK_PIPELINE_STAGE_2_NONE,
			VkSemaphore toSignal = nullptr, VkPipelineStageFlags2 signalStage = VK_PIPELINE_STAGE_2_NONE
			){

		VkSemaphoreSubmitInfo semaphoreSubmitInfo_Wait{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = toWait,
			.value = 0,
			.stageMask = waitStage,
			.deviceIndex = 0
		};

		VkSemaphoreSubmitInfo semaphoreSubmitInfo_Signal{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = toSignal,
			.value = 0,
			.stageMask = signalStage,
			.deviceIndex = 0
		};

		std::array<VkCommandBufferSubmitInfo, size> cmds{};
		for (const auto& [i, cmd] : cmds | std::views::enumerate){
			cmd = VkCommandBufferSubmitInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
				.commandBuffer = commandBuffer[i],
				.deviceMask = 0
			};
		}


		VkSubmitInfo2 submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.commandBufferInfoCount = static_cast<std::uint32_t>(cmds.size()),
			.pCommandBufferInfos = cmds.data(),
		};

		if(toWait){
			submitInfo.waitSemaphoreInfoCount = 1,
			submitInfo.pWaitSemaphoreInfos = &semaphoreSubmitInfo_Wait;
		}

		if(toSignal){
			submitInfo.signalSemaphoreInfoCount = 1,
			submitInfo.pSignalSemaphoreInfos = &semaphoreSubmitInfo_Signal;
		}

		if(const auto rst = vkQueueSubmit2(queue, 1, &submitInfo, fence)){
			// std::println(std::cerr, "[Vulkan] [{}] Failed to submit the command buffer!", static_cast<int>(rst));
			throw vk_error{rst, "Failed To Submit Command"};
		}
	}

	export
	void submit_command(VkQueue queue, const std::span<VkCommandBuffer> commandBuffer, VkFence fence = nullptr,
	                   VkSemaphore toWait = nullptr, const VkPipelineStageFlags2 wait_before = VK_PIPELINE_STAGE_2_NONE,
	                   VkSemaphore toSignal = nullptr, const VkPipelineStageFlags2 signal_after = VK_PIPELINE_STAGE_2_NONE
	){
		if(commandBuffer.size() == 0)return;
		VkSemaphoreSubmitInfo semaphoreSubmitInfo_Wait{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = toWait,
				.value = 0,
				.stageMask = wait_before,
				.deviceIndex = 0
			};

		VkSemaphoreSubmitInfo semaphoreSubmitInfo_Signal{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = toSignal,
				.value = 0,
				.stageMask = signal_after,
				.deviceIndex = 0
			};

		std::vector<VkCommandBufferSubmitInfo> cmds(commandBuffer.size());
		for(const auto& [i, cmd] : cmds | std::views::enumerate){
			cmd = VkCommandBufferSubmitInfo{
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
					.commandBuffer = commandBuffer[i],
					.deviceMask = 0
				};
		}


		VkSubmitInfo2 submitInfo{
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
				.commandBufferInfoCount = static_cast<std::uint32_t>(cmds.size()),
				.pCommandBufferInfos = cmds.data(),
			};

		if(toWait){
			submitInfo.waitSemaphoreInfoCount = 1,
				submitInfo.pWaitSemaphoreInfos = &semaphoreSubmitInfo_Wait;
		}

		if(toSignal){
			submitInfo.signalSemaphoreInfoCount = 1,
				submitInfo.pSignalSemaphoreInfos = &semaphoreSubmitInfo_Signal;
		}

		vkQueueSubmit2(queue, 1, &submitInfo, fence);
	}

	export
	void submit_command(VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence = nullptr,
		VkSemaphore toWait = nullptr, const VkPipelineStageFlags2 wait_before = VK_PIPELINE_STAGE_2_NONE,
		VkSemaphore toSignal = nullptr, const VkPipelineStageFlags2 signal_after = VK_PIPELINE_STAGE_2_NONE
		){

		submit_command(queue, std::array{commandBuffer}, fence, toWait, wait_before, toSignal, signal_after);
	}

	export
	template <std::size_t W, std::size_t S>
	void submit_command(VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence = nullptr,
		const std::array<VkSemaphore, W> toWait = {}, const std::array<VkPipelineStageFlags2, W> waitStage = {},
		const std::array<VkSemaphore, S> toSignal = {}, const std::array<VkPipelineStageFlags2, S> signalStage = {}
		){


		VkCommandBufferSubmitInfo cmdSubmitInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = commandBuffer,
			.deviceMask = 0
		};

		VkSubmitInfo2 submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &cmdSubmitInfo,
		};


		std::array<VkSemaphoreSubmitInfo, W> waits{};
		std::array<VkSemaphoreSubmitInfo, S> signals{};

		if constexpr (W){
			for (const auto & [i, semaphore] : toWait | std::views::enumerate){
				waits[i] = VkSemaphoreSubmitInfo{
					.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
					.semaphore = semaphore,
					.value = 0,
					.stageMask = waitStage[i],
					.deviceIndex = 0
				};
			}

			submitInfo.waitSemaphoreInfoCount = W;
			submitInfo.pWaitSemaphoreInfos = waits.data();
		}

		if constexpr (S){
			for (const auto & [i, semaphore] : toSignal | std::views::enumerate){
				signals[i] = VkSemaphoreSubmitInfo{
					.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
					.semaphore = semaphore,
					.value = 0,
					.stageMask = signalStage[i],
					.deviceIndex = 0
				};
			}

			submitInfo.signalSemaphoreInfoCount = S;
			submitInfo.pSignalSemaphoreInfos = signals.data();
		}

		vkQueueSubmit2(queue, 1, &submitInfo, fence);
	}

	export void set_scissor(VkCommandBuffer command_buffer, VkRect2D scissor){
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	}

	export void set_viewport(VkCommandBuffer command_buffer, VkRect2D region){
		VkViewport viewport{};
		viewport.x = region.offset.x;
		viewport.y = region.offset.y;
		viewport.width = region.extent.width;
		viewport.height = region.extent.height;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(command_buffer, 0, 1, &viewport);
	}

	export void set_viewport(VkCommandBuffer command_buffer, VkViewport viewport){
		vkCmdSetViewport(command_buffer, 0, 1, &viewport);
	}
}
