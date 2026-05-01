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
    std::uint32_t binding = 0;
};

template <std::uint32_t Binding, auto FirstAttr, auto... RestAttrs>
struct binding_class_finder {
    using type = std::conditional_t<
        FirstAttr.binding == Binding,
        typename decltype(FirstAttr)::class_type,
        typename binding_class_finder<Binding, RestAttrs...>::type
    >;
};

template <std::uint32_t Binding, auto LastAttr>
struct binding_class_finder<Binding, LastAttr> {
    using type = std::conditional_t<
        LastAttr.binding == Binding,
        typename decltype(LastAttr)::class_type,
        void
    >;
};

template <auto... Attrs>
struct VertexInputStateGenerator {
    static_assert(sizeof...(Attrs) > 0, "At least one attribute must be provided.");

    static consteval std::size_t count_bindings() {
        constexpr std::array bindings{Attrs.binding...};
        std::size_t count = 0;

        for(std::size_t i = 0; i < bindings.size(); ++i){
            bool seen = false;
            for(std::size_t j = 0; j < i; ++j){
                if(bindings[j] == bindings[i]){
                    seen = true;
                    break;
                }
            }

            if(!seen){
                ++count;
            }
        }

        return count;
    }

    static constexpr std::size_t binding_count = count_bindings();

    template <std::uint32_t Binding>
    using binding_class_type = typename binding_class_finder<Binding, Attrs...>::type;

    template <std::uint32_t Binding>
    static consteval std::uint32_t binding_stride() {
        using ClassType = binding_class_type<Binding>;
        static_assert(!std::is_void_v<ClassType>, "Binding must reference at least one vertex attribute.");
        static_assert(((Attrs.binding != Binding || std::is_same_v<typename decltype(Attrs)::class_type, ClassType>) && ...),
            "All attributes assigned to the same binding must belong to the same class.");
        return static_cast<std::uint32_t>(sizeof(ClassType));
    }

    static consteval std::array<VkVertexInputAttributeDescription, sizeof...(Attrs)> generate_attributes() {
        std::array<VkVertexInputAttributeDescription, sizeof...(Attrs)> attrs{};
        std::uint32_t location = 0;

        auto assign_attr = [&]<auto Attr>() {
            VkVertexInputAttributeDescription desc{};
            desc.location = location;
            desc.binding = Attr.binding;
            desc.format = Attr.format;
            desc.offset = static_cast<std::uint32_t>(mo_yanxi::offset_of(Attr.mptr));
            attrs[location] = desc;
            location++;
        };

        (assign_attr.template operator()<Attrs>(), ...);

        return attrs;
    }

    static constexpr std::array<VkVertexInputAttributeDescription, sizeof...(Attrs)> attributes = generate_attributes();

    static consteval std::array<VkVertexInputBindingDescription, binding_count> generate_bindings() {
        std::array<VkVertexInputBindingDescription, binding_count> bindings{};
        std::size_t count = 0;

        auto has_binding = [&bindings, &count](std::uint32_t binding) {
            for(std::size_t i = 0; i < count; ++i){
                if(bindings[i].binding == binding){
                    return true;
                }
            }
            return false;
        };

        auto append_binding = [&]<auto Attr>() {
            if(has_binding(Attr.binding)){
                return;
            }

            bindings[count++] = VkVertexInputBindingDescription{
                .binding = Attr.binding,
                .stride = binding_stride<Attr.binding>(),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            };
        };

        (append_binding.template operator()<Attrs>(), ...);

        return bindings;
    }

    static constexpr std::array<VkVertexInputBindingDescription, binding_count> bindings = generate_bindings();

    static consteval VkPipelineVertexInputStateCreateInfo get_info() {
        VkPipelineVertexInputStateCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;
        info.vertexBindingDescriptionCount = static_cast<std::uint32_t>(bindings.size());
        info.pVertexBindingDescriptions = bindings.data();
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
