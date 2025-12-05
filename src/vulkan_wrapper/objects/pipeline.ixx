module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk:pipeline;

import mo_yanxi.handle_wrapper;
import mo_yanxi.vk.util;
import :shader;
import std;

namespace mo_yanxi::vk{
	export namespace blending{
		constexpr auto default_mask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		constexpr VkPipelineColorBlendAttachmentState overwrite{
				.blendEnable = false,
				.colorWriteMask = default_mask
			};

		constexpr VkPipelineColorBlendAttachmentState disable = overwrite;

		constexpr VkPipelineColorBlendAttachmentState discard{.colorWriteMask = 0};

		constexpr VkPipelineColorBlendAttachmentState scaled_alpha_blend{
				.blendEnable = true,

				.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
				.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				.colorBlendOp = VK_BLEND_OP_ADD,
				.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
				.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				.alphaBlendOp = VK_BLEND_OP_ADD,

				.colorWriteMask = default_mask
			};

		constexpr VkPipelineColorBlendAttachmentState alpha_blend{
				.blendEnable = true,

				.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
				.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				.colorBlendOp = VK_BLEND_OP_ADD,
				.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
				.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				.alphaBlendOp = VK_BLEND_OP_ADD,

				.colorWriteMask = default_mask
			};

		constexpr VkPipelineColorBlendAttachmentState add{
				.blendEnable = true,

				.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
				.dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
				.colorBlendOp = VK_BLEND_OP_ADD,
				.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
				.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				.alphaBlendOp = VK_BLEND_OP_ADD,

				.colorWriteMask = default_mask
			};

		constexpr VkPipelineColorBlendAttachmentState reverse{
				.blendEnable = true,

				.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
				.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				.colorBlendOp = VK_BLEND_OP_ADD,
				.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
				.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				.alphaBlendOp = VK_BLEND_OP_ADD,

				.colorWriteMask = default_mask
			};

		constexpr VkPipelineColorBlendAttachmentState lock_alpha{
				.blendEnable = true,

				.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
				.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				.colorBlendOp = VK_BLEND_OP_ADD,
				.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
				.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
				.alphaBlendOp = VK_BLEND_OP_ADD,

				.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
			};
	}

	export
	struct graphic_pipeline_template{
	protected:
		VkFormat depth_format = VK_FORMAT_UNDEFINED;
		VkFormat stencil_format = VK_FORMAT_UNDEFINED;

		shader_chain shaderChain{};
		std::vector<VkDynamicState> dynamicStates{};

		std::optional<VkPipelineVertexInputStateCreateInfo> vertexInputInfo{VkPipelineVertexInputStateCreateInfo{
				.sType =  VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
			}};

		VkPipelineDepthStencilStateCreateInfo depthStencilState{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.depthTestEnable = false,
				.depthWriteEnable = false,
				.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
				.depthBoundsTestEnable = true,
				.stencilTestEnable = false,
				.front = {},
				.back = {},
				.minDepthBounds = 0.,
				.maxDepthBounds = 1.
			};

		VkPipelineMultisampleStateCreateInfo multisampleState{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
				.sampleShadingEnable = false,
				.minSampleShading = 1.0f,
				.pSampleMask = nullptr,
				.alphaToCoverageEnable = false,
				.alphaToOneEnable = false
			};

		VkPipelineInputAssemblyStateCreateInfo assemblyInfo{
				.sType =  VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				.primitiveRestartEnable = false
			};

		VkPipelineRasterizationStateCreateInfo rasterizationInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,

				.depthClampEnable = false,
				.rasterizerDiscardEnable = false,
				.polygonMode = VK_POLYGON_MODE_FILL,
				.cullMode = VK_CULL_MODE_NONE, //Never cull for 2D draw
				.frontFace = VK_FRONT_FACE_CLOCKWISE,

				.depthBiasEnable = false,
				.depthBiasConstantFactor = .0f,
				.depthBiasClamp = .0f,
				.depthBiasSlopeFactor = .0f,
				.lineWidth = 1.f
			};

		std::optional<VkViewport> staticViewport{};
		std::optional<VkRect2D> staticScissor{};

		struct color_attachment_info{
			VkFormat format;
			VkPipelineColorBlendAttachmentState blend;
		};
		std::vector<color_attachment_info> color_attachments{};

