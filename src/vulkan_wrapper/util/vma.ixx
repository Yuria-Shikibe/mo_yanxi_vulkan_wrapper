module;

#include <cassert>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

export module mo_yanxi.vk.util:vma;

export import :exception;
export import mo_yanxi.vk.universal_handle;
import mo_yanxi.handle_wrapper;

// NOLINTBEGIN(*-misplaced-const)
namespace mo_yanxi::vk{
export
template <typename T>
struct allocate_info{
	T handle;
	VmaAllocation allocation;
};

export
struct allocator_usage{
	VmaAllocator handle;

	[[nodiscard]] VmaAllocatorInfo get_info() const noexcept{
		VmaAllocatorInfo info;
		vmaGetAllocatorInfo(handle, &info);
		return info;
	}

	[[nodiscard]] VkDevice get_device() const noexcept{
		VmaAllocatorInfo info;
		vmaGetAllocatorInfo(handle, &info);
		return info.device;
	}

	[[nodiscard]] VkPhysicalDevice get_physical_device() const noexcept{
		VmaAllocatorInfo info;
		vmaGetAllocatorInfo(handle, &info);
		return info.physicalDevice;
	}


	[[nodiscard]] context_info get_context_info() const noexcept{
		VmaAllocatorInfo info;
		vmaGetAllocatorInfo(handle, &info);
		return {info.instance, info.physicalDevice, info.device};
	}

	[[nodiscard]] allocate_info<VkBuffer> allocate(
		const VkBufferCreateInfo& bufferInfo,
		const VmaAllocationCreateInfo& alloc_info
	) const{
		VkBuffer buffer;
		VmaAllocation allocation;
		if(const auto rst = vmaCreateBuffer(handle, &bufferInfo, &alloc_info, &buffer, &allocation, nullptr)){
			throw vk::vk_error{rst, "Memory Allocation Failed"};
		}
		return {buffer, allocation};
	}

	[[nodiscard]] allocate_info<VkImage> allocate(
		const VkImageCreateInfo& bufferInfo,
		const VmaAllocationCreateInfo& alloc_info
	) const{
		VkImage image;
		VmaAllocation allocation;
		if(const auto rst = vmaCreateImage(handle, &bufferInfo, &alloc_info, &image, &allocation, nullptr)){
			throw vk::vk_error{rst, "Memory Allocation Failed"};
		}
		return {image, allocation};
	}

	[[nodiscard]] VmaAllocation allocate_memory(
		const VkMemoryRequirements& requirement,
		const VmaAllocationCreateInfo& alloc_info
	) const{
		VmaAllocation allocation;

		if(const auto rst = vmaAllocateMemory(handle, &requirement, &alloc_info, &allocation, nullptr)){
			throw vk::vk_error{rst, "Memory Allocation Failed"};
		}

		return allocation;
	}

	void bind(
		const VkBuffer buffer,
		const VmaAllocation allocation
	) const{
		if(const auto rst = vmaBindBufferMemory(handle, allocation, buffer)){
			throw vk::vk_error{rst, "Memory Binding For Buffer Failed"};
		}
	}

	void bind(
		const VkImage image,
		const VmaAllocation allocation
	) const{
		if(const auto rst = vmaBindImageMemory(handle, allocation, image)){
			throw vk::vk_error{rst, "Memory Binding For Image Failed"};
		}
	}

	[[nodiscard]] VmaAllocation allocate_for(
		const VkBuffer buffer,
		const VmaAllocationCreateInfo& alloc_info
	) const{
		VmaAllocation allocation;
		if(const auto rst = vmaAllocateMemoryForBuffer(handle, buffer, &alloc_info, &allocation, nullptr)){
			throw vk::vk_error{rst, "Memory Allocation For Buffer Failed"};
		}
		return allocation;
	}

	[[nodiscard]] VmaAllocation allocate_for(
		const VkImage image,
		const VmaAllocationCreateInfo& alloc_info
	) const{
		VmaAllocation allocation;

		if(const auto rst = vmaAllocateMemoryForImage(handle, image, &alloc_info, &allocation, nullptr)){
			throw vk_error{rst, "Memory Allocation For Buffer Failed"};
		}
		return allocation;
	}

