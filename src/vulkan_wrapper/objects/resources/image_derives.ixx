module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk:image_derives;

export import mo_yanxi.vk.universal_handle;
import mo_yanxi.handle_wrapper;
import mo_yanxi.vk.util;

//TODO remove bitmap
import mo_yanxi.bitmap;
import :resources;

import std;

namespace mo_yanxi::vk{
	export
	template <typename Prov>
	concept texture_source_prov = std::is_invocable_r_v<bitmap, Prov, std::uint32_t, std::uint32_t, std::uint32_t>;

	export
	constexpr VkDeviceSize get_mipmap_pixels(VkDeviceSize src_area, std::uint32_t mipLevels) noexcept{
		const auto pow4_m = 1ULL << (2 * mipLevels);
		return 4 * src_area * (pow4_m - 1) / pow4_m / 3;
	}

	constexpr VkDeviceSize v = get_mipmap_pixels(4, 2);

	export
	struct combined_image{
	protected:
		image image{};
		image_view image_view{};

	public:
		[[nodiscard]] combined_image() = default;

		[[nodiscard]] combined_image(vk::image&& image, const VkImageViewCreateInfo& image_view_create_info)
			: image(std::move(image)),
			  image_view(this->image.get_device(), [](VkImage img, VkImageViewCreateInfo info) static{
				  info.image = img;
				  return info;
			  }(this->image.get(), image_view_create_info)){
		}

		explicit operator bool() const noexcept{
			return static_cast<bool>(image);
		}

		[[nodiscard]] const vk::image& get_image() const noexcept{
			return image;
		}

		[[nodiscard]] const vk::image_view& get_image_view() const noexcept{
			return image_view;
		}

		explicit(false) operator image_handle() const noexcept{
			return {
				image, image_view
			};
		}
	};

	export constexpr std::uint32_t get_recommended_mip_level(const std::uint32_t w, const std::uint32_t h) noexcept{
		return static_cast<std::uint32_t>(std::countr_zero(std::bit_floor(std::max(w, h))) + 1);
	}

	export constexpr std::uint32_t get_recommended_mip_level(const VkExtent2D extent, const std::uint32_t ceil = std::numeric_limits<std::uint32_t>::max()) noexcept{
		return std::min(get_recommended_mip_level(extent.width, extent.height), ceil);
	}


	export
	struct texture_bitmap_write{
		const bitmap& bitmap;
		math::point2 offset;
	};

	export
	struct texture : combined_image{
	private:
		std::uint32_t mipLevel{};
		std::uint32_t layers{};
		VkFormat format_{};

	public:
		[[nodiscard]] texture() = default;

		[[nodiscard]] texture(
			allocator& allocator,
			const VkExtent2D extent,
			const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
			const std::uint32_t layers = 1) :
			combined_image({
				               allocator,
				               {extent.width, extent.height, 1},
				               VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				               format,
				               get_recommended_mip_level(extent, 5), layers, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D
			               }, {
					               .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					               .viewType = layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D,
					               .format = format,
					               .components = {},
					               .subresourceRange = VkImageSubresourceRange{
						               .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						               .baseMipLevel = 0,
						               .levelCount = get_recommended_mip_level(extent, 5),
						               .baseArrayLayer = 0,
						               .layerCount = layers
					               }
				               }),
			mipLevel(get_recommended_mip_level(extent, 5)),
			layers(layers),
			format_(format)
		{
		}

		[[nodiscard]] std::uint32_t get_mip_level() const noexcept{
			return mipLevel;
		}

		[[nodiscard]] std::uint32_t get_layers() const noexcept{
			return layers;
		}

		void init_layout(VkCommandBuffer command_buffer) const noexcept{
			image.init_layout(
				command_buffer,
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
				VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				mipLevel, layers
			);
		}

		template <texture_source_prov Prov>
		[[nodiscard("Staging buffers must be reserved until the device has finished the command")]]
		buffer write(VkCommandBuffer command_buffer,
			VkRect2D region,
			Prov prov,
			std::uint32_t maxProvMipLevel = 2,
			const std::uint32_t baseLayer = 0,
			const std::uint32_t layerCount = 0 /*0 for all*/){
			if(region.extent.width == 0 || region.extent.height == 0){
				return {};
			}
			maxProvMipLevel = std::min(maxProvMipLevel, mipLevel);

			constexpr VkDeviceSize bpi = 4;

			buffer staging{templates::create_staging_buffer(
				*image.get_allocator(),
				get_mipmap_pixels(region.extent.width * region.extent.height, maxProvMipLevel) * bpi)
			};

			this->write(command_buffer, region, std::move(prov), staging, maxProvMipLevel, baseLayer, layerCount);

			return staging;
		}

