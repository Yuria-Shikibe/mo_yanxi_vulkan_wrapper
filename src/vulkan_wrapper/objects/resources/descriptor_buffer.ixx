module;

#include <vulkan/vulkan.h>
#include <cassert>

export module mo_yanxi.vk:descriptor_buffer;

import std;
import mo_yanxi.vk.util;
import mo_yanxi.handle_wrapper;
import :resources;
import :ext;

namespace mo_yanxi::vk {

	// =========================================================================
	// 配置与属性
	// =========================================================================

	export struct binding_spec {
		std::uint32_t binding;
		std::uint32_t count;
		VkDescriptorType type;
	};

	struct descriptor_properties {
		std::size_t uniformBufferDescriptorSize{};
		std::size_t combinedImageSamplerDescriptorSize{};
		std::size_t storageImageDescriptorSize{};
		std::size_t inputAttachmentDescriptorSize{};
		std::size_t storageBufferDescriptorSize{};
		std::size_t descriptorBufferOffsetAlignment{};

		void init(VkPhysicalDevice physical_device) {
			VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptor_buffer_properties{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT,
				.pNext = nullptr,
			};

			VkPhysicalDeviceProperties2KHR device_properties{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR,
				.pNext = &descriptor_buffer_properties,
			};

			getPhysicalDeviceProperties2KHR(physical_device, &device_properties);

			uniformBufferDescriptorSize = descriptor_buffer_properties.uniformBufferDescriptorSize;
			combinedImageSamplerDescriptorSize = descriptor_buffer_properties.combinedImageSamplerDescriptorSize;
			storageImageDescriptorSize = descriptor_buffer_properties.storageImageDescriptorSize;
			inputAttachmentDescriptorSize = descriptor_buffer_properties.inputAttachmentDescriptorSize;
			storageBufferDescriptorSize = descriptor_buffer_properties.storageBufferDescriptorSize;
			descriptorBufferOffsetAlignment = descriptor_buffer_properties.descriptorBufferOffsetAlignment;
		}
	};

	// =========================================================================
	// Buffer 基类 (共享属性与偏移量管理)
	// =========================================================================

	export struct descriptor_buffer_base : buffer {
	protected:
		descriptor_properties properties{};
		std::vector<VkDeviceSize> offsets{};

		void init_base(VkPhysicalDevice pd, VkDevice device, VkDescriptorSetLayout layout, std::uint32_t binding_count) {
			properties.init(pd);
			offsets.resize(binding_count);

			for (auto&& [i, offset] : offsets | std::views::enumerate) {
				getDescriptorSetLayoutBindingOffsetEXT(device, layout, static_cast<std::uint32_t>(i), offsets.data() + i);
			}
		}

	public:
		[[nodiscard]] descriptor_buffer_base() = default;

		[[nodiscard]] std::size_t get_uniform_buffer_descriptor_size() const noexcept { return properties.uniformBufferDescriptorSize; }
		[[nodiscard]] std::size_t get_combined_image_sampler_descriptor_size() const noexcept { return properties.combinedImageSamplerDescriptorSize; }
		[[nodiscard]] std::size_t get_storage_image_descriptor_size() const noexcept { return properties.storageImageDescriptorSize; }
		[[nodiscard]] std::size_t get_storage_buffer_descriptor_size() const noexcept { return properties.storageBufferDescriptorSize; }
		[[nodiscard]] std::size_t get_input_attachment_descriptor_size() const noexcept { return properties.inputAttachmentDescriptorSize; }
		[[nodiscard]] std::size_t get_offset_alignment() const noexcept { return properties.descriptorBufferOffsetAlignment; }

		[[nodiscard]] VkDeviceSize get_binding_offset(std::uint32_t binding) const noexcept {
			if (binding >= offsets.size()) return 0;
			return offsets[binding];
		}

		explicit(false) operator VkDescriptorBufferBindingInfoEXT() const noexcept{
			return get_bind_info(VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_2_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT);
		}

		[[nodiscard]] VkDescriptorBufferBindingInfoEXT get_bind_info(const VkBufferUsageFlags usage) const {
			return {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
				.pNext = nullptr,
				.address = get_address(),
				.usage = usage
			};
		}

