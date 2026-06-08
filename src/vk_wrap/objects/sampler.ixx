module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk:sampler;

import std;
import mo_yanxi.handle_wrapper;
import mo_yanxi.vk.util;

namespace mo_yanxi::vk{

	namespace preset{
		export VkSamplerCreateInfo default_texture_sampler{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.magFilter = VK_FILTER_NEAREST,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
			.mipLodBias = 0,
			.anisotropyEnable = true,
			.maxAnisotropy = 4,
			.compareEnable = false,
			.compareOp = VK_COMPARE_OP_NEVER,
			.minLod = 0,
			.maxLod = VK_LOD_CLAMP_NONE,
			.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
			.unnormalizedCoordinates = false
		};

		export VkSamplerCreateInfo ui_texture_sampler{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
			.mipLodBias = 0,
			.anisotropyEnable = false,
			.maxAnisotropy = 0,
			.compareEnable = false,
			.compareOp = VK_COMPARE_OP_NEVER,
			.minLod = 0,
			.maxLod = VK_LOD_CLAMP_NONE,
			.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
			.unnormalizedCoordinates = false
		};

		export VkSamplerCreateInfo default_blit_sampler{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.mipLodBias = 0,
			.anisotropyEnable = false,
			.maxAnisotropy = 0,
			.compareEnable = false,
			.compareOp = VK_COMPARE_OP_NEVER,
			.minLod = 0,
			.maxLod = VK_LOD_CLAMP_NONE,
			.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
			.unnormalizedCoordinates = false
		};

		export VkSamplerCreateInfo default_nearest_sampler{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.magFilter = VK_FILTER_NEAREST,
			.minFilter = VK_FILTER_NEAREST,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.mipLodBias = 0,
			.anisotropyEnable = true,
			.maxAnisotropy = 4,
			.compareEnable = false,
			.compareOp = VK_COMPARE_OP_NEVER,
			.minLod = 0,
			.maxLod = VK_LOD_CLAMP_NONE,
			.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
			.unnormalizedCoordinates = false
		};
	}
	/*namespace SamplerInfo{
		constexpr Util::Component Filter_Linear{
				VkSamplerCreateInfo{
					.magFilter = VK_FILTER_LINEAR,
					.minFilter = VK_FILTER_LINEAR
				},
				&VkSamplerCreateInfo::magFilter, &VkSamplerCreateInfo::minFilter
			};

	    constexpr Util::Component Filter_PIXEL_Linear{
				VkSamplerCreateInfo{
					.magFilter = VK_FILTER_NEAREST,
					.minFilter = VK_FILTER_LINEAR
				},
				&VkSamplerCreateInfo::magFilter, &VkSamplerCreateInfo::minFilter
			};

		constexpr Util::Component Filter_Nearest{
				VkSamplerCreateInfo{
					.magFilter = VK_FILTER_NEAREST,
					.minFilter = VK_FILTER_NEAREST
				},
				&VkSamplerCreateInfo::magFilter, &VkSamplerCreateInfo::minFilter
			};

		constexpr Util::Component AddressMode_Repeat{
				VkSamplerCreateInfo{
					.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
					.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
					.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
				},
				&VkSamplerCreateInfo::addressModeU, &VkSamplerCreateInfo::addressModeV, &VkSamplerCreateInfo::addressModeW
			};

		constexpr Util::Component AddressMode_Clamp{
				VkSamplerCreateInfo{
					.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
					.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
					.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				},
				&VkSamplerCreateInfo::addressModeU, &VkSamplerCreateInfo::addressModeV, &VkSamplerCreateInfo::addressModeW
			};


		constexpr Util::Component LOD_Max{
				VkSamplerCreateInfo{
					.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
					.mipLodBias = 0,
					.minLod = 0,
					.maxLod = VK_LOD_CLAMP_NONE,
				},
				&VkSamplerCreateInfo::mipmapMode, &VkSamplerCreateInfo::minLod,
				&VkSamplerCreateInfo::maxLod, &VkSamplerCreateInfo::mipLodBias
			};

		constexpr Util::Component LOD_Max_Nearest{
				VkSamplerCreateInfo{
					.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
					.mipLodBias = 0,
					.minLod = 0,
					.maxLod = 5,
				},
				&VkSamplerCreateInfo::mipmapMode, &VkSamplerCreateInfo::minLod,
				&VkSamplerCreateInfo::maxLod, &VkSamplerCreateInfo::mipLodBias
			};

		constexpr Util::Component LOD_None{
				VkSamplerCreateInfo{
					.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
					.mipLodBias = 0,
					.minLod = 0,
					.maxLod = 0,
				},
				&VkSamplerCreateInfo::mipmapMode, &VkSamplerCreateInfo::minLod,
				&VkSamplerCreateInfo::maxLod, &VkSamplerCreateInfo::mipLodBias
			};


		//designators must appear in member declaration order of class
		template <VkCompareOp op = VK_COMPARE_OP_NEVER>
		constexpr Util::Component CompareOp{
				VkSamplerCreateInfo{
					.compareEnable = static_cast<bool>(op),
					.compareOp = op,
				},
				&VkSamplerCreateInfo::compareEnable, &VkSamplerCreateInfo::compareOp
			};

		template <std::uint32_t anisotropy = 4>
		constexpr Util::Component Anisotropy{
				VkSamplerCreateInfo{
					.anisotropyEnable = static_cast<bool>(anisotropy),
					.maxAnisotropy = anisotropy,
				},
				&VkSamplerCreateInfo::anisotropyEnable, &VkSamplerCreateInfo::maxAnisotropy
			};

		constexpr Util::Component Default{
				VkSamplerCreateInfo{
					.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
					.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
					.unnormalizedCoordinates = false
				},
				&VkSamplerCreateInfo::sType,
				&VkSamplerCreateInfo::borderColor,
				&VkSamplerCreateInfo::unnormalizedCoordinates
			};

		constexpr VkSamplerCreateInfo TextureSampler = SamplerInfo::Default
			| SamplerInfo::Filter_Linear
			| SamplerInfo::AddressMode_Clamp
			| SamplerInfo::LOD_Max
			| SamplerInfo::CompareOp<VK_COMPARE_OP_NEVER>
			| SamplerInfo::Anisotropy<4>;
	}


	}*/

