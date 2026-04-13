module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.vertex_consteval_def;

import mo_yanxi.meta_programming;
import std;

namespace mo_yanxi::vk{
export
template <typename T, typename C>
struct vertex_attribute {
	using class_type = C;
    T C::* mptr;
    VkFormat format;
};

// 2. 辅助元函数：用于从 attribute<T, C> 中提取基类类型 C
// template <typename A>
// struct attribute_class_traits;
//
// template <typename T, typename C>
// struct attribute_class_traits<attribute<T, C>> {
//     using class_type = C;
// };

// 3. 核心生成器类模板
template <auto... Attrs>
struct VertexInputStateGenerator {
    static_assert(sizeof...(Attrs) > 0, "At least one attribute must be provided.");

    using FirstAttrType = std::tuple_element_t<0, std::tuple<decltype(Attrs)...>>;
    using ClassType = typename FirstAttrType::class_type;

    static_assert((std::is_same_v<typename decltype(Attrs)::class_type, ClassType> && ...),
                  "All attributes must belong to the same base class.");

    static consteval std::array<VkVertexInputAttributeDescription, sizeof...(Attrs)> generate_attributes() {
        std::array<VkVertexInputAttributeDescription, sizeof...(Attrs)> attrs{};
        std::uint32_t location = 0;

        // 关键修改：使用 C++20 模板 Lambda，让 Attr 成为 NTTP
        auto assign_attr = [&]<auto Attr>() {
            VkVertexInputAttributeDescription desc{};
            desc.location = location;
            desc.binding = 0;
            desc.format = Attr.format;
            // 此时 Attr 属于 NTTP，Attr.mptr 是核心常量表达式，完美契合 offset_of
            desc.offset = static_cast<std::uint32_t>(mo_yanxi::offset_of(Attr.mptr));
            attrs[location] = desc;
            location++;
        };

        // 关键修改：利用折叠表达式，显式调用 Lambda 的模板 operator()
        (assign_attr.template operator()<Attrs>(), ...);

        return attrs;
    }

    static constexpr std::array<VkVertexInputAttributeDescription, sizeof...(Attrs)> attributes = generate_attributes();

    static constexpr VkVertexInputBindingDescription binding = {
        .binding = 0,
        .stride = sizeof(ClassType),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    static consteval VkPipelineVertexInputStateCreateInfo get_info() {
        VkPipelineVertexInputStateCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;
        info.vertexBindingDescriptionCount = 1;
        info.pVertexBindingDescriptions = &binding;
        info.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributes.size());
        info.pVertexAttributeDescriptions = attributes.data();
        return info;
    }
};

export
template <auto... Attrs>
consteval VkPipelineVertexInputStateCreateInfo make_vertex_input_state() {
    return VertexInputStateGenerator<Attrs...>::get_info();
}


}