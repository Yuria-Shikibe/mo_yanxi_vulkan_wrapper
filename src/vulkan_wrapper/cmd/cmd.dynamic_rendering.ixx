module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.cmd:dynamic_rendering;
import std;

namespace mo_yanxi::vk{
	export
	struct dynamic_rendering{
	protected:
		std::vector<VkRenderingAttachmentInfo> colorAttachmentsInfo{};
		VkRenderingAttachmentInfo depthInfo{};
		VkRenderingAttachmentInfo stencilInfo{};

	public:
		[[nodiscard]] dynamic_rendering() = default;

		template <std::ranges::input_range Rng = std::initializer_list<VkImageView>>
			requires (std::convertible_to<std::ranges::range_reference_t<Rng>, VkImageView>)
		[[nodiscard]] explicit(false) dynamic_rendering(const Rng& colorAttachments, VkImageView depth = nullptr, VkImageView stencil = nullptr){
			for (VkImageView view : colorAttachments){
				push_color_attachment(view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			}

			if(depth)set_depth_attachment(depth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		}

		void set_depth_attachment(
			VkImageView view,
			const VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			const VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			const VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE,

			VkImageView resolveImageView = nullptr,
			const VkImageLayout resolveImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			const VkResolveModeFlagBits resolveMode = VK_RESOLVE_MODE_MAX_BIT
		){
			depthInfo = VkRenderingAttachmentInfo{
				.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
				.pNext = nullptr,
				.imageView = view,
				.imageLayout = layout,
				.resolveMode = resolveImageView ? resolveMode : VK_RESOLVE_MODE_NONE,
				.resolveImageView = resolveImageView,
				.resolveImageLayout = resolveImageLayout,
				.loadOp = loadOp,
				.storeOp = storeOp,
				.clearValue = {
					.depthStencil = {1.f},
				}
			};
		}

		void clear_color_attachments(){
			colorAttachmentsInfo.clear();
		}

		void push_color_attachment(
			VkImageView view,
			const VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			const VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			const VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE,

			VkImageView resolveImageView = nullptr,
			const VkImageLayout resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			const VkResolveModeFlagBits resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT
			){
			colorAttachmentsInfo.push_back(VkRenderingAttachmentInfo{
				.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
				.pNext = nullptr,
				.imageView = view,
				.imageLayout = layout,
				.resolveMode = resolveImageView ? resolveMode : VK_RESOLVE_MODE_NONE,
				.resolveImageView = resolveImageView,
				.resolveImageLayout = resolveImageLayout,
				.loadOp = loadOp,
				.storeOp = storeOp,
				.clearValue = {}
			});
		}

		void begin_rendering(VkCommandBuffer commandBuffer, const VkRect2D& area) const{
			const VkRenderingInfo info{
				.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
				.pNext = nullptr,
				.flags = 0,
				.renderArea = area,
				.layerCount = 1,
				.viewMask = 0,
				.colorAttachmentCount = static_cast<std::uint32_t>(colorAttachmentsInfo.size()),
				.pColorAttachments = colorAttachmentsInfo.data(),
				.pDepthAttachment = depthInfo.imageView ? &depthInfo : nullptr,
				.pStencilAttachment = nullptr
			};

			vkCmdBeginRendering(commandBuffer, &info);
		}

		void begin_rendering(
			VkCommandBuffer commandBuffer,
			std::integral auto x,
			std::integral auto y,
			std::integral auto w,
			std::integral auto h
		) const{
			begin_rendering(commandBuffer, {
				.offset = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)},
				.extent = {static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h)}
			});
		}
	};

}
