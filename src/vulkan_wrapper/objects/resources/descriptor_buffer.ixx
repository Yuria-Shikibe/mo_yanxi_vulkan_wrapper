module;

#include <vulkan/vulkan.h>
#include <cassert>

export module mo_yanxi.vk:descriptor_buffer;

import std;
import mo_yanxi.vk.util;
import mo_yanxi.handle_wrapper;
import :resources;
import :ext;


//TODO more compact dbo using descriptorBufferOffsetAlignment
namespace mo_yanxi::vk{
	export struct descriptor_mapper;

	export
	struct descriptor_buffer : buffer{
	private:
		friend descriptor_mapper;
		std::vector<VkDeviceSize> offsets{};
		std::uint32_t chunk_count{};

		std::size_t uniformBufferDescriptorSize{};
		std::size_t combinedImageSamplerDescriptorSize{};
		std::size_t storageImageDescriptorSize{};
		std::size_t inputAttachmentDescriptorSize{};
		std::size_t storageBufferDescriptorSize{};

	public:
		[[nodiscard]] descriptor_buffer() = default;

		[[nodiscard]] descriptor_buffer(
			const allocator_usage allocator,
			VkDescriptorSetLayout layout,
			std::uint32_t bindings,
			std::uint32_t chunk_count = 1
		) : chunk_count{chunk_count}{

			const auto device = allocator.get_device();
			const auto physical_device = allocator.get_physical_device();

			VkDeviceSize dbo_size;
			getDescriptorSetLayoutSizeEXT(device, layout, &dbo_size);

			this->buffer::operator=(buffer{allocator, {
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = dbo_size * chunk_count,
				.usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			}, {
				.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			}});


			{
				VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptor_buffer_properties{
					.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT,
					.pNext = nullptr,
				};

				VkPhysicalDeviceProperties2KHR device_properties{
					.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR,
					.pNext = &descriptor_buffer_properties,
				};

				getPhysicalDeviceProperties2KHR(physical_device, &device_properties);

				auto offset_align = descriptor_buffer_properties.descriptorBufferOffsetAlignment;
				uniformBufferDescriptorSize = descriptor_buffer_properties.uniformBufferDescriptorSize;
				combinedImageSamplerDescriptorSize = descriptor_buffer_properties.combinedImageSamplerDescriptorSize;
				storageImageDescriptorSize = descriptor_buffer_properties.storageImageDescriptorSize;
				inputAttachmentDescriptorSize = descriptor_buffer_properties.inputAttachmentDescriptorSize;
				storageBufferDescriptorSize = descriptor_buffer_properties.storageBufferDescriptorSize;
			}


			offsets.resize(bindings);
			// Get descriptor bindings offsets as descriptors are placed inside set layout by those offsets.
			for(auto&& [i, offset] : offsets | std::views::enumerate){
				getDescriptorSetLayoutBindingOffsetEXT(device, layout, static_cast<std::uint32_t>(i), offsets.data() + i);
			}
		}

		[[nodiscard]] std::size_t get_uniform_buffer_descriptor_size() const noexcept{
			return uniformBufferDescriptorSize;
		}

		[[nodiscard]] std::size_t get_combined_image_sampler_descriptor_size() const noexcept{
			return combinedImageSamplerDescriptorSize;
		}

		[[nodiscard]] std::size_t get_storage_image_descriptor_size() const noexcept{
			return storageImageDescriptorSize;
		}

		[[nodiscard]] std::size_t get_storage_buffer_descriptor_size() const noexcept{
			return storageBufferDescriptorSize;
		}

		[[nodiscard]] std::size_t get_input_attachment_descriptor_size() const noexcept{
			return inputAttachmentDescriptorSize;
		}

		explicit(false) operator VkDescriptorBufferBindingInfoEXT() const noexcept{
			return get_bind_info(VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_2_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT);
		}

		[[nodiscard]] VkDescriptorBufferBindingInfoEXT get_bind_info(const VkBufferUsageFlags usage) const{
			return {
					.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
					.pNext = nullptr,
					.address = get_address(),
					.usage = usage
				};
		}

		[[nodiscard]] constexpr VkDeviceSize get_chunk_offset(const std::uint32_t chunkIndex) const noexcept{
			assert(chunkIndex < chunk_count);
			return  chunkIndex * get_chunk_size();
		}

		[[nodiscard]] constexpr VkDeviceSize get_chunk_size() const noexcept{
			return get_size() / chunk_count;
		}

		[[nodiscard]] constexpr std::uint32_t get_chunk_count() const noexcept{
			return chunk_count;
		}