		void bind_to(const VkCommandBuffer commandBuffer, const VkBufferUsageFlags usage) const {
			const auto info = get_bind_info(usage);
			cmd::bindDescriptorBuffersEXT(commandBuffer, 1, &info);
		}

		// 辅助函数：将 ImageInfo 转换为 GetInfo
		[[nodiscard]] std::pair<VkDescriptorGetInfoEXT, VkDeviceSize> get_image_get_info(
			const VkDescriptorImageInfo& imageInfo_non_rv, const VkDescriptorType descriptorType) const {

			std::pair<VkDescriptorGetInfoEXT, VkDeviceSize> rst{
				VkDescriptorGetInfoEXT{
					.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
					.pNext = nullptr,
					.type = descriptorType,
					.data = {}
				},
				{}
			};

			switch (descriptorType) {
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
				rst.first.data.pCombinedImageSampler = &imageInfo_non_rv;
				rst.second = properties.combinedImageSamplerDescriptorSize;
				break;
			}
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
				rst.first.data.pStorageImage = &imageInfo_non_rv;
				rst.second = properties.storageImageDescriptorSize;
				break;
			}
			case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
				rst.first.data.pInputAttachmentImage = &imageInfo_non_rv;
				rst.second = properties.inputAttachmentDescriptorSize;
				break;
			}
			default: throw std::runtime_error("Invalid descriptor type!");
			}
			return rst;
		}
	};

	// =========================================================================
	// 定长 Buffer (Original descriptor_buffer)
	// =========================================================================

	export struct descriptor_buffer : descriptor_buffer_base {
	private:
		std::uint32_t chunk_count{};

	public:
		using descriptor_buffer_base::descriptor_buffer_base;

		[[nodiscard]] descriptor_buffer(
			const allocator_usage allocator,
			VkDescriptorSetLayout layout,
			std::uint32_t bindings,
			std::uint32_t chunk_count = 1
		) : chunk_count{ chunk_count } {

			const auto device = allocator.get_device();
			const auto physical_device = allocator.get_physical_device();

			init_base(physical_device, device, layout, bindings);

			VkDeviceSize dbo_size;
			getDescriptorSetLayoutSizeEXT(device, layout, &dbo_size);

			this->buffer::operator=(buffer{ allocator, {
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = dbo_size * chunk_count,
				.usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			}, {
				.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			} });
		}

		[[nodiscard]] constexpr VkDeviceSize get_chunk_offset(const std::uint32_t chunkIndex) const noexcept {
			assert(chunkIndex < chunk_count);
			return chunkIndex * get_chunk_size();
		}

		[[nodiscard]] constexpr VkDeviceSize get_chunk_size() const noexcept {
			return get_size() / chunk_count;
		}

		[[nodiscard]] constexpr std::uint32_t get_chunk_count() const noexcept {
			return chunk_count;
		}

		void bind_to(
			VkCommandBuffer commandBuffer,
			const VkPipelineBindPoint bindingPoint,
			VkPipelineLayout layout,
			const std::uint32_t setIndex,
			const VkDeviceSize offset = 0,
			const VkBufferUsageFlags usage = VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_2_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
		) const {
			static constexpr std::uint32_t ZERO = 0;
			descriptor_buffer_base::bind_to(commandBuffer, usage);
			cmd::setDescriptorBufferOffsetsEXT(commandBuffer, bindingPoint, layout, setIndex, 1, &ZERO, &offset);
		}

		void bind_chunk_to(
			VkCommandBuffer commandBuffer,
			const VkPipelineBindPoint bindingPoint,
			VkPipelineLayout layout,
			const std::uint32_t setIndex,
			const std::uint32_t chunkIndex = 0,
			const VkBufferUsageFlags usage = VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_2_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
		) const {
			static constexpr std::uint32_t ZERO = 0;
			descriptor_buffer_base::bind_to(commandBuffer, usage);
			const VkDeviceSize offset = get_chunk_offset(chunkIndex);
			cmd::setDescriptorBufferOffsetsEXT(commandBuffer, bindingPoint, layout, setIndex, 1, &ZERO, &offset);
		}
	};

	// =========================================================================
	// 动态 Buffer (New dynamic_descriptor_buffer)
	// =========================================================================

	export struct dynamic_descriptor_buffer : descriptor_buffer_base {
	private:
		struct runtime_binding_info {
			std::size_t stride{};
			std::uint32_t capacity{};
		};
		std::vector<runtime_binding_info> bindings_info{};

		// ---------------------------------------------------------------------
		// 内部辅助：计算布局需求 (被构造函数和 reconfigure 复用)
		// 注意：调用此函数前，必须确保 this->offsets 已经根据当前的 layout 更新完毕
		// ---------------------------------------------------------------------
		template <std::ranges::input_range Rng>
		[[nodiscard]] VkDeviceSize calc_requirements(
			std::uint32_t max_binding_index,
			const Rng& specs
		) {
			bindings_info.resize(max_binding_index);
			VkDeviceSize max_required_size = 0;

			for (const binding_spec& spec : specs) {
				assert(spec.binding < max_binding_index);

				std::size_t current_stride = 0;
				switch (spec.type) {
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: current_stride = properties.combinedImageSamplerDescriptorSize; break;
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: current_stride = properties.storageImageDescriptorSize; break;
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: current_stride = properties.uniformBufferDescriptorSize; break;
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: current_stride = properties.storageBufferDescriptorSize; break;
				case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: current_stride = properties.inputAttachmentDescriptorSize; break;
				case VK_DESCRIPTOR_TYPE_SAMPLER: current_stride = properties.combinedImageSamplerDescriptorSize; break;
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: current_stride = properties.combinedImageSamplerDescriptorSize; break;
				default: throw std::runtime_error("Unsupported variable descriptor type");
				}

				bindings_info[spec.binding] = runtime_binding_info{
					.stride = current_stride,
					.capacity = spec.count
				};

				// 计算该 binding 的结束位置：offset + (count * stride)
				// 依赖于外部已经正确更新了 offsets 数组
				VkDeviceSize binding_end = offsets[spec.binding] + (static_cast<VkDeviceSize>(spec.count) * current_stride);

				if (binding_end > max_required_size) {
					max_required_size = binding_end;
				}
			}

			if (max_required_size == 0) max_required_size = 1024;

			return max_required_size;
		}

	public:
		[[nodiscard]] dynamic_descriptor_buffer() = default;

		[[nodiscard]] dynamic_descriptor_buffer(
			const allocator_usage allocator,
			VkDescriptorSetLayout layout,
			std::uint32_t max_binding_index,
			std::initializer_list<binding_spec> specs
		) {
			const auto device = allocator.get_device();
			const auto physical_device = allocator.get_physical_device();

			// 1. 初始化基类 (获取 properties 并计算初始 offsets)
			init_base(physical_device, device, layout, max_binding_index);

			// 2. 计算需求
			auto required_size = calc_requirements(max_binding_index, specs);

			// 3. 分配内存
			this->buffer::operator=(buffer{ allocator, {
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = required_size,
				.usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			}, {
				.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			} });
		}

		// ---------------------------------------------------------------------
		// 重新配置函数 (复用逻辑，条件分配)
		// ---------------------------------------------------------------------
		template <std::ranges::input_range Rng = std::initializer_list<binding_spec>>
		void reconfigure(
			VkDescriptorSetLayout layout,
			std::uint32_t max_binding_index,
			const Rng& specs
		) {

			const auto device = get_allocator().get_device();

			// 1. 根据新 Layout 更新 Offset 信息
			// descriptor_buffer_base::offsets 是 protected 的，可以直接修改
			offsets.resize(max_binding_index);
			for (auto&& [i, offset] : offsets | std::views::enumerate) {
				getDescriptorSetLayoutBindingOffsetEXT(device, layout, static_cast<std::uint32_t>(i), offsets.data() + i);
			}

			if (auto required_size = this->calc_requirements(max_binding_index, specs); required_size > this->get_size()) {
				this->buffer::operator=(buffer{ get_allocator(), {
					.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
					.size = required_size,
					.usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				}, {
					.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				} });
			}
		}

		void bind_to(
			VkCommandBuffer commandBuffer,
			const VkPipelineBindPoint bindingPoint,
			VkPipelineLayout layout,
			const std::uint32_t setIndex,
			const VkDeviceSize offset = 0,
			const VkBufferUsageFlags usage = VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_2_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
		) const {
			static constexpr std::uint32_t ZERO = 0;
			descriptor_buffer_base::bind_to(commandBuffer, usage);
			cmd::setDescriptorBufferOffsetsEXT(commandBuffer, bindingPoint, layout, setIndex, 1, &ZERO, &offset);
		}

		[[nodiscard]] std::size_t get_binding_stride(std::uint32_t binding) const noexcept {
			if (binding >= bindings_info.size()) return 0;
			return bindings_info[binding].stride;
		}

		[[nodiscard]] std::uint32_t get_binding_capacity(std::uint32_t binding) const noexcept {
			if (binding >= bindings_info.size()) return 0;
			return bindings_info[binding].capacity;
		}
	};

	// =========================================================================
	// 写入逻辑 Helper (提取重复代码)
	// =========================================================================

	struct descriptor_write_helper {
		// 写入 Buffer (UBO/SSBO)
		static void write_buffer(
			VkDevice device,
			std::byte* dst_ptr,
			std::size_t descriptor_size,
			VkDescriptorType type,
			VkDeviceAddress address,
			VkDeviceSize range,
			VkFormat format = VK_FORMAT_UNDEFINED
		) {
			const VkDescriptorAddressInfoEXT addr_info{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
				.pNext = nullptr,
				.address = address,
				.range = range,
				.format = format
			};
			const VkDescriptorGetInfoEXT info{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
				.pNext = nullptr,
				.type = type,
				.data = VkDescriptorDataEXT{
					.pStorageBuffer = &addr_info // Union pointer, type determines usage
				}
			};
			vk::getDescriptorEXT(device, &info, descriptor_size, dst_ptr);
		}

		// 写入 Image (Combined/Storage/Input)
		// 需要 buffer 对象来辅助获取 ImageGetInfo (利用 buffer 中缓存的 properties)
		static void write_image(
			VkDevice device,
			std::byte* dst_ptr,
			const descriptor_buffer_base* buf_base,
			const VkDescriptorImageInfo& imageInfo,
			VkDescriptorType type
		) {
			auto [info, size] = buf_base->get_image_get_info(imageInfo, type);
			vk::getDescriptorEXT(device, &info, size, dst_ptr);
		}
	};

	// =========================================================================
	// 定长 Mapper (逻辑保持不变)
	// =========================================================================

	export struct descriptor_mapper : buffer_mapper<descriptor_buffer> {
		using buffer_mapper<descriptor_buffer>::buffer_mapper;

	private:

		// 辅助：获取定长模式下的写入地址
		// offset = binding_base + chunk_offset + extra_offset
		std::byte* get_ptr(std::uint32_t binding, std::uint32_t chunkIndex, std::size_t byte_offset_in_binding = 0) const {
			auto* buf = buffer_obj.get();
			return mapped + buf->get_binding_offset(binding) + buf->get_chunk_offset(chunkIndex) + byte_offset_in_binding;
		}

	public:
		const descriptor_mapper& set_storage_buffer(
			const std::uint32_t binding,
			const VkDeviceAddress address,
			const VkDeviceSize size,
			const std::uint32_t chunkIndex = 0,
			const VkFormat format = VK_FORMAT_UNDEFINED
		) const {
			descriptor_write_helper::write_buffer(
				buffer_obj->get_device(),
				get_ptr(binding, chunkIndex),
				buffer_obj->get_storage_buffer_descriptor_size(),
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				address, size, format
			);
			return *this;
		}

		const descriptor_mapper& set_uniform_buffer(
			const std::uint32_t binding,
			const buffer& ubo,
			const VkDeviceSize ubo_offset = 0,
			const std::uint32_t chunkIndex = 0
		) const {
			return this->set_uniform_buffer(binding, ubo.get_address() + ubo_offset, ubo.get_size(), chunkIndex);
		}

		const descriptor_mapper& set_uniform_buffer(
			const std::uint32_t binding,
			const VkDeviceAddress address,
			const VkDeviceSize size,
			const std::uint32_t chunkIndex = 0
		) const {
			descriptor_write_helper::write_buffer(
				buffer_obj->get_device(),
				get_ptr(binding, chunkIndex),
				buffer_obj->get_uniform_buffer_descriptor_size(),
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				address, size, VK_FORMAT_UNDEFINED
			);
			return *this;
		}

		const descriptor_mapper& set_uniform_buffer_segments(
			std::uint32_t initialBinding,
			const buffer& ubo,
			std::initializer_list<VkDeviceSize> segment_sizes,
			const std::uint32_t chunkIndex = 0) const {

			VkDeviceSize currentOffset{};
			for (const auto& [idx, sz] : segment_sizes | std::views::enumerate) {
				(void)this->set_uniform_buffer(initialBinding + static_cast<std::uint32_t>(idx), ubo.get_address() + currentOffset, sz, chunkIndex);
				currentOffset += sz;
			}
			return *this;
		}

		const descriptor_mapper& set_storage_image(
			const std::uint32_t binding,
			VkImageView imageView,
			const VkImageLayout imageLayout = VK_IMAGE_LAYOUT_GENERAL,
			const std::uint32_t chunkIndex = 0,
			const VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) const {

			VkDescriptorImageInfo imageInfo{
				.sampler = nullptr,
				.imageView = imageView,
				.imageLayout = imageLayout
			};
			descriptor_write_helper::write_image(
				buffer_obj->get_device(),
				get_ptr(binding, chunkIndex),
				buffer_obj.get(),
				imageInfo,
				descriptorType
			);
			return *this;
		}

		const descriptor_mapper& set_image(
			const std::uint32_t binding,
			const VkDescriptorImageInfo& imageInfo,
			const std::uint32_t chunkIndex = 0,
			const VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) const {

			assert(imageInfo.imageView != nullptr);
			descriptor_write_helper::write_image(
				buffer_obj->get_device(),
				get_ptr(binding, chunkIndex),
				buffer_obj.get(),
				imageInfo,
				descriptorType
			);
			return *this;
		}

		template <std::ranges::input_range Rng>
			requires (std::convertible_to<std::ranges::range_const_reference_t<Rng>, const VkDescriptorImageInfo&>)
		const descriptor_mapper& set_image(
			const std::uint32_t binding,
			const Rng& imageInfos,
			const std::uint32_t chunkIndex = 0,
			const VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) const {

			std::size_t currentOffset = 0;
			// 注意：这里需要知道 descriptor size 才能递增 offset，我们从 buffer_obj 获取一次
			auto [_, size] = buffer_obj->get_image_get_info({}, descriptorType); // Dummy call to get size

			for (const VkDescriptorImageInfo& imageInfo : imageInfos) {
				descriptor_write_helper::write_image(
					buffer_obj->get_device(),
					get_ptr(binding, chunkIndex, currentOffset),
					buffer_obj.get(),
					imageInfo,
					descriptorType
				);
				currentOffset += size;
			}
			return *this;
		}

		template <std::ranges::input_range Rng>
			requires (std::convertible_to<std::ranges::range_const_reference_t<Rng>, VkImageView>)
		const descriptor_mapper& set_image(
			const std::uint32_t binding,
			const VkImageLayout layout,
			VkSampler sampler,
			const Rng& imageViews,
			const std::uint32_t chunkIndex = 0,
			const VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM
		) const {
			std::size_t currentOffset = 0;
			// 确定类型
			VkDescriptorType realType = (type == VK_DESCRIPTOR_TYPE_MAX_ENUM) ?
				(sampler ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) : type;

			auto [_, size] = buffer_obj->get_image_get_info({}, realType);

			for (VkImageView imageView : imageViews) {
				VkDescriptorImageInfo image_info{
					.sampler = sampler,
					.imageView = imageView,
					.imageLayout = layout
				};
				descriptor_write_helper::write_image(
					buffer_obj->get_device(),
					get_ptr(binding, chunkIndex, currentOffset),
					buffer_obj.get(),
					image_info,
					realType
				);
				currentOffset += size;
			}
			return *this;
		}

		const descriptor_mapper& set_image(
			const std::uint32_t binding,
			VkImageView imageView,
			const std::uint32_t chunkIndex = 0,
			const VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VkSampler sampler = nullptr,
			const VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
		) const {
			const VkDescriptorImageInfo image_descriptor{
					.sampler = sampler,
					.imageView = imageView,
					.imageLayout = imageLayout
			};
			return set_image(binding, image_descriptor, chunkIndex, descriptorType);
		}
	};

	// =========================================================================
	// 动态 Mapper (新)
	// =========================================================================

	export struct dynamic_descriptor_mapper : buffer_mapper<dynamic_descriptor_buffer> {
		using buffer_mapper<dynamic_descriptor_buffer>::buffer_mapper;

	private:
		// 辅助：获取动态模式下的写入地址
		// offset = binding_base + (arrayIndex * stride)
		std::byte* get_ptr(std::uint32_t binding, std::size_t arrayIndex) const {
			auto* buf = buffer_obj.get();
			VkDeviceSize stride = buf->get_binding_stride(binding);
			// 可以在此添加对 capacity 的断言
			// assert(arrayIndex < buf->get_binding_capacity(binding));
			return mapped + buf->get_binding_offset(binding) + (arrayIndex * stride);
		}

	public:
		// 设置单个 UBO/SSBO 元素
		const dynamic_descriptor_mapper& set_element_at(
			std::uint32_t binding,
			std::size_t arrayIndex,
			VkDeviceAddress addr,
			VkDeviceSize size,
			VkDescriptorType type
		) const {
			// 推断大小
			std::size_t desc_size = (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) ?
				buffer_obj->get_uniform_buffer_descriptor_size() : buffer_obj->get_storage_buffer_descriptor_size();

			descriptor_write_helper::write_buffer(
				buffer_obj->get_device(),
				get_ptr(binding, arrayIndex),
				desc_size,
				type,
				addr, size, VK_FORMAT_UNDEFINED
			);
			return *this;
		}

		// 设置单个 Image 元素
		const dynamic_descriptor_mapper& set_element_at(
			std::uint32_t binding,
			std::size_t arrayIndex,
			const VkDescriptorImageInfo& imageInfo,
			VkDescriptorType type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
		) const {
			descriptor_write_helper::write_image(
				buffer_obj->get_device(),
				get_ptr(binding, arrayIndex),
				buffer_obj.get(),
				imageInfo,
				type
			);
			return *this;
		}

		template <std::ranges::input_range Rng>
			requires (std::convertible_to<std::ranges::range_const_reference_t<Rng>, const VkDescriptorImageInfo&>)
		const dynamic_descriptor_mapper& set_images_at(
			std::uint32_t binding,
			std::size_t startIndex,
			const Rng& imageInfos,
			VkDescriptorType type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
		) const {
			auto* buf = buffer_obj.get();
			VkDeviceSize stride = buf->get_binding_stride(binding);

			// 获取起始位置指针
			std::byte* current_ptr = get_ptr(binding, startIndex);

			for (const VkDescriptorImageInfo& imageInfo : imageInfos) {
				descriptor_write_helper::write_image(
					buf->get_device(),
					current_ptr,
					buf,
					imageInfo,
					type
				);
				// 动态 Buffer 中，下一个元素的地址 = 当前地址 + Binding Stride
				current_ptr += stride;
			}
			return *this;
		}

		// 2. 传入 VkImageView 的 Range (共享 Layout 和 Sampler)
		template <std::ranges::input_range Rng>
			requires (std::convertible_to<std::ranges::range_const_reference_t<Rng>, VkImageView>)
		const dynamic_descriptor_mapper& set_images_at(
			std::uint32_t binding,
			std::size_t startIndex,
			const VkImageLayout layout,
			VkSampler sampler,
			const Rng& imageViews,
			const VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM
		) const {
			// 自动推导类型
			VkDescriptorType realType = (type == VK_DESCRIPTOR_TYPE_MAX_ENUM) ?
				(sampler ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) : type;

			auto* buf = buffer_obj.get();
			VkDeviceSize stride = buf->get_binding_stride(binding);

			// 获取起始位置指针
			std::byte* current_ptr = get_ptr(binding, startIndex);

			for (VkImageView imageView : imageViews) {
				VkDescriptorImageInfo image_info{
					.sampler = sampler,
					.imageView = imageView,
					.imageLayout = layout
				};

				descriptor_write_helper::write_image(
					buf->get_device(),
					current_ptr,
					buf,
					image_info,
					realType
				);

				current_ptr += stride;
			}
			return *this;
		}
	};
}