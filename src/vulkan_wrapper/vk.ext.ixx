module;

#include <vulkan/vulkan.h>
#include "vk_loader.hpp"

export module mo_yanxi.vk:ext;
import std;

namespace mo_yanxi::vk{
export inline std::add_pointer_t<decltype(vkGetPhysicalDeviceProperties2KHR)> getPhysicalDeviceProperties2KHR = nullptr;
export inline std::add_pointer_t<decltype(vkGetDescriptorSetLayoutSizeEXT)> getDescriptorSetLayoutSizeEXT = nullptr;
std::add_pointer_t<decltype(vkGetDescriptorSetLayoutBindingOffsetEXT)> _PFNgetDescriptorSetLayoutBindingOffsetEXT = nullptr;

std::add_pointer_t<decltype(vkGetDescriptorEXT)> getDescriptorEXT_ = nullptr;
std::add_pointer_t<decltype(vkCmdDrawMeshTasksEXT)> drawMeshTasksEXT = nullptr;
std::add_pointer_t<decltype(vkCmdDrawMeshTasksIndirectEXT)> drawMeshTasksIndirectEXT = nullptr;

std::add_pointer_t<decltype(vkGetDeviceFaultInfoEXT)> vkGetDeviceFaultInfoEXT = nullptr;

std::add_pointer_t<decltype(vkCmdBindDescriptorBuffersEXT)> PFN_cmdBindDescriptorBuffersEXT = nullptr;
std::add_pointer_t<decltype(vkCmdSetDescriptorBufferOffsetsEXT)> PFN_cmdSetDescriptorBufferOffsetsEXT = nullptr;
std::add_pointer_t<decltype(vkCmdSetDescriptorBufferOffsets2EXT)> PFN_cmdSetDescriptorBufferOffsets2EXT = nullptr;

std::add_pointer_t<decltype(vkCmdSetColorBlendEnableEXT)> _PFN_vkCmdSetColorBlendEnableEXT = nullptr;
std::add_pointer_t<decltype(vkCmdSetColorBlendEquationEXT)> _PFN_vkCmdSetColorBlendEquationEXT = nullptr;

export void getDescriptorSetLayoutBindingOffsetEXT(
	VkDevice
	device,
	VkDescriptorSetLayout                       layout,
	uint32_t                                    binding,
	VkDeviceSize*                               pOffset){
	_PFNgetDescriptorSetLayoutBindingOffsetEXT(device, layout, binding, pOffset);
}

export void getDescriptorEXT(
	VkDevice device,
	const VkDescriptorGetInfoEXT* pDescriptorInfo,
	size_t dataSize,
	void* pDescriptor){
	assert(pDescriptorInfo != nullptr);
	assert(pDescriptor != nullptr);
	getDescriptorEXT_(device, pDescriptorInfo, dataSize, pDescriptor);
}

export void getDeviceFaultInfo(
	VkDevice device){
	VkDeviceFaultCountsEXT faultCounts{
			.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT,
			.pNext = nullptr,
		};
	VkResult result = vkGetDeviceFaultInfoEXT(device, &faultCounts, nullptr);
	if(result == VK_SUCCESS || result == VK_INCOMPLETE){
		std::cout << "Device fault count: " << faultCounts.vendorInfoCount << std::endl;
	}

	std::vector<VkDeviceFaultAddressInfoEXT> addressInfos(faultCounts.addressInfoCount);
	std::vector<VkDeviceFaultVendorInfoEXT> vendorInfos(faultCounts.vendorInfoCount);

	VkDeviceFaultInfoEXT faultInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT,
			.pAddressInfos = addressInfos.data(),
			.pVendorInfos = vendorInfos.data(),
		};

	vkGetDeviceFaultInfoEXT(device, &faultCounts, &faultInfo);
	std::println(std::cerr, "[VULKAN] DEVICE FAULT: {}", faultInfo.description);
}