	namespace Util{
		[[nodiscard]] constexpr VkDescriptorImageInfo
		getDescriptorInfo_ShaderRead(VkSampler handle, VkImageView imageView) noexcept{
			return {
				.sampler = handle,
				.imageView = imageView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};
		}

		template <typename... T>
			requires (std::convertible_to<T, VkImageView> && ...)
		[[nodiscard]] constexpr auto getDescriptorInfo_ShaderRead(VkSampler handle, const T&... imageViews) noexcept{
			std::array<VkDescriptorImageInfo, sizeof...(T)> rst{};

			[&]<std::size_t ... I>(std::index_sequence<I...>){
				((
					rst[I].imageView = imageViews,
					rst[I].sampler = handle,
					rst[I].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				), ...);
			}(std::index_sequence_for<T...>{});

			return rst;
		}

		VkDescriptorImageInfo getDescriptorInfo(VkSampler handle){
			return {
				.sampler = handle,
			};
		}
	}

	export
	class sampler : public exclusive_handle<VkSampler>{
		exclusive_handle_member<VkDevice> device{};

	public:
		[[nodiscard]] constexpr sampler() = default;

		[[nodiscard]] explicit sampler(VkDevice device, VkSampler textureSampler = nullptr)
			: wrapper{textureSampler}, device{device}{}

		[[nodiscard]] sampler(VkDevice device, const VkSamplerCreateInfo& samplerInfo)
			: device{device}{
			if(const auto rst = vkCreateSampler(device, &samplerInfo, nullptr, &handle)){
				throw vk_error(rst, "failed to create texture sampler!");
			}
		}

		~sampler(){
			if(device)vkDestroySampler(device, handle, nullptr);
		}

		sampler(const sampler& other) = delete;
		sampler(sampler&& other) noexcept = default;
		sampler& operator=(const sampler& other) = delete;
		sampler& operator=(sampler&& other) noexcept = default;

		[[nodiscard]] constexpr VkDescriptorImageInfo
			get_descriptor_info_shader_read(VkImageView imageView, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const noexcept{
			return {
					.sampler = handle,
					.imageView = imageView,
					.imageLayout = layout
				};
		}

		template <typename ...T>
			requires (std::convertible_to<T, VkImageView> && ...)
		[[nodiscard]] constexpr auto get_descriptor_info_shader_read(const T&... imageViews) const noexcept{
			return Util::getDescriptorInfo_ShaderRead(handle, imageViews...);
		}

		template <std::ranges::sized_range Rng>
			requires (std::convertible_to<std::ranges::range_value_t<Rng>, VkImageView>)
		[[nodiscard]] constexpr auto get_descriptor_info_shader_read(const Rng& imageViews) const noexcept{
			return Util::getDescriptorInfo_ShaderRead(handle, imageViews);

		}

		[[nodiscard]] VkDescriptorImageInfo get_descriptor_info() const{
			return Util::getDescriptorInfo(handle);
		}
	};