	private:
		void bind_to(
			const VkCommandBuffer commandBuffer,
			const VkBufferUsageFlags usage) const{
			const auto info = get_bind_info(usage);
			cmd::bindDescriptorBuffersEXT(commandBuffer, 1, &info);
		}
	public:

		void bind_to(
			VkCommandBuffer commandBuffer,
			const VkPipelineBindPoint bindingPoint,
			VkPipelineLayout layout,
			const std::uint32_t setIndex,

			const VkDeviceSize offset = 0,
			const VkBufferUsageFlags usage = VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_2_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
		) const{
			static constexpr std::uint32_t ZERO = 0;
			bind_to(commandBuffer, usage);
			cmd::setDescriptorBufferOffsetsEXT(
				commandBuffer,
				bindingPoint,
				layout,
				setIndex, 1, &ZERO, &offset);
		}

		void bind_chunk_to(
			VkCommandBuffer commandBuffer,

			const VkPipelineBindPoint bindingPoint,
			VkPipelineLayout layout,
			const std::uint32_t setIndex,

			const std::uint32_t chunkIndex = 0,
			const VkBufferUsageFlags usage = VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_2_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
		) const{
			static constexpr std::uint32_t ZERO = 0;
			bind_to(commandBuffer, usage);

			const VkDeviceSize offset = get_chunk_offset(chunkIndex);

			cmd::setDescriptorBufferOffsetsEXT(
				commandBuffer,
				bindingPoint,
				layout,
				setIndex, 1, &ZERO, &offset);
		}

		[[nodiscard]] VkDescriptorBufferBindingInfoEXT get_info(VkBufferUsageFlags usage) const noexcept{
			return {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
				.pNext = nullptr,
				.address = get_address(),
				.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | usage
			};
		}
	private:
		std::pair<VkDescriptorGetInfoEXT, VkDeviceSize> get_image_info(
			VkDescriptorImageInfo&& dangling, const VkDescriptorType descriptorType) const = delete;

		[[nodiscard]] std::pair<VkDescriptorGetInfoEXT, VkDeviceSize> get_image_info(
			const VkDescriptorImageInfo& imageInfo_non_rv, const VkDescriptorType descriptorType) const{
			std::pair<VkDescriptorGetInfoEXT, VkDeviceSize> rst{
					VkDescriptorGetInfoEXT{
						.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
						.pNext = nullptr,
						.type = descriptorType,
						.data = {}
					},
					{}
				};

			switch(descriptorType){
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER :{
				rst.first.data.pCombinedImageSampler = &imageInfo_non_rv;
				rst.second = combinedImageSamplerDescriptorSize;
				break;
			}

			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE :{
				rst.first.data.pStorageImage = &imageInfo_non_rv;
				rst.second = storageImageDescriptorSize;
				break;
			}

			case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT :{
				rst.first.data.pInputAttachmentImage = &imageInfo_non_rv;
				rst.second = inputAttachmentDescriptorSize;
				break;
			}

			default : throw std::runtime_error("Invalid descriptor type!");
			}

			return rst;
		}
	};

	struct descriptor_mapper : buffer_mapper<descriptor_buffer>{
		// using buffer_mapper::buffer_mapper;

		const descriptor_mapper& set_storage_buffer(
			const std::uint32_t binding,
			const VkDeviceAddress address,
			const VkDeviceSize size,
			const std::uint32_t chunkIndex = 0,
			const VkFormat format = VK_FORMAT_UNDEFINED
		) const{
			const VkDescriptorAddressInfoEXT addr_info{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
				.pNext = nullptr,
				.address = address,
				.range = size,
				.format = format
			};
			const VkDescriptorGetInfoEXT info{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
				.pNext = nullptr,
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.data = VkDescriptorDataEXT{
					.pStorageBuffer = &addr_info,
				}
			};

			vk::getDescriptorEXT(
				buffer_obj->get_device(),
				&info,
				buffer_obj->get_storage_buffer_descriptor_size(),
				mapped + buffer_obj->offsets[binding] + buffer_obj->get_chunk_offset(chunkIndex)
			);

			return *this;
		}

		const descriptor_mapper& set_uniform_buffer(
			const std::uint32_t binding,
			const buffer& ubo,
			const VkDeviceSize ubo_offset = 0,
			const std::uint32_t chunkIndex = 0
		) const{
			return this->set_uniform_buffer(binding, ubo.get_address() + ubo_offset, ubo.get_size(), chunkIndex);
		}