	template <typename T>
	void deallocate(allocate_info<T> info) const noexcept{
		if constexpr(std::same_as<T, VkBuffer>){
			::vmaDestroyBuffer(handle, info.handle, info.allocation);
		} else if constexpr(std::same_as<T, VkImage>){
			::vmaDestroyImage(handle, info.handle, info.allocation);
		} else{
			static_assert(false, "Unsupported type");
		}
	}

	void deallocate(VmaAllocation allocation) const noexcept{
		::vmaFreeMemory(handle, allocation);
	}

	explicit operator bool() const noexcept{
		return handle != nullptr;
	}

	explicit(false) operator VmaAllocator() const noexcept{
		return handle;
	}

	[[nodiscard]] VkBuffer create_aliasing_buffer(
		VmaAllocation allocation,
		VkDeviceSize offset,
		const VkBufferCreateInfo& create_info
	) const {
		VkBuffer buffer;
		// 使用 vmaCreateAliasingBuffer2 以绑定到现有的 Allocation 及特定 offset
		if(const auto rst = vmaCreateAliasingBuffer2(handle, allocation, offset, &create_info, &buffer)){
			throw vk::vk_error{rst, "Create Aliasing Buffer Failed"};
		}
		return buffer;
	}

	[[nodiscard]] VkImage create_aliasing_image(
		VmaAllocation allocation,
		VkDeviceSize offset,
		const VkImageCreateInfo& create_info
	) const {
		VkImage image;
		if(const auto rst = vmaCreateAliasingImage2(handle, allocation, offset, &create_info, &image)){
			throw vk::vk_error{rst, "Create Aliasing Image Failed"};
		}
		return image;
	}

	void destroy_aliased(VkBuffer handle) const noexcept{
		const auto device = get_device();
		::vkDestroyBuffer(device, handle, nullptr);

	}

	void destroy_aliased(VkImage handle) const noexcept{
		const auto device = get_device();
		::vkDestroyImage(device, handle, nullptr);
	}

};

export struct allocator{
private:
	VmaAllocator handle;
public:

	[[nodiscard]] allocator() = default;

	[[nodiscard]] explicit(false) allocator(const VmaAllocatorCreateInfo& info){
		vmaCreateAllocator(&info, &handle);
	}

	[[nodiscard]] explicit(false)
	allocator(
		VkInstance instance,
		VkPhysicalDevice physical_device,
		VkDevice device,
		VmaAllocatorCreateFlags append_flags = 0
	) : allocator(
		{
			.flags = append_flags,
			.physicalDevice = physical_device,
			.device = device,
			.instance = instance,
			.vulkanApiVersion = VK_API_VERSION_1_4,
		}
	){
	}

	[[nodiscard]] explicit(false)
	allocator(
		const context_info& context_info,
		VmaAllocatorCreateFlags append_flags = 0
	) : allocator(context_info.instance, context_info.physical_device, context_info.device, append_flags){
	}


	~allocator(){
		vmaDestroyAllocator(this->handle);
	}

	explicit operator bool() const noexcept{
		return this->handle != nullptr;
	}

	explicit(false) operator VmaAllocator() const noexcept{
		return this->handle;
	}

	explicit(false) operator allocator_usage() const noexcept{
		return {this->handle};
	}

	allocator(const allocator& other) = delete;
	allocator(allocator&& other) noexcept : handle(std::exchange(other.handle, {})){

	}

	allocator& operator=(const allocator& other) = delete;

	allocator& operator=(allocator&& other) noexcept{
		if(this == &other) return *this;
		if(handle){
			vmaDestroyAllocator(this->handle);
		}
		handle = std::exchange(other.handle, {});
		return *this;
	}
};

export
template <typename HandleTy>
struct resource_base;

export
template <typename HandleTy>
struct aliased_resource_base;

struct gpu_resource_{
private:
	allocator_usage allocator{};
	VmaAllocation allocation{};

	template <typename HandleTy>
	friend struct resource_base;

	template <typename HandleTy>
	friend struct aliased_resource_base;
protected:

	[[nodiscard]] explicit gpu_resource_(const allocator_usage allocator)
	: allocator(allocator){
	}

	[[nodiscard]] gpu_resource_(const allocator_usage& allocator, VmaAllocation allocation)
		: allocator(allocator),
		  allocation(allocation){
	}