	export
	class sampler_vector{
		VkDevice device_{};
		std::vector<VkSampler> samplers_{};

	public:
		[[nodiscard]] sampler_vector() = default;

		template <std::ranges::input_range InputRange>
			requires std::convertible_to<std::ranges::range_reference_t<InputRange>, VkSamplerCreateInfo>
		[[nodiscard]] sampler_vector(VkDevice device, InputRange&& create_infos)
			: device_{device}{
			try{
				this->create_from_(std::forward<InputRange>(create_infos));
			} catch(...){
				reset();
				throw;
			}
		}

		[[nodiscard]] sampler_vector(VkDevice device, std::initializer_list<VkSamplerCreateInfo> create_infos)
			: sampler_vector(device, std::span{create_infos.begin(), create_infos.size()}){
		}

		~sampler_vector(){
			reset();
		}

		sampler_vector(const sampler_vector&) = delete;

		sampler_vector(sampler_vector&& other) noexcept
			: device_{std::exchange(other.device_, VK_NULL_HANDLE)}
			  , samplers_{std::move(other.samplers_)}{
		}

		sampler_vector& operator=(const sampler_vector&) = delete;

		sampler_vector& operator=(sampler_vector&& other) noexcept{
			if(this == &other){
				return *this;
			}
			reset();
			device_ = std::exchange(other.device_, VK_NULL_HANDLE);
			samplers_ = std::move(other.samplers_);
			return *this;
		}

		void reset() noexcept{
			destroy_all_();
			samplers_.clear();
			device_ = VK_NULL_HANDLE;
		}

		[[nodiscard]] VkDevice device() const noexcept{
			return device_;
		}

		[[nodiscard]] const VkSampler* data() const noexcept{
			return samplers_.data();
		}

		[[nodiscard]] std::size_t size() const noexcept{
			return samplers_.size();
		}

		[[nodiscard]] bool empty() const noexcept{
			return samplers_.empty();
		}

		[[nodiscard]] VkSampler operator[](std::size_t index) const noexcept{
			return samplers_[index];
		}

		[[nodiscard]] auto begin() const noexcept{
			return samplers_.begin();
		}

		[[nodiscard]] auto end() const noexcept{
			return samplers_.end();
		}

		[[nodiscard]] auto cbegin() const noexcept{
			return samplers_.cbegin();
		}

		[[nodiscard]] auto cend() const noexcept{
			return samplers_.cend();
		}

		[[nodiscard]] std::span<const VkSampler> span() const noexcept{
			return {samplers_.data(), samplers_.size()};
		}

		[[nodiscard]] std::span<const VkSampler> as_span() const noexcept{
			return span();
		}

		[[nodiscard]] explicit(false) operator std::span<const VkSampler>() const noexcept{
			return span();
		}

	private:
		template <std::ranges::input_range InputRange>
		void create_from_(InputRange&& create_infos){
			if constexpr(std::ranges::sized_range<InputRange>){
				samplers_.reserve(std::ranges::size(create_infos));
			}

			for(auto&& create_info_like : create_infos){
				if(device_ == VK_NULL_HANDLE){
					throw std::invalid_argument{"sampler_vector requires a non-null VkDevice"};
				}

				VkSamplerCreateInfo create_info = std::forward<decltype(create_info_like)>(create_info_like);
				samplers_.push_back(VK_NULL_HANDLE);
				if(const auto rst = vkCreateSampler(device_, &create_info, nullptr, &samplers_.back())){
					throw vk_error(rst, "failed to create sampler_vector sampler");
				}
			}
		}

		void destroy_all_() noexcept{
			if(device_ == VK_NULL_HANDLE){
				return;
			}

			for(auto& sampler : samplers_){
				if(sampler != VK_NULL_HANDLE){
					vkDestroySampler(device_, sampler, nullptr);
					sampler = VK_NULL_HANDLE;
				}
			}
		}
	};
}
