#include <vulkan/vulkan.h>
import mo_yanxi.vk;

int main(){
	using namespace mo_yanxi::vk;

	constexpr VkApplicationInfo ApplicationInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Hello Triangle",
		.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0),
		.apiVersion = VK_API_VERSION_1_3,
	};

	instance instance{VkApplicationInfo{
		ApplicationInfo
	}, {}};
}