namespace cmd{
export
void setColorBlendEnableEXT(
	VkCommandBuffer commandBuffer,
	uint32_t firstAttachment,
	uint32_t attachmentCount,
	const VkBool32* pColorBlendEnables){
	_PFN_vkCmdSetColorBlendEnableEXT(commandBuffer,
		firstAttachment,
		attachmentCount,
		pColorBlendEnables);
}

export
void setColorBlendEquationEXT(
	VkCommandBuffer commandBuffer,
	uint32_t firstAttachment,
	uint32_t attachmentCount,
	const VkColorBlendEquationEXT* pColorBlendEquations){
	_PFN_vkCmdSetColorBlendEquationEXT(commandBuffer, firstAttachment, attachmentCount, pColorBlendEquations);
}


constexpr std::uint32_t MaxStackBufferSize = 16;
constexpr std::uint32_t IndicesDesignator[MaxStackBufferSize]{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	};
export constexpr inline VkDeviceSize zero_offset_designator[MaxStackBufferSize]{};

export void setDescriptorBufferOffsets2(
	VkCommandBuffer commandBuffer,
	const VkSetDescriptorBufferOffsetsInfoEXT* pSetDescriptorBufferOffsetsInfo){
	PFN_cmdSetDescriptorBufferOffsets2EXT(commandBuffer, pSetDescriptorBufferOffsetsInfo);
}

export void drawMeshTasks(
	VkCommandBuffer commandBuffer,
	uint32_t groupCountX,
	uint32_t groupCountY,
	uint32_t groupCountZ){
	drawMeshTasksEXT(commandBuffer, groupCountX, groupCountY, groupCountZ);
}

export void drawMeshTasksIndirect(
	VkCommandBuffer commandBuffer,
	VkBuffer buffer,
	VkDeviceSize offset = 0,
	uint32_t drawCount = 1,
	uint32_t stride = sizeof(VkDrawMeshTasksIndirectCommandEXT)){
	drawMeshTasksIndirectEXT(commandBuffer, buffer, offset, drawCount, stride);
}

export void bindDescriptorBuffersEXT(
	VkCommandBuffer commandBuffer,
	std::uint32_t bufferCount,
	const VkDescriptorBufferBindingInfoEXT* pBindingInfos){
	PFN_cmdBindDescriptorBuffersEXT(commandBuffer, bufferCount, pBindingInfos);
}

export void bindDescriptorBuffersEXT(
	VkCommandBuffer commandBuffer,
	const std::span<const VkDescriptorBufferBindingInfoEXT> bindingInfos){
	PFN_cmdBindDescriptorBuffersEXT(commandBuffer, static_cast<std::uint32_t>(bindingInfos.size()),
		bindingInfos.data());
}

export void setDescriptorBufferOffsetsEXT(
	const VkCommandBuffer commandBuffer,
	const VkPipelineBindPoint pipelineBindPoint,
	const VkPipelineLayout layout,
	const uint32_t firstSet,
	const uint32_t setCount,
	const uint32_t* pBufferIndices,
	const VkDeviceSize* pOffsets) noexcept{
	PFN_cmdSetDescriptorBufferOffsetsEXT(commandBuffer, pipelineBindPoint, layout, firstSet, setCount,
		pBufferIndices, pOffsets);
}

export void setDescriptorBufferOffsetsEXT(
	const VkCommandBuffer commandBuffer,
	const VkPipelineBindPoint pipelineBindPoint,
	const VkPipelineLayout layout,
	const uint32_t firstSet,
	const std::initializer_list<VkDeviceSize> offsets) noexcept{
	static constexpr std::uint32_t MaxStackBufferSize = 16;

	static constexpr std::uint32_t IndicesDesignator[MaxStackBufferSize]{
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
		};

	PFN_cmdSetDescriptorBufferOffsetsEXT(commandBuffer, pipelineBindPoint, layout, firstSet, offsets.size(),
		IndicesDesignator, offsets.begin());
}

export void bindThenSetDescriptorBuffers(
	const VkCommandBuffer commandBuffer,
	const VkPipelineBindPoint pipelineBindPoint,
	const VkPipelineLayout layout,
	const std::uint32_t firstSet,
	const VkDescriptorBufferBindingInfoEXT* pBindingInfos,
	std::uint32_t bufferCount
){
	bindDescriptorBuffersEXT(commandBuffer, bufferCount, pBindingInfos);

	if(bufferCount <= MaxStackBufferSize){
		setDescriptorBufferOffsetsEXT(commandBuffer, pipelineBindPoint, layout, firstSet, bufferCount,
			IndicesDesignator, zero_offset_designator);
	} else{
		throw std::overflow_error("Too many descriptor buffers in the command buffer.");
	}
}

export void bindThenSetDescriptorBuffers(
	const VkCommandBuffer commandBuffer,
	const VkPipelineBindPoint pipelineBindPoint,
	const VkPipelineLayout layout,
	const uint32_t firstSet,
	const std::span<const VkDescriptorBufferBindingInfoEXT> infos
){
	bindThenSetDescriptorBuffers(commandBuffer, pipelineBindPoint, layout, firstSet, infos.data(),
		infos.size());
}

export void bindThenSetDescriptorBuffers(
	const VkCommandBuffer commandBuffer,
	const VkPipelineBindPoint pipelineBindPoint,
	const VkPipelineLayout layout,
	const uint32_t firstSet,
	const std::initializer_list<const VkDescriptorBufferBindingInfoEXT> infos
){
	bindThenSetDescriptorBuffers(commandBuffer, pipelineBindPoint, layout, firstSet, infos.begin(),
		infos.size());
}

export
template <
	std::ranges::contiguous_range InfoRng = std::initializer_list<VkDescriptorBufferBindingInfoEXT>,
	std::ranges::contiguous_range Offset = std::initializer_list<VkDeviceSize>>
void bind_descriptors(
	VkCommandBuffer commandBuffer,
	const VkPipelineBindPoint pipelineBindPoint,
	const VkPipelineLayout layout,
	const uint32_t firstSet,

	const InfoRng& infos,
	const Offset& offsets
){
	assert(std::ranges::size(infos) == std::ranges::size(offsets));
	cmd::bindDescriptorBuffersEXT(commandBuffer, std::ranges::size(infos), std::ranges::data(infos));
	cmd::setDescriptorBufferOffsetsEXT(
		commandBuffer,
		pipelineBindPoint,
		layout, firstSet, std::ranges::size(offsets), IndicesDesignator, std::ranges::data(offsets));
}

export
template <
	std::ranges::contiguous_range InfoRng = std::initializer_list<VkDescriptorBufferBindingInfoEXT>,
	std::ranges::contiguous_range Offset = std::initializer_list<VkDeviceSize>>
void bind_descriptors(
	VkCommandBuffer commandBuffer,
	const InfoRng& infos
){
	cmd::bindDescriptorBuffersEXT(commandBuffer, std::ranges::size(infos), std::ranges::data(infos));
}

export
template <
	std::ranges::contiguous_range Targets = std::initializer_list<std::uint32_t>,
	std::ranges::contiguous_range Offset = std::initializer_list<VkDeviceSize>>
void set_descriptor_offsets(
	VkCommandBuffer commandBuffer,
	const VkPipelineBindPoint pipelineBindPoint,
	const VkPipelineLayout layout,
	const uint32_t firstSet,

	const Targets& targets,
	const Offset& offsets
){
	assert(std::ranges::size(offsets) >= std::ranges::size(targets));
	cmd::setDescriptorBufferOffsetsEXT(
		commandBuffer,
		pipelineBindPoint,
		layout, firstSet, std::ranges::size(targets), std::ranges::data(targets), std::ranges::data(offsets));
}
}

