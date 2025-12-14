module;

#include <cassert>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.vk:resources;

import std;
import mo_yanxi.vk.util;
import mo_yanxi.vk.cmd;
import mo_yanxi.handle_wrapper;
import mo_yanxi.math;

namespace mo_yanxi::vk{
	export
	std::size_t get_format_size(VkFormat format) {
		switch (format) {
			// 8-bit formats
		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R8_SNORM:
			return 1;

			// 16-bit formats
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R16_UNORM:
			return 2;

			// 32-bit formats
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_R32_SFLOAT:
			return 4;

			// 64-bit formats
		case VK_FORMAT_R16G16B16A16_SFLOAT:
		case VK_FORMAT_R32G32_SFLOAT:
			return 8;

			// 128-bit formats
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return 16;

		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
			return 8; //

		default:
			return 0;
		}
	}

	export
	struct buffer_range{
		VkBuffer buffer;
		VkDeviceAddress address;
		VkDeviceSize offset;
		VkDeviceSize size;

		[[nodiscard]] constexpr buffer_range sub_range(VkDeviceSize off, VkDeviceSize size) const noexcept{
			assert(off + size <= this->size);
			return buffer_range{buffer, address + offset + off, size};
		}

		[[nodiscard]] constexpr VkDeviceAddress begin() const noexcept{
			return address + offset;
		}

		[[nodiscard]] constexpr VkDeviceAddress end() const noexcept{
			return address + offset + size;
		}
	};

	export struct buffer : resource_base<VkBuffer>{
	protected:
		VkDeviceSize size{};

		void create(const VkBufferCreateInfo& create_info, const VmaAllocationCreateInfo& alloc_create_info){
			pass_alloc(get_allocator().allocate(create_info, alloc_create_info));
			size = create_info.size;
		}

	public:
		[[nodiscard]] buffer() = default;

		[[nodiscard]] explicit buffer(
			const vk::allocator_usage allocator,
			const VkBufferCreateInfo& create_info,
			const VmaAllocationCreateInfo& alloc_create_info
		)
			: resource_base(allocator){
			if(create_info.size)create(create_info, alloc_create_info);
		}

		[[nodiscard]] constexpr VkDeviceSize get_size() const noexcept{
			return size;
		}

		template <contigious_range_of<VkBufferCopy> Rng = std::initializer_list<VkBufferCopy>>
		void copy_from(VkCommandBuffer command_buffer, VkBuffer src, Rng&& rng) const noexcept{
			cmd::copy_buffer(command_buffer, src, get(), std::forward<Rng>(rng));
		}

		template <contigious_range_of<VkBufferCopy> Rng = std::initializer_list<VkBufferCopy>>
		void copy_to(VkCommandBuffer command_buffer, VkBuffer dst, Rng&& rng) const noexcept{
			cmd::copy_buffer(command_buffer, get(), dst, std::forward<Rng>(rng));
		}

		[[nodiscard]] VkDeviceAddress get_address() const noexcept{
			const VkBufferDeviceAddressInfo bufferDeviceAddressInfo{
				.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
				.pNext = nullptr,
				.buffer = handle
			};

			return vkGetBufferDeviceAddress(get_device(), &bufferDeviceAddressInfo);
		}

		buffer_range borrow(VkDeviceSize off, VkDeviceSize size) const noexcept{
			assert(off + size < get_size());
			return buffer_range{*this, get_address(), off, size};
		}

		buffer_range borrow_after(VkDeviceSize off) const noexcept{
			assert(off < get_size());
			return buffer_range{*this, get_address(), off, get_size() - off};
		}

		buffer_range borrow() const noexcept{
			return buffer_range{*this, get_address(), 0, get_size()};
		}
	};

	export struct buffer_cpu_to_gpu : buffer{
	private:
		exclusive_handle_member<std::byte*> mapped{};

	public:
		[[nodiscard]] buffer_cpu_to_gpu() = default;