	public:
		void set_blending_dynamic(){
			dynamicStates.push_back(VK_DYNAMIC_STATE_COLOR_BLEND_ENABLE_EXT);
			dynamicStates.push_back(VK_DYNAMIC_STATE_COLOR_BLEND_EQUATION_EXT);
		}

		graphic_pipeline_template& set_multisample(
			const VkSampleCountFlagBits bits,
			const float minShading = 1.f,
			bool perPixel = false){
			multisampleState.rasterizationSamples = bits;
			multisampleState.sampleShadingEnable = perPixel;
			multisampleState.minSampleShading = minShading;
			// multisampleState.alphaToCoverageEnable = true;
			return *this;
		}


		template <
			contigious_range_of<VkVertexInputBindingDescription> BindDescTy = std::initializer_list<VkVertexInputBindingDescription>,
			contigious_range_of<VkVertexInputAttributeDescription> AttrDescTy = std::initializer_list<VkVertexInputAttributeDescription>
		>
		graphic_pipeline_template& set_vertex_info(
			const BindDescTy& binding_desc,
			const AttrDescTy& attribute_desc
		){
			vertexInputInfo.emplace();
			vertexInputInfo->vertexBindingDescriptionCount = static_cast<std::uint32_t>(std::ranges::size(binding_desc)),
			vertexInputInfo->pVertexBindingDescriptions = std::ranges::data(binding_desc),
			vertexInputInfo->vertexAttributeDescriptionCount = static_cast<std::uint32_t>(std::ranges::size(attribute_desc)),
			vertexInputInfo->pVertexAttributeDescriptions = std::ranges::data(attribute_desc);

			return *this;
		}

		graphic_pipeline_template& set_vertex_info(
			const VkPipelineVertexInputStateCreateInfo& info
		){
			vertexInputInfo = info;
			return *this;
		}

		graphic_pipeline_template& set_depth_state(const bool enableTest = true, const bool enableWrite = true){
			depthStencilState.depthTestEnable = enableTest;
			depthStencilState.depthWriteEnable = enableWrite;
			return *this;
		}

		graphic_pipeline_template& set_depth_format(const VkFormat format = VK_FORMAT_D24_UNORM_S8_UINT) noexcept{
			depth_format = format;
			return *this;
		}

		graphic_pipeline_template& set_stencil_format(const VkFormat format = VK_FORMAT_D24_UNORM_S8_UINT) noexcept{
			stencil_format = format;
			return *this;
		}

		graphic_pipeline_template& push_color_attachment_format(
			const VkFormat format,
			const VkPipelineColorBlendAttachmentState& blend = blending::alpha_blend
			){
			color_attachments.emplace_back(format, blend);
			return *this;
		}

		graphic_pipeline_template& push_color_attachment_format(
			const VkFormat format,
			const VkPipelineColorBlendAttachmentState& blend,
			std::size_t count){
			for(const auto& [r, b] : std::ranges::views::repeat(color_attachment_info{format, blend}, count)){
				push_color_attachment_format(r, b);
			}
			return *this;
		}

		graphic_pipeline_template& set_viewport(
			const VkViewport& viewport){
			this->staticViewport = viewport;
			return *this;
		}

		graphic_pipeline_template& set_scissor(
			const VkRect2D& scissor){
			this->staticScissor = scissor;
			return *this;
		}

		graphic_pipeline_template& set_shaders(
			const shader_chain& chain){
			this->shaderChain = chain;
			return *this;
		}

		VkPipeline create(
			VkDevice device,
			VkPipelineLayout layout,
			const VkPipelineCreateFlags flags,
			std::array<float, 4> blend_constants = {}
		) const{

			std::vector attachmentFormats{std::from_range, color_attachments | std::views::transform(&color_attachment_info::format)};
			std::vector attachmentBlends{std::from_range, color_attachments | std::views::transform(&color_attachment_info::blend)};

			VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
					.pNext = nullptr,
					.viewMask = 0,
					.colorAttachmentCount = static_cast<std::uint32_t>(attachmentFormats.size()),
					.pColorAttachmentFormats = attachmentFormats.data(),
					.depthAttachmentFormat = depth_format,
					.stencilAttachmentFormat = stencil_format
				};

			std::vector tempDynamicStates{dynamicStates};
			if(!staticViewport){
				tempDynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
			}

			if(!staticScissor){
				tempDynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
			}