		const descriptor_mapper& set_uniform_buffer(
			const std::uint32_t binding,
			const VkDeviceAddress address,
			const VkDeviceSize size,
			const std::uint32_t chunkIndex = 0
		) const{
			const VkDescriptorAddressInfoEXT addr_info{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
				.pNext = nullptr,
				.address = address,
				.range = size,
				.format = VK_FORMAT_UNDEFINED
			};

			const VkDescriptorGetInfoEXT info{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
				.pNext = nullptr,
				.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.data = VkDescriptorDataEXT{
					.pUniformBuffer = &addr_info
				}
			};

			vk::getDescriptorEXT(
				buffer_obj->get_device(),
				&info,
				buffer_obj->get_uniform_buffer_descriptor_size(),
				mapped + buffer_obj->offsets[binding] + buffer_obj->get_chunk_offset(chunkIndex)
			);

			return *this;
		}

		const descriptor_mapper& set_uniform_buffer_segments(
			std::uint32_t initialBinding,
			const buffer& ubo,
			std::initializer_list<VkDeviceSize> segment_sizes,
			const std::uint32_t chunkIndex = 0) const{

			VkDeviceSize currentOffset{};

			for (const auto& [idx, sz] : segment_sizes | std::views::enumerate){
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
					const VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) const{

			VkDescriptorImageInfo imageInfo{
				.sampler = nullptr,
				.imageView = imageView,
				.imageLayout = imageLayout
			};

			auto [info, size] = buffer_obj->get_image_info(imageInfo, descriptorType);

			vk::getDescriptorEXT(
				buffer_obj->get_device(),
				&info,
				size,
				mapped + buffer_obj->offsets[binding] + buffer_obj->get_chunk_offset(chunkIndex)
			);

			return *this;
		}

		const descriptor_mapper& set_image(
			const std::uint32_t binding,
			const VkDescriptorImageInfo& imageInfo,
			const std::uint32_t chunkIndex = 0,
			const VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) const{
			auto [info, size] = buffer_obj->get_image_info(imageInfo, descriptorType);
			assert(imageInfo.imageView != nullptr);

			vk::getDescriptorEXT(
				buffer_obj->get_device(),
				&info,
				size,
				mapped + buffer_obj->offsets[binding] + buffer_obj->get_chunk_offset(chunkIndex)
			);

			return *this;
		}

		template <std::ranges::input_range Rng>
			requires (std::convertible_to<std::ranges::range_const_reference_t<Rng>, const VkDescriptorImageInfo&>)
		const descriptor_mapper& set_image(
			const std::uint32_t binding,
			const Rng& imageInfos,
			const std::uint32_t chunkIndex = 0,
			const VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) const{
			std::size_t currentOffset = 0;

			for(const VkDescriptorImageInfo& imageInfo : imageInfos){
				auto [info, size] = buffer_obj->get_image_info(imageInfo, descriptorType);

				vk::getDescriptorEXT(
					buffer_obj->get_device(),
					&info,
					size,
					mapped + buffer_obj->offsets[binding] + currentOffset + buffer_obj->get_chunk_offset(chunkIndex)
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
			) const{
			std::size_t currentOffset = 0;

			for(VkImageView imageInfo : imageViews){
				VkDescriptorImageInfo image_info{
					.sampler = sampler,
					.imageView = imageInfo,
					.imageLayout = layout
				};

				auto [info, size] = buffer_obj->get_image_info(
					image_info, type == VK_DESCRIPTOR_TYPE_MAX_ENUM ? sampler ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : type);

				vk::getDescriptorEXT(
					buffer_obj->get_device(),
					&info,
					size,
					mapped + buffer_obj->offsets[binding] + currentOffset + buffer_obj->get_chunk_offset(chunkIndex)
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
		) const{
			const VkDescriptorImageInfo image_descriptor{
					.sampler = sampler,
					.imageView = imageView,
					.imageLayout = imageLayout
				};

			return set_image(binding, image_descriptor, chunkIndex, descriptorType);
		}

		// template <std::invocable<descriptor_buffer&> Writer>
		// void load(Writer&& writer){
		// 	map();
		// 	writer(*this);
		// 	unmap();
		// }

		// template <typename... Args>
		// 	requires (std::same_as<UniformBuffer, std::remove_cvref_t<Args>> && ...)
		// void loadUniformBuffer(std::uint32_t initialBinding, const Args&... Ubos){
		// 	map();
		//
		// 	[&, this]<std::size_t... Indices>(std::index_sequence<Indices...>){
		// 		((descriptor_buffer::mapUniformBuffer(initialBinding++, Ubos, 0)), ...);
		// 	}(std::index_sequence_for<Args...>{});
		//
		// 	unmap();
		// }
		//
	};
}