		[[nodiscard]] buffer_cpu_to_gpu(allocator_usage allocator, VkDeviceSize size_in_bytes, VkBufferUsageFlags usage)
		: buffer(allocator, {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size_in_bytes,
			.usage = usage
		}, {
			.usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
			.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT
		}), mapped(static_cast<std::byte*>(buffer::map())){

		}

		~buffer_cpu_to_gpu(){
			if(mapped && buffer::operator bool()){
				buffer::unmap();
			}
		}

		buffer_cpu_to_gpu(buffer_cpu_to_gpu&& other) noexcept = default;
		buffer_cpu_to_gpu& operator=(buffer_cpu_to_gpu&& other) noexcept = default;

		[[nodiscard]] void* map() const noexcept{
			return mapped;
		}

		static void unmap() noexcept{

		}
	};

	export struct storage_buffer : buffer{

	private:
		VkBufferUsageFlags usage{};
	public:
		[[nodiscard]] storage_buffer() = default;

		[[nodiscard]] storage_buffer(const vk::allocator_usage allocator, VkDeviceSize size, VkBufferUsageFlags usages)
			: buffer(allocator, {
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = size,
				.usage = usages,
			}, {.usage = VMA_MEMORY_USAGE_GPU_ONLY}), usage(usages){
		}

		void resize(VkDeviceSize size){
			this->operator=(storage_buffer{get_allocator(), size, usage});
		}
	};

	export
	template <std::derived_from<buffer> Bty>
	struct buffer_mapper{
		using buffer_type = Bty;

	protected:
		exclusive_handle_member<buffer_type*> buffer_obj;
		exclusive_handle_member<std::byte*> mapped;

	public:
		[[nodiscard]] explicit(false) buffer_mapper(buffer_type& buffer)
			: buffer_obj(&buffer){
			mapped = static_cast<std::byte*>(buffer.map());
		}

		~buffer_mapper(){
			if(mapped){
				buffer_obj->unmap();
				auto flags = buffer_obj->get_allocation_prop_flags();
				if(!(flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){
					buffer_obj->flush();
				}
			}
		}

		[[nodiscard]] std::byte* get_mapper_ptr() const noexcept{
			return mapped;
		}

		buffer_type& base() noexcept{
			assert(buffer_obj);
			return *buffer_obj;
		}

		const buffer_type& base() const noexcept{
			assert(buffer_obj);
			return *buffer_obj;
		}

		template <typename T>
			requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
		const buffer_mapper& load(const T& data, const std::ptrdiff_t dst_byte_offset = 0) const noexcept{
			std::memcpy(mapped + dst_byte_offset, &data, sizeof(T));
			return *this;
		}

		template <typename T, typename O>
			requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
		const buffer_mapper& load(const T& data, T O::* mptr, const std::ptrdiff_t chunk_index = 0) const noexcept{
			std::memcpy(mapped + chunk_index * sizeof(O) + std::bit_cast<std::uint32_t>(mptr), &data, sizeof(T));
			return *this;
		}

		template <typename T>
			requires (std::is_trivially_copyable_v<T>)
		const buffer_mapper& load(const T* data, const std::size_t count, const std::ptrdiff_t dst_byte_offset = 0) const noexcept{
			std::memcpy(mapped + dst_byte_offset, data, sizeof(T) * count);
			return *this;
		}

		const buffer_mapper& load(const void* data, const std::size_t size, const std::ptrdiff_t dst_byte_offset = 0) const noexcept{
			std::memcpy(mapped + dst_byte_offset, data, size);
			return *this;
		}

		template <typename T>
			requires (std::is_trivially_copyable_v<T>)
		const buffer_mapper& load_range(const std::span<const T> span, const std::ptrdiff_t dst_byte_offset = 0) const noexcept{
			std::memcpy(mapped + dst_byte_offset, span.data(), span.size_bytes());
			return *this;
		}

		template <std::ranges::contiguous_range T>
			requires (std::is_trivially_copyable_v<std::ranges::range_value_t<T>> && std::ranges::sized_range<T>)
		const buffer_mapper& load_range(const T& range, const std::ptrdiff_t dst_byte_offset = 0) const noexcept{
			std::memcpy(mapped + dst_byte_offset, std::ranges::cdata (range), std::ranges::size(range) * sizeof(std::ranges::range_value_t<T>));
			return *this;
		}

		const buffer_mapper& fill(const int value) const noexcept{
			std::memset(mapped, value, base().get_size());
			return *this;
		}

		buffer_mapper(const buffer_mapper& other) = delete;
		buffer_mapper(buffer_mapper&& other) noexcept = default;
		buffer_mapper& operator=(const buffer_mapper& other) = delete;
		buffer_mapper& operator=(buffer_mapper&& other) noexcept = default;
	};