			VkPipelineDynamicStateCreateInfo dynamicInfo{
					.sType =  VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.dynamicStateCount = static_cast<std::uint32_t>(tempDynamicStates.size()),
					.pDynamicStates = tempDynamicStates.data()
				};

			VkPipelineColorBlendStateCreateInfo blendInfo{
					.sType =  VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.logicOpEnable = false,
					.logicOp = VK_LOGIC_OP_COPY,
					.attachmentCount = static_cast<std::uint32_t>(attachmentBlends.size()),
					.pAttachments = attachmentBlends.data(),
					.blendConstants = {blend_constants[0], blend_constants[1], blend_constants[2], blend_constants[3]}
				};

			VkRect2D scissor = staticScissor.value_or(VkRect2D{});
			VkViewport viewport = staticViewport.value_or(VkViewport{});
			VkPipelineViewportStateCreateInfo viewportInfo{
					.sType =   VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.viewportCount = 1,
					.pViewports = staticViewport ? &viewport : nullptr,
					.scissorCount = 1,
					.pScissors = staticScissor ? &scissor : nullptr
				};

			VkPipelineTessellationStateCreateInfo tessellation_state_create_info{
					.sType =    VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.patchControlPoints = 0
				};

			VkGraphicsPipelineCreateInfo createInfo{
					.sType =  VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
					.pNext = &pipelineRenderingCreateInfo,
					.flags = flags,
					.stageCount = shaderChain.size(),
					.pStages = shaderChain.data(),
					.pVertexInputState = vertexInputInfo ? &*vertexInputInfo : nullptr,
					.pInputAssemblyState = &assemblyInfo,
					.pTessellationState = &tessellation_state_create_info,
					.pViewportState = &viewportInfo,
					.pRasterizationState = &rasterizationInfo,
					.pMultisampleState = &multisampleState,
					.pDepthStencilState = &depthStencilState,
					.pColorBlendState = &blendInfo,
					.pDynamicState = &dynamicInfo,
					.layout = layout,
					.renderPass = nullptr,
					.subpass = 0,
					.basePipelineHandle = nullptr,
					.basePipelineIndex = 0
				};

			VkPipeline pipeline;
			if(const auto rst = vkCreateGraphicsPipelines(device, nullptr, 1, &createInfo, nullptr, &pipeline)){
				throw vk_error{rst, "Failed to create pipeline"};
			}
			return pipeline;
		}
	};

	export
	struct pipeline : exclusive_handle<VkPipeline>{
	private:
		exclusive_handle_member<VkDevice> device{};

	public:

		[[nodiscard]] pipeline() = default;

		[[nodiscard]] pipeline(VkDevice device, VkPipeline pipeline) : wrapper{pipeline}, device{device}{}

		[[nodiscard]] pipeline(
			VkDevice device,
			VkPipelineLayout layout,
			const VkPipelineCreateFlags flags,
			const graphic_pipeline_template& pipelineTemplate,
			std::array<float, 4> blend_constants = {}) : device{device}{
			handle = pipelineTemplate.create(device, layout, flags, blend_constants);
		}

		[[nodiscard]] pipeline(
			VkDevice device,
			VkPipelineLayout layout,
			const VkPipelineCreateFlags flags,
			const VkPipelineShaderStageCreateInfo& stageInfo
			) : device{device}{

			VkComputePipelineCreateInfo createInfo{
				.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
				.pNext = nullptr,
				.flags = flags,
				.stage = stageInfo,
				.layout = layout,
				.basePipelineHandle = nullptr,
				.basePipelineIndex = 0
			};

			if(const auto rst = vkCreateComputePipelines(device, nullptr, 1, &createInfo, nullptr, &handle)){
				throw vk_error(rst, "Failed to create compute pipeline");
			}
		}

		void bind(VkCommandBuffer commandBuffer, const VkPipelineBindPoint bindPoint) const noexcept{
			vkCmdBindPipeline(commandBuffer, bindPoint, handle);
		}

		~pipeline(){
			if(device)vkDestroyPipeline(device, handle, nullptr);
		}

		[[nodiscard]] VkDevice get_device() const noexcept{
			return device;
		}

		pipeline(const pipeline& other) = delete;

		pipeline(pipeline&& other) noexcept = default;

		pipeline& operator=(const pipeline& other) = delete;

		pipeline& operator=(pipeline&& other) noexcept = default;
	};
}