export void load_ext(VkInstance instance){
	getPhysicalDeviceProperties2KHR = LoadFuncPtr(instance, vkGetPhysicalDeviceProperties2KHR);
	getDescriptorSetLayoutSizeEXT = LoadFuncPtr(instance, vkGetDescriptorSetLayoutSizeEXT);
	_PFNgetDescriptorSetLayoutBindingOffsetEXT = LoadFuncPtr(instance, vkGetDescriptorSetLayoutBindingOffsetEXT);
	getDescriptorEXT_ = LoadFuncPtr(instance, vkGetDescriptorEXT);

	drawMeshTasksEXT = LoadFuncPtr(instance, vkCmdDrawMeshTasksEXT);
	drawMeshTasksIndirectEXT = LoadFuncPtr(instance, vkCmdDrawMeshTasksIndirectEXT);

	PFN_cmdBindDescriptorBuffersEXT = LoadFuncPtr(instance, vkCmdBindDescriptorBuffersEXT);
	PFN_cmdSetDescriptorBufferOffsetsEXT = LoadFuncPtr(instance, vkCmdSetDescriptorBufferOffsetsEXT);
	PFN_cmdSetDescriptorBufferOffsets2EXT = LoadFuncPtr(instance, vkCmdSetDescriptorBufferOffsets2EXT);

	_PFN_vkCmdSetColorBlendEnableEXT = LoadFuncPtr(instance, vkCmdSetColorBlendEnableEXT);
	_PFN_vkCmdSetColorBlendEquationEXT = LoadFuncPtr(instance, vkCmdSetColorBlendEquationEXT);


	vkGetDeviceFaultInfoEXT = LoadFuncPtr(instance, vkGetDeviceFaultInfoEXT);
}
}