	[[nodiscard]] gpu_resource_() = default;

public:

	[[nodiscard]] VmaAllocation get_allocation() const noexcept{
		return allocation;
	}

	[[nodiscard]] allocator_usage get_allocator() const noexcept{
		return allocator;
	}

	[[nodiscard]] VkDevice get_device() const noexcept{
		return allocator.get_device();
	}

	[[nodiscard]] VkPhysicalDevice get_physical_device() const noexcept{
		return allocator.get_physical_device();
	}

	void flush(const VkDeviceSize size = VK_WHOLE_SIZE, const VkDeviceSize offset = 0) const{
		if(const auto rst = vmaFlushAllocation(allocator, allocation, offset, size)){
			throw vk::vk_error(rst, "Flush Memory Failed");
		}
	}

	void invalidate(const VkDeviceSize size = VK_WHOLE_SIZE, const VkDeviceSize offset = 0) const{
		if(const auto rst = vmaInvalidateAllocation(allocator, allocation, offset, size)){
			throw vk::vk_error(rst, "Invalidate Memory Failed");
		}
	}

	[[nodiscard]] void* map() const{
		assert(allocator);
		void* mapped;
		if(auto rst = vmaMapMemory(allocator, allocation, &mapped)){
			throw vk::vk_error(rst, "Mapping Memory Failed");
		}
		return mapped;
	}

	void unmap() const{
		assert(allocator);
		vmaUnmapMemory(allocator, allocation);
	}

	[[nodiscard]] VkMemoryPropertyFlags get_allocation_prop_flags() const noexcept{
		assert(allocator);
		VkMemoryPropertyFlags flags{};
		if(allocation) vmaGetAllocationMemoryProperties(allocator, allocation, &flags);
		return flags;
	}

	gpu_resource_(const gpu_resource_& other) = delete;

	gpu_resource_(gpu_resource_&& other) noexcept
		: allocator{std::exchange(other.allocator, {})},
		  allocation{std::exchange(other.allocation, {})}{
	}

	gpu_resource_& operator=(gpu_resource_&& other) noexcept {
		std::swap(allocator, other.allocator);
		std::swap(allocation, other.allocation);
		return *this;
	}
};


template <typename HandleTy>
struct resource_base : gpu_resource_, exclusive_handle<HandleTy>{

protected:
	using gpu_resource_::gpu_resource_;

	void pass_alloc(const allocate_info<HandleTy> info){
		deallocate();
		this->handle = info.handle;
		allocation = info.allocation;
	}


public:
	void deallocate() noexcept requires requires{
		allocator.deallocate(allocate_info<HandleTy>{});
	}{
		if(allocation) allocator.deallocate<HandleTy>(allocate_info<HandleTy>{this->handle, allocation});
		allocation = nullptr;
		this->handle = nullptr;
	}

	[[nodiscard]] resource_base() = default;

	~resource_base() requires requires{
		deallocate();
	}{
		deallocate();
	}

	~resource_base() requires (!requires{
		deallocate();
	}) = default;

	resource_base(const resource_base& other) = default;
	resource_base& operator=(const resource_base& other) = default;


	resource_base(resource_base&& other) noexcept = default;
	resource_base& operator=(resource_base&& other) noexcept = default;
};


template <typename HandleTy>
struct aliased_resource_base : gpu_resource_, exclusive_handle<HandleTy>{
protected:
	using gpu_resource_::gpu_resource_;

	void deallocate() noexcept requires requires{
		allocator.destroy_aliased(HandleTy{});
	}{
		if(this->handle && allocation){
			allocator.destroy_aliased(this->handle);
		}
		this->handle = nullptr;
		allocation = nullptr;
	}

public:
	~aliased_resource_base() requires requires{
		deallocate();
	}{
		deallocate();
	}

	~aliased_resource_base() requires (!requires{
		deallocate();
	}) = default;

	aliased_resource_base(const aliased_resource_base& other) = delete;
	aliased_resource_base& operator=(const aliased_resource_base& other) = delete;
	aliased_resource_base(aliased_resource_base&& other) noexcept = default;
	aliased_resource_base& operator=(aliased_resource_base&& other) noexcept = default;
};


}

// NOLINTEND(*-misplaced-const)