	export struct image : resource_base<VkImage>{
		static constexpr VkImageSubresourceRange default_image_subrange{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		};

		static constexpr VkImageSubresourceRange depth_image_subrange{
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		};

	protected:
		VkExtent3D extent{};

		void create(const VkImageCreateInfo& create_info, const VmaAllocationCreateInfo& alloc_create_info){
			pass_alloc(get_allocator().allocate(create_info, alloc_create_info));
			extent = create_info.extent;
		}

	public:
		[[nodiscard]] image() noexcept = default;

		[[nodiscard]] image(
			const allocator_usage allocator,
			const VkImageCreateInfo& create_info,
			const VmaAllocationCreateInfo& alloc_create_info
		)
			: resource_base(allocator){
			create(create_info, alloc_create_info);
		}

		[[nodiscard]] image(
			const allocator_usage allocator,
			const VkExtent3D extent,
			const VkImageUsageFlags usage,
			const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
			const std::uint32_t mipLevels = 1,
			const std::uint32_t arrayLayers = 1,
			const VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
			const VkImageType type = VK_IMAGE_TYPE_MAX_ENUM /*Auto Deduction*/
		) : image{
				allocator,
				{
					.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.imageType = type != VK_IMAGE_TYPE_MAX_ENUM
						             ? type
						             : extent.depth > 1
						             ? VK_IMAGE_TYPE_3D
						             : extent.height > 1
						             ? VK_IMAGE_TYPE_2D
						             : VK_IMAGE_TYPE_1D,
					.format = format,
					.extent = extent,
					.mipLevels = mipLevels,
					.arrayLayers = arrayLayers,
					.samples = samples,
					.tiling = VK_IMAGE_TILING_OPTIMAL,
					.usage = usage,
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
				},
				{
					.usage = VMA_MEMORY_USAGE_GPU_ONLY,
				}
			}{
		}

		[[nodiscard]] VkExtent3D get_extent() const noexcept{
			return extent;
		}

		[[nodiscard]] VkExtent2D get_extent2() const noexcept{
			return {extent.width, extent.height};
		}

		[[nodiscard]] VkImageType type() const noexcept{
			if(!handle || !extent.width || !extent.height || !extent.depth){
				return VK_IMAGE_TYPE_MAX_ENUM;
			}

			if(extent.depth == 1){
				if(extent.height == 1) return VK_IMAGE_TYPE_2D;
				return VK_IMAGE_TYPE_2D;
			}

			return VK_IMAGE_TYPE_3D;
		}

		void init_layout_write(
			VkCommandBuffer command_buffer,
			VkImageLayout layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			const VkImageSubresourceRange& range = default_image_subrange,
			const std::uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			const std::uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED) const noexcept{
			cmd::memory_barrier(
				command_buffer, handle,
				VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
				VK_ACCESS_2_NONE,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				VK_ACCESS_2_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				layout,
				range,
				srcQueueFamilyIndex,
				dstQueueFamilyIndex);
		}

