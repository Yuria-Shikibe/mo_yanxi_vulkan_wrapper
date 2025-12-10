module;

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cassert>

export module mo_yanxi.vk:aliasing;

import :resources;
import mo_yanxi.vk.util;
import std;

namespace mo_yanxi::vk{

export struct aliased_image : aliased_resource_base<VkImage> {
protected:

    // 核心创建逻辑：调用 allocator 的别名创建函数
    void create(VkDeviceSize offset, const VkImageCreateInfo& create_info) {
        this->handle = get_allocator().create_aliasing_image(get_allocation(), offset, create_info);
    }

public:
    [[nodiscard]] aliased_image() noexcept = default;

    [[nodiscard]] aliased_image(
       const allocator_usage allocator,
       VmaAllocation base_allocation,
       VkDeviceSize offset,
       const VkImageCreateInfo& create_info
    ) : aliased_resource_base(allocator, base_allocation) { // 初始化基类 gpu_resource_
       create(offset, create_info);
    }

    // 便捷构造函数：自动推导 CreateInfo
    [[nodiscard]] aliased_image(
       const allocator_usage allocator,
       VmaAllocation base_allocation,
       VkDeviceSize offset,
       const VkExtent3D extent,
       const VkImageUsageFlags usage,
       const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
       const std::uint32_t mipLevels = 1,
       const std::uint32_t arrayLayers = 1,
       const VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
       VkImageCreateFlags flags = 0, // 可能需要 ALIAS 或 MUTABLE 标志
       const VkImageType type = VK_IMAGE_TYPE_MAX_ENUM
    ) : aliased_image{
          allocator,
          base_allocation,
          offset,
          {
             .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
             .pNext = nullptr,
             .flags = flags | VK_IMAGE_CREATE_ALIAS_BIT, // 强制加上 ALIAS 标志
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
             .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED // 别名图像初始布局必须是 UNDEFINED
          }
       }{
    }
};


export struct aliased_buffer : aliased_resource_base<VkBuffer> {
protected:
    VkDeviceSize size{};

    // 核心创建逻辑：调用 allocator 的别名创建函数
    void create(VkDeviceSize offset, const VkBufferCreateInfo& create_info){
       this->handle = get_allocator().create_aliasing_buffer(get_allocation(), offset, create_info);
    }

public:
    [[nodiscard]] aliased_buffer() = default;

    [[nodiscard]] explicit aliased_buffer(
       const vk::allocator_usage allocator,
       VmaAllocation base_allocation,
       VkDeviceSize offset,
       const VkBufferCreateInfo& create_info
    ) : aliased_resource_base(allocator, base_allocation) {
       create(offset, create_info);
    }

    // --- 以下为复用的辅助函数 ---

    [[nodiscard]] constexpr VkDeviceSize get_size() const noexcept{
       return size;
    }

    [[nodiscard]] VkDeviceAddress get_address() const noexcept{
       const VkBufferDeviceAddressInfo bufferDeviceAddressInfo{
          .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
          .pNext = nullptr,
          .buffer = handle
       };
       return vkGetBufferDeviceAddress(get_device(), &bufferDeviceAddressInfo);
    }

    // 借用机制：注意这里的 off 是相对于这个 buffer 的，而不是相对于底层 allocation 的
    buffer_range borrow(VkDeviceSize off, VkDeviceSize len) const noexcept{
       assert(off + len <= get_size());
       return buffer_range{*this, get_address(), off, len};
    }

    buffer_range borrow_after(VkDeviceSize off) const noexcept{
       assert(off < get_size());
       return buffer_range{*this, get_address(), off, get_size() - off};
    }

    buffer_range borrow() const noexcept{
       return buffer_range{*this, get_address(), 0, get_size()};
    }
};

}