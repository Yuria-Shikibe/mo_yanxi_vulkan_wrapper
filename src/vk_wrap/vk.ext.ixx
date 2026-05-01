module;

#include <vulkan/vulkan.h>
#include "vk_loader.hpp"

export module mo_yanxi.vk:ext;
import std;

namespace mo_yanxi::vk{
// ==========================================
// 1. 定义函数指针 (统一使用 DEFINE_FUNC_PTR)
// ==========================================

inline DEFINE_FUNC_PTR(vkGetPhysicalDeviceProperties2KHR);
inline DEFINE_FUNC_PTR(vkGetDescriptorSetLayoutSizeEXT);
inline DEFINE_FUNC_PTR(vkGetDescriptorSetLayoutBindingOffsetEXT);
inline DEFINE_FUNC_PTR(vkGetDescriptorEXT);
inline DEFINE_FUNC_PTR(vkCmdDrawMeshTasksEXT);
inline DEFINE_FUNC_PTR(vkCmdDrawMeshTasksIndirectEXT);
inline DEFINE_FUNC_PTR(vkGetDeviceFaultInfoEXT);
inline DEFINE_FUNC_PTR(vkCmdBindDescriptorBuffersEXT);
inline DEFINE_FUNC_PTR(vkCmdSetDescriptorBufferOffsetsEXT);
inline DEFINE_FUNC_PTR(vkCmdSetDescriptorBufferOffsets2EXT);
inline DEFINE_FUNC_PTR(vkCmdSetColorBlendEnableEXT);
inline DEFINE_FUNC_PTR(vkCmdSetColorBlendEquationEXT);
inline DEFINE_FUNC_PTR(vkCmdSetColorWriteMaskEXT);

inline DEFINE_FUNC_PTR(vkWriteSamplerDescriptorsEXT);
inline DEFINE_FUNC_PTR(vkWriteResourceDescriptorsEXT);
inline DEFINE_FUNC_PTR(vkCmdBindSamplerHeapEXT);
inline DEFINE_FUNC_PTR(vkCmdBindResourceHeapEXT);
inline DEFINE_FUNC_PTR(vkCmdPushDataEXT);

export VkResult writeSamplerDescriptorsEXT(
	VkDevice device,
	uint32_t samplerCount,
	const VkSamplerCreateInfo* pSamplers,
	const VkHostAddressRangeEXT* pDescriptors){
	return _PFN_vkWriteSamplerDescriptorsEXT(device, samplerCount, pSamplers, pDescriptors);
}

export VkResult writeResourceDescriptorsEXT(
	VkDevice device,
	uint32_t resourceCount,
	const VkResourceDescriptorInfoEXT* pResources,
	const VkHostAddressRangeEXT* pDescriptors){
	return _PFN_vkWriteResourceDescriptorsEXT(device, resourceCount, pResources, pDescriptors);
}


// ==========================================
// 2. 导出包装函数
// ==========================================

export void getPhysicalDeviceProperties2KHR(
	VkPhysicalDevice physicalDevice,
	VkPhysicalDeviceProperties2* pProperties){
	_PFN_vkGetPhysicalDeviceProperties2KHR(physicalDevice, pProperties);
}

export void getDescriptorSetLayoutSizeEXT(
	VkDevice device,
	VkDescriptorSetLayout layout,
	VkDeviceSize* pLayoutSizeInBytes){
	_PFN_vkGetDescriptorSetLayoutSizeEXT(device, layout, pLayoutSizeInBytes);
}

export void getDescriptorSetLayoutBindingOffsetEXT(
	VkDevice device,
	VkDescriptorSetLayout layout,
	uint32_t binding,
	VkDeviceSize* pOffset){
	_PFN_vkGetDescriptorSetLayoutBindingOffsetEXT(device, layout, binding, pOffset);
}

export void getDescriptorEXT(
	VkDevice device,
	const VkDescriptorGetInfoEXT* pDescriptorInfo,
	size_t dataSize,
	void* pDescriptor){
	assert(pDescriptorInfo != nullptr);
	assert(pDescriptor != nullptr);
	_PFN_vkGetDescriptorEXT(device, pDescriptorInfo, dataSize, pDescriptor);
}

export void getDeviceFaultInfo(
	VkDevice device){
	VkDeviceFaultCountsEXT faultCounts{
			.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT,
			.pNext = nullptr,
		};
	// 注意：这里调用的是生成的 _PFN 指针
	VkResult result = _PFN_vkGetDeviceFaultInfoEXT(device, &faultCounts, nullptr);

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

	_PFN_vkGetDeviceFaultInfoEXT(device, &faultCounts, &faultInfo);
	std::println(std::cerr, "[VULKAN] DEVICE FAULT: {}", faultInfo.description);
}

namespace cmd{
export void bindSamplerHeapEXT(
	VkCommandBuffer commandBuffer,
	const VkBindHeapInfoEXT& pBindInfo){
	_PFN_vkCmdBindSamplerHeapEXT(commandBuffer, &pBindInfo);
}

export void bindResourceHeapEXT(
	VkCommandBuffer commandBuffer,
	const VkBindHeapInfoEXT& pBindInfo){
	_PFN_vkCmdBindResourceHeapEXT(commandBuffer, &pBindInfo);
}

export void pushDataEXT(
	VkCommandBuffer commandBuffer,
	const VkPushDataInfoEXT& pPushDataInfo){
	_PFN_vkCmdPushDataEXT(commandBuffer, &pPushDataInfo);
}

export void setColorBlendEnableEXT(
	VkCommandBuffer commandBuffer,
	uint32_t firstAttachment,
	uint32_t attachmentCount,
	const VkBool32* pColorBlendEnables){
	_PFN_vkCmdSetColorBlendEnableEXT(commandBuffer,
		firstAttachment,
		attachmentCount,
		pColorBlendEnables);
}

export void setColorBlendEquationEXT(
	VkCommandBuffer commandBuffer,
	uint32_t firstAttachment,
	uint32_t attachmentCount,
	const VkColorBlendEquationEXT* pColorBlendEquations){
	_PFN_vkCmdSetColorBlendEquationEXT(commandBuffer, firstAttachment, attachmentCount, pColorBlendEquations);
}

// 新增：为了保持一致性，补充了 WriteMask 的包装
export void setColorWriteMaskEXT(
	VkCommandBuffer commandBuffer,
	uint32_t firstAttachment,
	uint32_t attachmentCount,
	const VkColorComponentFlags* pColorWriteMasks){
	_PFN_vkCmdSetColorWriteMaskEXT(commandBuffer, firstAttachment, attachmentCount, pColorWriteMasks);
}

constexpr std::uint32_t MaxStackBufferSize = 16;
constexpr std::uint32_t IndicesDesignator[MaxStackBufferSize]{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	};

export constexpr inline VkDeviceSize zero_offset_designator[MaxStackBufferSize]{};

export void setDescriptorBufferOffsets2(
	VkCommandBuffer commandBuffer,
	const VkSetDescriptorBufferOffsetsInfoEXT* pSetDescriptorBufferOffsetsInfo){
	_PFN_vkCmdSetDescriptorBufferOffsets2EXT(commandBuffer, pSetDescriptorBufferOffsetsInfo);
}

export void drawMeshTasks(
	VkCommandBuffer commandBuffer,
	uint32_t groupCountX,
	uint32_t groupCountY,
	uint32_t groupCountZ){
	_PFN_vkCmdDrawMeshTasksEXT(commandBuffer, groupCountX, groupCountY, groupCountZ);
}

export void drawMeshTasksIndirect(
	VkCommandBuffer commandBuffer,
	VkBuffer buffer,
	VkDeviceSize offset = 0,
	uint32_t drawCount = 1,
	uint32_t stride = sizeof(VkDrawMeshTasksIndirectCommandEXT)){
	_PFN_vkCmdDrawMeshTasksIndirectEXT(commandBuffer, buffer, offset, drawCount, stride);
}

export void bindDescriptorBuffersEXT(
	VkCommandBuffer commandBuffer,
	std::uint32_t bufferCount,
	const VkDescriptorBufferBindingInfoEXT* pBindingInfos){
	_PFN_vkCmdBindDescriptorBuffersEXT(commandBuffer, bufferCount, pBindingInfos);
}

export void bindDescriptorBuffersEXT(
	VkCommandBuffer commandBuffer,
	const std::span<const VkDescriptorBufferBindingInfoEXT> bindingInfos){
	_PFN_vkCmdBindDescriptorBuffersEXT(commandBuffer, static_cast<std::uint32_t>(bindingInfos.size()),
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
	_PFN_vkCmdSetDescriptorBufferOffsetsEXT(commandBuffer, pipelineBindPoint, layout, firstSet, setCount,
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
	_PFN_vkCmdSetDescriptorBufferOffsetsEXT(commandBuffer, pipelineBindPoint, layout, firstSet, offsets.size(),
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

// ==========================================
// 3. 加载函数
// ==========================================

export void load_ext(VkInstance instance){
	LoadFuncPtrByName(instance, vkGetPhysicalDeviceProperties2KHR);
	LoadFuncPtrByName(instance, vkGetDescriptorSetLayoutSizeEXT);
	LoadFuncPtrByName(instance, vkGetDescriptorSetLayoutBindingOffsetEXT);
	LoadFuncPtrByName(instance, vkGetDescriptorEXT);

	LoadFuncPtrByName(instance, vkCmdDrawMeshTasksEXT);
	LoadFuncPtrByName(instance, vkCmdDrawMeshTasksIndirectEXT);

	LoadFuncPtrByName(instance, vkCmdBindDescriptorBuffersEXT);
	LoadFuncPtrByName(instance, vkCmdSetDescriptorBufferOffsetsEXT);
	LoadFuncPtrByName(instance, vkCmdSetDescriptorBufferOffsets2EXT);

	LoadFuncPtrByName(instance, vkCmdSetColorBlendEnableEXT);
	LoadFuncPtrByName(instance, vkCmdSetColorBlendEquationEXT);
	LoadFuncPtrByName(instance, vkCmdSetColorWriteMaskEXT);

	LoadFuncPtrByName(instance, vkGetDeviceFaultInfoEXT);

	LoadFuncPtrByName(instance, vkWriteSamplerDescriptorsEXT);
	LoadFuncPtrByName(instance, vkWriteResourceDescriptorsEXT);
	LoadFuncPtrByName(instance, vkCmdBindSamplerHeapEXT);
	LoadFuncPtrByName(instance, vkCmdBindResourceHeapEXT);
	LoadFuncPtrByName(instance, vkCmdPushDataEXT);
}
}