		void init_layout(
			VkCommandBuffer command_buffer,

			VkPipelineStageFlags2 stage,
			VkAccessFlags2 access,
			VkImageLayout dstLayout,

			std::uint32_t mipLevels = 1,
			std::uint32_t arrayLayers = 1
			) const noexcept{
			cmd::memory_barrier(
				command_buffer, handle,
				VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
				VK_ACCESS_2_NONE,
				stage,
				access,
				VK_IMAGE_LAYOUT_UNDEFINED,
				dstLayout,
				{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = mipLevels,
					.baseArrayLayer = 0,
					.layerCount = arrayLayers
				});
		}
	};

	export struct image_view : exclusive_handle<VkImageView>{
	private:
		exclusive_handle_member<VkDevice> device{};

	public:
		[[nodiscard]] image_view() = default;

		[[nodiscard]] image_view(VkDevice device, const VkImageViewCreateInfo& create_info) : device(device){
			if(const auto rst = vkCreateImageView(device, &create_info, nullptr, as_data())){
				throw vk_error(rst, "Failed to create image view");
			}
		}

		~image_view(){
			if(get_device())vkDestroyImageView(get_device(), handle, nullptr);
		}

		image_view(const image_view& other) = delete;
		image_view(image_view&& other) noexcept = default;
		image_view& operator=(const image_view& other) = delete;
		image_view& operator=(image_view&& other) noexcept = default;

		[[nodiscard]] VkDevice get_device() const noexcept{
			return device;
		}
	};
}

namespace mo_yanxi::vk::templates{
	export
	[[nodiscard]] buffer create_staging_buffer(const allocator_usage allocator, VkDeviceSize size){
		return buffer{
			allocator,
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = size,
				.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			}, {
				.usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
			}
		};
	}

	export
	[[nodiscard]] buffer create_index_buffer(const allocator_usage allocator, const std::size_t count){
		return buffer{
			allocator,
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = count * sizeof(std::uint32_t),
				.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			}, {
				.usage = VMA_MEMORY_USAGE_GPU_ONLY,
			}
		};
	}

	export
	[[nodiscard]] buffer create_vertex_buffer(const allocator_usage allocator, const std::size_t size_in_bytes){
		return buffer{
			allocator,
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = size_in_bytes,
				.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			}, {
				.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT
			}
		};
	}

	export
	[[nodiscard]] buffer create_storage_buffer(const allocator_usage allocator, const std::size_t size_in_bytes, VkBufferUsageFlags append_usage = 0){
		return buffer{
			allocator,
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = size_in_bytes,
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | append_usage
			}, {
				.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT
			}
		};
	}

	export
	[[nodiscard]] buffer create_uniform_buffer(const allocator_usage allocator, const std::size_t size_in_bytes){
		return buffer{
			allocator,
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = size_in_bytes,
				.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			}, {
				.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT
			}
		};
	}
}

namespace mo_yanxi::vk{
	template <typename T>
	struct void_or{
		T value;

		template <typename Sty>
		constexpr explicit(false) operator decltype(std::forward_like<Sty>(std::declval<T&>())) (this Sty&& self) noexcept{
			return std::forward_like<Sty>(self.value);
		}
	};

	template <>
	struct void_or<void>{

	};

	export
	struct temp_resource_reserver{
		struct promise_type;
		using handle = std::coroutine_handle<promise_type>;
		using value_type = VkCommandBuffer;

		[[nodiscard]] temp_resource_reserver() = default;

		[[nodiscard]] explicit temp_resource_reserver(handle&& hdl)
			: hdl{std::move(hdl)}{}

		struct promise_type{
			[[nodiscard]] promise_type() = default;

			temp_resource_reserver get_return_object(){
				return temp_resource_reserver{handle::from_promise(*this)};
			}

			[[nodiscard]] static auto initial_suspend() noexcept{ return std::suspend_never{}; }

