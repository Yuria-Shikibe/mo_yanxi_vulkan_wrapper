module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk:pipeline_layout;

import mo_yanxi.handle_wrapper;
import mo_yanxi.vk.util;
import std;

namespace mo_yanxi::vk{
	export
	class descriptor_layout_builder{
		struct Result{
			std::vector<VkDescriptorSetLayoutBinding> bindings{};
			std::vector<VkDescriptorBindingFlags> flags{};
		};

		struct Binding{
			VkDescriptorSetLayoutBinding binding;
			VkDescriptorBindingFlags flag;
		};

		struct VkDescriptorSetLayoutBinding_Mapper{
			constexpr bool operator()(const Binding& lhs,
			                          const Binding& rhs) const noexcept{
				return lhs.binding.binding < rhs.binding.binding;
			}
		};

		std::set<Binding, VkDescriptorSetLayoutBinding_Mapper> bindings{};

	public:
		[[nodiscard]] std::uint32_t size() const noexcept{
			return static_cast<std::uint32_t>(bindings.size());
		}

		void push(const Binding& binding){
			const auto [rst, success] = bindings.insert(binding);

			if(!success){
				std::println(std::cerr, "Conflicted Desc Layout At[{}]", binding.binding.binding);
				throw std::runtime_error("Conflicted Layout Binding");
			}
		}

		void push(
			const std::uint32_t index,
			const VkDescriptorType type,
			const VkShaderStageFlags stageFlags,
		    const std::uint32_t count = 1,
		    const VkDescriptorBindingFlags flag = 0
		){
			push({{
					.binding = index,
					.descriptorType = type,
					.descriptorCount = count,
					.stageFlags = stageFlags,
					.pImmutableSamplers = nullptr
				}, flag});
		}

		template <std::size_t size>
		void push(
			const std::uint32_t index,
			const VkDescriptorType type,
			const VkShaderStageFlags stageFlags,
			const std::uint32_t count,
			/* do not accept rvalue */ std::array<VkSampler, size>& immutableSamplers,
			const VkDescriptorBindingFlags flag = 0
			){
			this->push({{
					.binding = index,
					.descriptorType = type,
					.descriptorCount = count,
					.stageFlags = stageFlags,
					.pImmutableSamplers = size == 0 ? nullptr : immutableSamplers.data()
				}, flag});
		}

		void push_seq(const VkDescriptorType type, const VkShaderStageFlags stageFlags, const std::uint32_t count = 1, const VkDescriptorBindingFlags flag = 0){
			push(bindings.size(), type, stageFlags, count, flag);
		}

		template <std::size_t size>
		void push_seq(const VkDescriptorType type, const VkShaderStageFlags stageFlags, const std::uint32_t count,
		              std::array<VkSampler, size>& immutableSamplers){
			this->push(bindings.size(), type, stageFlags, count, immutableSamplers);
		}

		[[nodiscard]] Result export_bindings() const{
			Result result{
				bindings | std::views::transform(&Binding::binding) | std::ranges::to<std::vector>(),
				bindings | std::views::transform(&Binding::flag) | std::ranges::to<std::vector>(),
			};
			return result;
		}
	};

	export
	class descriptor_layout : public exclusive_handle<VkDescriptorSetLayout>{
		exclusive_handle_member<VkDevice> device{};
		std::size_t bindings{};

	public:
		[[nodiscard]] descriptor_layout() = default;

		~descriptor_layout(){
			if(device) vkDestroyDescriptorSetLayout(device, handle, nullptr);
		}

		descriptor_layout(const descriptor_layout& other) = delete;
		descriptor_layout(descriptor_layout&& other) noexcept = default;
		descriptor_layout& operator=(const descriptor_layout& other) = delete;
		descriptor_layout& operator=(descriptor_layout&& other) noexcept = default;

		descriptor_layout(
			VkDevice device,
			const VkDescriptorSetLayoutCreateFlags flag,
			const descriptor_layout_builder& builder
		) : device{device}{
			create(flag, builder);
		}

