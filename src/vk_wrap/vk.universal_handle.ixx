module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.universal_handle;

namespace mo_yanxi{
	namespace vk{


export struct context_info{
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;
};


export struct texture_buffer_write{
	VkBuffer buffer;
	VkRect2D region;
	VkDeviceSize buffer_offset;
};

export struct image_handle{
	VkImage image;
	VkImageView image_view;
};

export struct render_output_image{
	image_handle handle{};
	VkExtent2D extent{};
	VkFormat format{};
	uint32_t index{};
};

	}
}