			[[nodiscard]] static auto final_suspend() noexcept{ return std::suspend_always{}; }

			static void return_void() noexcept{

			}

			auto yield_value(value_type val) noexcept{
				toSubmit = val;
				return std::suspend_always{};
			}

			[[noreturn]] static void unhandled_exception() noexcept{
				std::terminate();
			}

		private:
			friend temp_resource_reserver;

			value_type toSubmit;
		};

		void resume() const{
			hdl->resume();
		}

		[[nodiscard]] bool done() const noexcept{
			return hdl->done();
		}

		[[nodiscard]] value_type current() const noexcept{
			return hdl->promise().toSubmit;
		}

		~temp_resource_reserver(){
			if(hdl){
				assert(done());
				hdl->destroy();
			}
		}

		temp_resource_reserver(const temp_resource_reserver& other) = delete;
		temp_resource_reserver(temp_resource_reserver&& other) noexcept = default;
		temp_resource_reserver& operator=(const temp_resource_reserver& other) = delete;
		temp_resource_reserver& operator=(temp_resource_reserver&& other) noexcept = default;

	private:
		exclusive_handle_member<handle> hdl{};
	};

	export
	template <typename T, typename ExtentTy>
		requires (std::is_trivially_copyable_v<T>)
	[[nodiscard]] temp_resource_reserver dump_image(
		allocator& alloc,
		VkCommandBuffer command_buffer,
		VkImage image,
		VkFormat image_format,
		VkRect2D region,

		std::mdspan<T, ExtentTy> target_row_major,

		VkPipelineStageFlags2 currentStage,
		VkAccessFlags2 currentAccess,
		VkImageLayout currentLayout,
		std::uint32_t mipmapLevel = 0,
		VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
		std::uint32_t layer = 0){

		const auto div = math::pow_integral(2u, mipmapLevel);
		const VkRect2D scaled{
			static_cast<std::int32_t>(region.offset.x / div), static_cast<std::int32_t>(region.offset.y / div), region.extent.width / div, region.extent.height / div
		};

		const std::size_t formatSize{get_format_size(image_format)};
		const auto staging_buffer = templates::create_staging_buffer(alloc, formatSize * target_row_major.size() / 4);

		cmd::memory_barrier(command_buffer, image,
			currentStage,
			currentAccess,
			VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			VK_ACCESS_2_TRANSFER_READ_BIT,
			currentLayout,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			{
				.aspectMask = aspectFlags,
				.baseMipLevel = mipmapLevel,
				.levelCount = 1,
				.baseArrayLayer = layer,
				.layerCount = 1
			}
		);


		cmd::copy_image_to_buffer(
			command_buffer, image, staging_buffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, {
				VkBufferImageCopy{
					.bufferOffset = 0,
					.bufferRowLength = static_cast<std::uint32_t>(target_row_major.extent(0)),
					.bufferImageHeight = static_cast<std::uint32_t>(target_row_major.extent(1)),
					.imageSubresource = {
						.aspectMask = aspectFlags,
						.mipLevel = mipmapLevel,
						.baseArrayLayer = layer,
						.layerCount = 1
					},
					.imageOffset = {scaled.offset.x, scaled.offset.y, 0},
					.imageExtent = {scaled.extent.width, scaled.extent.height, 1},
				}
			});

		cmd::memory_barrier(command_buffer, image,
			VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			VK_ACCESS_2_TRANSFER_READ_BIT,
			currentStage,
			currentAccess,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			currentLayout,
			{
				.aspectMask = aspectFlags,
				.baseMipLevel = mipmapLevel,
				.levelCount = 1,
				.baseArrayLayer = layer,
				.layerCount = 1
			}
		);

		co_yield command_buffer;

		auto mapped = staging_buffer.map();

		std::memcpy(target_row_major.data_handle(), mapped, staging_buffer.get_size());

		staging_buffer.unmap();
	}
}