		descriptor_layout(
			VkDevice device,
			const VkDescriptorSetLayoutCreateFlags flag,
			std::regular_invocable<descriptor_layout&, descriptor_layout_builder&> auto func
		) : device{device}{
			descriptor_layout_builder builder{};
			func(*this, builder);

			create(flag, builder);
		}

		descriptor_layout(
			VkDevice device,
			const VkDescriptorSetLayoutCreateFlags flag,
			std::regular_invocable<descriptor_layout_builder&> auto func
		) : descriptor_layout{device, flag, [f = std::move(func)](descriptor_layout&, descriptor_layout_builder& b){
			std::invoke(f, b);
		}}{}

		descriptor_layout(
			VkDevice device,
			std::regular_invocable<descriptor_layout_builder&> auto func
		) : descriptor_layout{device, VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, std::move(func)}{}

		[[nodiscard]] std::uint32_t binding_count() const noexcept{
			return bindings;
		}

		[[nodiscard]] VkDevice get_device() const noexcept{
			return device;
		}

	private:
		void create(VkDescriptorSetLayoutCreateFlags flag, const descriptor_layout_builder& builder){
			const auto builder_rst = builder.export_bindings();

			if(!builder_rst.bindings.empty()){
				bindings = builder_rst.bindings.back().binding + 1;
			}

			VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo{
				VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO
			};

			bindingFlagsCreateInfo.bindingCount = builder_rst.flags.size();
			bindingFlagsCreateInfo.pBindingFlags = builder_rst.flags.data();


			const VkDescriptorSetLayoutCreateInfo layoutInfo{
					.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
					.pNext = &bindingFlagsCreateInfo,
					.flags = flag,
					.bindingCount = static_cast<std::uint32_t>(builder_rst.bindings.size()),
					.pBindings = builder_rst.bindings.data()
				};

			if(const auto rst = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &handle)){
				throw vk_error(rst, "Failed to create descriptor set layout!");
			}
		}
	};

	export
	struct constant_layout{
	private:
		std::vector<VkPushConstantRange> constants{};

	public:
		void push(const VkPushConstantRange& constantRange){
			constants.push_back(constantRange);
		}


		[[nodiscard]] std::span<const VkPushConstantRange> get_constant_ranges() const noexcept{
			return constants;
		}

	};

	export
	struct pipeline_layout : exclusive_handle<VkPipelineLayout>{
	private:
		exclusive_handle_member<VkDevice> device{};

	public:
		[[nodiscard]] pipeline_layout() = default;

		pipeline_layout(const pipeline_layout& other) = delete;

		pipeline_layout(pipeline_layout&& other) noexcept = default;

		pipeline_layout& operator=(const pipeline_layout& other) = delete;

		pipeline_layout& operator=(pipeline_layout&& other) noexcept = default;

		~pipeline_layout(){
			if(device)vkDestroyPipelineLayout(device, handle, nullptr);
		}

		[[nodiscard]] VkDevice get_device() const noexcept{
			return device;
		}

		template<
			contigious_range_of<const VkDescriptorSetLayout> R1 = std::initializer_list<const VkDescriptorSetLayout>,
			contigious_range_of<const VkPushConstantRange> R2 = std::initializer_list<const VkPushConstantRange>>
		[[nodiscard]] explicit(false) pipeline_layout(
			VkDevice device,
			const VkPipelineLayoutCreateFlags flags = 0,
			const R1& descriptorSetLayouts = {},
			const R2& constantRange = {}) : device{device}{

			VkPipelineLayoutCreateInfo pipelineLayoutInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
				.flags = flags,
				.setLayoutCount = static_cast<std::uint32_t>(std::ranges::size(descriptorSetLayouts)),
				.pSetLayouts = std::ranges::data(descriptorSetLayouts),
				.pushConstantRangeCount = static_cast<std::uint32_t>(std::ranges::size(constantRange)),
				.pPushConstantRanges = std::ranges::data(constantRange)
			};

			if(const auto rst = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &handle)){
				throw vk_error(rst, "Failed to create pipeline layout!");
			}
		}
	};
}
