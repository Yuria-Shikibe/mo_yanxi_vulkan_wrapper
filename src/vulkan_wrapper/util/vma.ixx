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
		const VkMemoryRequirements& bufferInfo,
		const VmaAllocationCreateInfo& alloc_info
	) const{
		VmaAllocation allocation;

		if(const auto rst = vmaAllocateMemory(handle, &bufferInfo, &alloc_info, &allocation, nullptr)){
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


// using HandleTy = VkImage;
export
template <typename HandleTy>
struct resource_base : exclusive_handle<HandleTy>{
private:
	allocator_usage allocator{};
	VmaAllocation allocation{};

protected:
	void pass_alloc(const allocate_info<HandleTy> info){
		deallocate();
		this->handle = info.handle;
		allocation = info.allocation;
	}

	[[nodiscard]] explicit resource_base(const allocator_usage allocator)
	: allocator(allocator){
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

	resource_base(resource_base&& other) noexcept
	: exclusive_handle<HandleTy>{std::move(other)},
	allocator{std::exchange(other.allocator, {})},
	allocation{std::exchange(other.allocation, {})}{
	}

	resource_base& operator=(resource_base&& other) noexcept{
		if(this == &other) return *this;
		exclusive_handle<HandleTy>::operator =(std::move(other));
		std::swap(this->allocator, other.allocator);
		std::swap(this->allocation, other.allocation);
		return *this;
	}

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
};
}

// NOLINTEND(*-misplaced-const)