		template <texture_source_prov Prov>
		void write(VkCommandBuffer command_buffer,
			VkRect2D region,
			Prov prov,
			vk::buffer& buffer,
			std::uint32_t maxProvMipLevel = 3,
			const std::uint32_t baseLayer = 0,
			const std::uint32_t layerCount = 0 /*0 for all*/){
			if(region.extent.width == 0 || region.extent.height == 0){
				return;
			}
			maxProvMipLevel = std::min(maxProvMipLevel, mipLevel);

			constexpr VkDeviceSize bpi = 4;

			VkDeviceSize off{};
			for(std::uint32_t mip_lv = 0; mip_lv < maxProvMipLevel; ++mip_lv){
				auto scl = 1u << mip_lv;
				VkExtent2D extent{region.extent.width / scl, region.extent.height / scl};
				const bitmap& bitmap = prov(extent.width, extent.height, mip_lv);
				(void)buffer_mapper{buffer}.load_range(bitmap.to_span(), static_cast<std::ptrdiff_t>(off));
				off += extent.width * extent.height * bpi;
			}
			const auto lc = layerCount ? layerCount : layers;
			cmd::generate_mipmaps(command_buffer, image, buffer, region, mipLevel, maxProvMipLevel, baseLayer, lc);
		}

		template <range_of<texture_bitmap_write> Rng = std::initializer_list<texture_bitmap_write>>
		[[nodiscard("Staging buffers must be reserved until the device has finished the command")]]
		std::vector<buffer> write(
			VkCommandBuffer command_buffer,
			const Rng& args,
			const std::uint32_t baseLayer = 0,
			const std::uint32_t layerCount = 0 /*0 for all*/
			){
			if(std::ranges::empty(args))return {};

			std::vector<buffer> reservation{};
			std::vector<texture_buffer_write> trans{};
			reservation.reserve(std::ranges::size(args));
			trans.reserve(reservation.size());

			for(const texture_bitmap_write& arg : args){
				auto buffer = templates::create_staging_buffer(image.get_allocator(), arg.bitmap.size_bytes());

				(void)buffer_mapper{buffer}.load_range(arg.bitmap.to_span());
				trans.emplace_back(buffer, VkRect2D{arg.offset.x, arg.offset.y, arg.bitmap.width(), arg.bitmap.height()});

				reservation.push_back(std::move(buffer));
			}

			write(command_buffer, trans, baseLayer, layerCount);

			return reservation;
		}

		template <range_of<texture_buffer_write> Rng = std::initializer_list<texture_buffer_write>>
		void write(
			VkCommandBuffer command_buffer,
			const Rng& args,
			const std::uint32_t baseLayer = 0,
			const std::uint32_t layerCount = 0 /*0 for all*/
			){

			if(std::ranges::empty(args))return;

			const auto lc = layerCount ? layerCount : layers;

			VkImageMemoryBarrier barrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = image,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = mipLevel,
					.baseArrayLayer = baseLayer,
					.layerCount = lc
				}
			};

			vkCmdPipelineBarrier(
					command_buffer,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

			for(const texture_buffer_write& arg : args){
				cmd::copy_buffer_to_image(command_buffer, arg.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {VkBufferImageCopy{
					.bufferOffset = arg.buffer_offset,
					.imageSubresource = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.mipLevel = 0,
						.baseArrayLayer = baseLayer,
						.layerCount = lc
					},
					.imageOffset = {arg.region.offset.x, arg.region.offset.y, 0},
					.imageExtent = {arg.region.extent.width, arg.region.extent.height, 1},
				}});
			}

			if(std::ranges::size(args) == 1){
				std::array regions{static_cast<const texture_buffer_write&>(*std::ranges::begin(args)).region};
				cmd::generate_mipmaps(command_buffer, get_image(), regions, mipLevel, 1, baseLayer, lc);
			}else{
				std::vector<VkRect2D> regions;
				cmd::generate_mipmaps(command_buffer, get_image(), regions, mipLevel, 1, baseLayer, lc);
			}
		}
	};

	export
	struct color_attachment : combined_image{
	private:
		VkFormat format_;
		VkImageUsageFlags usage;
		VkSampleCountFlagBits samples;
	public:
		[[nodiscard]] color_attachment() = default;

		[[nodiscard]] color_attachment(
			allocator& allocator,
			const VkExtent2D extent,
			VkImageUsageFlags usage,
			VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
			VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT) :
			combined_image({
				               allocator,
				               {extent.width, extent.height, 1},
				               usage,
				               format,
				               1, 1, samples, VK_IMAGE_TYPE_2D
			               }, {
					               .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					               .viewType = VK_IMAGE_VIEW_TYPE_2D,
					               .format = format,
					               .components = {},
					               .subresourceRange = image::default_image_subrange
				               }),
			usage{usage}, samples(samples), format_{format}{
		}

		void init_layout(VkCommandBuffer command_buffer) const noexcept{
			image.init_layout(
				command_buffer,
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			);
		}

		void set_layout_to_read_optimal(
			VkCommandBuffer command_buffer,
			const std::uint32_t src_queue,
			const std::uint32_t dst_queue
			) const noexcept{
			vk::cmd::memory_barrier(
				command_buffer,
				get_image(),
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				vk::image::default_image_subrange,
				src_queue,
				dst_queue
			);
		}

		void resize(VkExtent2D extent) noexcept{
			image = {
				image.get_allocator(),
				{extent.width, extent.height, 1},
				usage,
				format_,
				1, 1, samples, VK_IMAGE_TYPE_2D
			};

			image_view = {image.get_device(), {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = image.get(),
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = format_,
				.components = {},
				.subresourceRange = image::default_image_subrange
			}};
		}

		[[nodiscard]] VkImageUsageFlags get_usage() const{
			return usage;
		}

		[[nodiscard]] VkSampleCountFlagBits get_samples() const{
			return samples;
		}
	};

	export
	struct storage_image : combined_image{
		static constexpr VkImageUsageFlags default_usage = 0
			| VK_IMAGE_USAGE_STORAGE_BIT
			| VK_IMAGE_USAGE_TRANSFER_SRC_BIT
			| VK_IMAGE_USAGE_TRANSFER_DST_BIT
			| VK_IMAGE_USAGE_SAMPLED_BIT;

	private:
		VkImageUsageFlags usage{};
		VkFormat format{};
		std::uint32_t mip_level{};

	public:
		[[nodiscard]] storage_image() = default;

		[[nodiscard]] storage_image(
			allocator& allocator,
			const VkExtent2D extent,
			const std::uint32_t mip_level = 1,
			const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
			const VkImageUsageFlags usage = default_usage
			)
			: combined_image({
				                 allocator,
				                 {extent.width, extent.height, 1},
				                 default_usage,
				                 format,
				                 mip_level, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D
			                 }, {
				                 .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				                 .viewType = VK_IMAGE_VIEW_TYPE_2D,
				                 .format = format,
				                 .components = {},
				                 .subresourceRange = VkImageSubresourceRange{
					                 .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					                 .baseMipLevel = 0,
					                 .levelCount = mip_level,
					                 .baseArrayLayer = 0,
					                 .layerCount = 1
				                 }
			                 }
			  ), usage(usage), format{format}, mip_level(mip_level){
		}




		[[nodiscard]] VkFormat get_format() const{
			return format;
		}

		[[nodiscard]] std::uint32_t get_mip_level() const{
			return mip_level;
		}

		void resize(VkExtent2D extent) noexcept{
			if(extent.width == image.get_extent2().width && extent.height == image.get_extent2().height)return;

			image = {
				image.get_allocator(),
				{extent.width, extent.height, 1},
				usage,
				format,
				mip_level, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D
			};

			image_view = {image.get_device(), {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = image.get(),
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = format,
				.components = {},
				.subresourceRange = VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = mip_level,
					.baseArrayLayer = 0,
					.layerCount = 1
				 }
			}};
		}

		void init_layout_general(VkCommandBuffer command_buffer) const noexcept{
			get_image().init_layout(command_buffer,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
				VK_IMAGE_LAYOUT_GENERAL
			);
		}

		[[nodiscard]] VkImageUsageFlags get_usage() const{
			return usage;
		}
	};
}
