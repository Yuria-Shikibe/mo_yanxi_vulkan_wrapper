module;

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cassert>

export module mo_yanxi.vk:descriptor_heap;

export import :resources;
export import :ext;

namespace mo_yanxi::vk{

constexpr auto align_up(VkDeviceSize value, VkDeviceSize alignment) noexcept {
	return (value + alignment - 1) & ~(alignment - 1);
}

export
[[nodiscard]] VkPhysicalDeviceDescriptorHeapPropertiesEXT GetDescriptorHeapProperties(VkPhysicalDevice physicalDevice) {
	VkPhysicalDeviceDescriptorHeapPropertiesEXT heapProps{};
	heapProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT;
	heapProps.pNext = nullptr;

	VkPhysicalDeviceProperties2 deviceProps2{};
	deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProps2.pNext = &heapProps;

	vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProps2);

	return heapProps;
}

struct sampler_heap_property{
	VkDeviceSize max_heap_size;
	VkDeviceSize min_reserved_size;

	VkDeviceSize descriptor_entry_size;
	VkDeviceSize descriptor_entry_alignment;
};

struct resource_heap_property{
	VkDeviceSize max_heap_size;
	VkDeviceSize min_reserved_size;

	VkDeviceSize descriptor_image_size;
	VkDeviceSize descriptor_buffer_size;
	VkDeviceSize descriptor_image_alignment;
	VkDeviceSize descriptor_buffer_alignment;
};

template <typename T, std::size_t N = 8, std::size_t CertainSize = std::dynamic_extent>
class hybrid_buffer {
private:
	union {
		std::array<T, N> static_buf_;
		std::vector<T> dynamic_buf_;
	};
	std::size_t size_;

	constexpr bool is_dynamic_() const noexcept{
		return size_ > N;
	}

public:
	constexpr explicit hybrid_buffer(std::size_t size) : size_(size) {
		if (is_dynamic_()) {
			// 当元素数量大于 N 时，激活并构造 std::vector
			std::construct_at(&dynamic_buf_, size);
		} else {
			// 否则，激活 std::array 成员
			std::construct_at(&static_buf_);
		}
	}

	constexpr ~hybrid_buffer() {
		if (is_dynamic_()) {
			// 销毁 std::vector
			std::destroy_at(&dynamic_buf_);
		} else {
			// 当 T 不可平凡析构时，手动调用 std::array 的析构逻辑
			if constexpr (!std::is_trivially_destructible_v<T>) {
				std::destroy_at(&static_buf_);
			}
		}
	}

	// 禁用拷贝和移动，防止 union 状态和内存管理的意外错误
	constexpr hybrid_buffer(const hybrid_buffer&) = delete;
	constexpr hybrid_buffer& operator=(const hybrid_buffer&) = delete;

	constexpr T* data() noexcept {
		return is_dynamic_() ? dynamic_buf_.data() : static_buf_.data();
	}

	constexpr const T* data() const noexcept {
		return is_dynamic_() ? dynamic_buf_.data() : static_buf_.data();
	}

	constexpr auto begin(this auto& self) noexcept{
		return self.data();
	}

	constexpr auto end(this auto& self) noexcept{
		return self.data() + self.size_;
	}
};

[[nodiscard]] constexpr sampler_heap_property make_sampler_heap_property(const VkPhysicalDeviceDescriptorHeapPropertiesEXT& props, bool with_embedded = false) noexcept {
	return sampler_heap_property{
		.max_heap_size = props.maxSamplerHeapSize,
		.min_reserved_size = with_embedded ? props.minSamplerHeapReservedRangeWithEmbedded : props.minSamplerHeapReservedRange,
		.descriptor_entry_size = props.samplerDescriptorSize,
		.descriptor_entry_alignment = props.samplerDescriptorAlignment
	};
}

[[nodiscard]] constexpr resource_heap_property make_resource_heap_property(const VkPhysicalDeviceDescriptorHeapPropertiesEXT& props) noexcept {
	return resource_heap_property{
		.max_heap_size = props.maxResourceHeapSize,
		.min_reserved_size = props.minResourceHeapReservedRange,
		.descriptor_image_size = props.imageDescriptorSize,
		.descriptor_buffer_size = props.bufferDescriptorSize,
		.descriptor_image_alignment = props.imageDescriptorAlignment,
		.descriptor_buffer_alignment = props.bufferDescriptorAlignment
	};
}

export
struct sampler_descriptor_heap : buffer_cpu_to_gpu{
private:
	sampler_heap_property prop_{};
	std::uint32_t capacity_{};
	std::uint32_t size_{};

public:
	sampler_descriptor_heap() = default;

	sampler_descriptor_heap(const allocator_usage& allocator, std::uint32_t max_item_count, bool embedded = false)
		: buffer_cpu_to_gpu(), prop_(make_sampler_heap_property(GetDescriptorHeapProperties(allocator.get_physical_device()), embedded)), capacity_(max_item_count) {
		prop_.min_reserved_size = align_up(prop_.min_reserved_size, prop_.descriptor_entry_alignment);
		const auto required_size = prop_.min_reserved_size + max_item_count * prop_.descriptor_entry_size;

		if(required_size > prop_.max_heap_size){
			throw std::bad_alloc{};
		}

		this->buffer_cpu_to_gpu::operator=(buffer_cpu_to_gpu{
			allocator, required_size, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT
		});
	}

	sampler_descriptor_heap(const allocator_usage& allocator, const std::initializer_list<VkSamplerCreateInfo> create_infos, bool embedded = false)
		: sampler_descriptor_heap(allocator, create_infos.size(), embedded){
		push_back(create_infos);
	}

	[[nodiscard]] VkDeviceSize get_reserved_size() const{
		return prop_.min_reserved_size;
	}

	[[nodiscard]] VkDeviceSize get_user_space_size() const{
		return get_size() - get_reserved_size();
	}

	std::uint32_t allocate(std::uint32_t count = 1) {
		if (size_ + count > capacity_) {
			throw std::bad_alloc();
		}
		std::uint32_t start_idx = size_;
		size_ += count;
		return start_idx;
	}

	void pop_back(std::uint32_t count = 1) noexcept {
		assert(size_ >= count && "Cannot pop more elements than currently allocated!");
		size_ -= count;
	}

	void clear() noexcept {
		size_ = 0;
	}


	VkDeviceSize get_offset(std::uint32_t index) const noexcept {
		assert(index < size_);
		return prop_.min_reserved_size + index * prop_.descriptor_entry_size;
	}

	void* get_host_ptr(std::uint32_t index) const noexcept {
		assert(index < size_);
		return this->get_mapped_ptr() + get_offset(index);
	}

	VkHostAddressRangeEXT get_host_address_range(std::uint32_t index) const noexcept {
		return VkHostAddressRangeEXT{
			.address = get_host_ptr(index),
			.size = prop_.descriptor_entry_size
		};
	}

	template <std::ranges::input_range IdxRng = const std::initializer_list<std::uint32_t>, std::output_iterator<VkHostAddressRangeEXT> OutIt>
		requires (std::convertible_to<std::ranges::range_reference_t<IdxRng>, std::uint32_t>)
	OutIt get_host_address_range(IdxRng&& rng, OutIt out){
		auto src = this->get_mapped_ptr();
		for(std::uint32_t index : rng){
			*out = VkHostAddressRangeEXT{
				.address = src + get_offset(index),
				.size = prop_.descriptor_entry_size
			};
			++out;
		}
		return out;
	}

	[[nodiscard]] VkBindHeapInfoEXT get_bind_info() const noexcept {
		return VkBindHeapInfoEXT{
			.sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
			.pNext = nullptr,
			.heapRange = {
				.address = get_address(),
				.size = get_size()
			},
			.reservedRangeOffset = 0, // 规范强制要求必须为 0
			.reservedRangeSize = prop_.min_reserved_size
		};
	}

	void push_back(std::span<const VkSamplerCreateInfo> create_infos){
		push_back_samplers_(create_infos);
	}

	void push_back(VkSamplerCreateInfo create_infos){
		push_back({&create_infos, 1});
	}

	void push_back(std::initializer_list<VkSamplerCreateInfo> create_infos){
		push_back(std::span{create_infos});
	}

protected:
	void push_back_samplers_(std::span<const VkSamplerCreateInfo> create_infos){
		hybrid_buffer<VkHostAddressRangeEXT, 8> buffer{create_infos.size()};

		const auto begin = buffer.data();
		auto begin_idx = this->allocate(create_infos.size());
		this->get_host_address_range(std::views::iota(begin_idx, begin_idx + create_infos.size()), begin);
		vk::writeSamplerDescriptorsEXT(get_device(), create_infos.size(), create_infos.data(), begin);
	}

	void push_back_resources_(std::span<const VkResourceDescriptorInfoEXT> create_infos){
		hybrid_buffer<VkHostAddressRangeEXT, 8> buffer{create_infos.size()};

		const auto begin = buffer.data();
		auto begin_idx = this->allocate(create_infos.size());
		this->get_host_address_range(std::views::iota(begin_idx, begin_idx + create_infos.size()), begin);
		vk::writeResourceDescriptorsEXT(get_device(), create_infos.size(), create_infos.data(), begin);
	}
};

export
enum class heap_section_type {
	image,
	buffer
};

export
struct heap_section {
	std::uint32_t max_capacity;
	heap_section_type type;
};

/**
 * @brief 根据资源属性和段落信息，计算每个 section 的起始偏移量
 * @tparam R 输入的 range 范围类型 (例如 std::vector<section> 或 std::span<section>)
 * @tparam OutIt 输出迭代器类型 (接收 std::uint32_t)
 * @param props 堆属性（包含尺寸和对齐要求）
 * @param sections 输入的段落划分区间
 * @param out 输出迭代器，用于输出每个段落的起始下标
 * @return VkDeviceSize 返回这块内存区域所需的总大小（包含所有的 padding）
 */
template <std::ranges::input_range R, std::output_iterator<std::uint32_t> OutIt>
	requires (std::convertible_to<std::ranges::range_reference_t<R>, heap_section>)
VkDeviceSize calculate_section_offsets(const resource_heap_property& props,
	R&& sections,
	OutIt out){
	VkDeviceSize current_offset{};

	for (const heap_section& sec : sections) {
		VkDeviceSize item_size;
		VkDeviceSize alignment;

		switch(sec.type){
		case heap_section_type::image :{
			item_size = props.descriptor_image_size;
			alignment = props.descriptor_image_alignment;
			break;
		}
		case heap_section_type::buffer :{
			item_size = props.descriptor_buffer_size;
			alignment = props.descriptor_buffer_alignment;
			break;
		}
		default : std::unreachable();
		}

		const VkDeviceSize aligned_offset = align_up(current_offset, std::max(item_size, alignment));

		*out++ = static_cast<std::uint32_t>(aligned_offset / item_size);
		current_offset = aligned_offset + (static_cast<VkDeviceSize>(sec.max_capacity) * item_size);
	}

	return current_offset;
}

export
struct resource_descriptor_heap : buffer_cpu_to_gpu{
	struct storage_subrange{
		std::uint32_t start_index;
		std::uint32_t max_capacity;
	};
private:
	resource_heap_property prop_{};
	std::vector<storage_subrange> subranges_{};

public:
	resource_descriptor_heap() = default;

	template <std::ranges::forward_range R = const std::initializer_list<heap_section>>
		requires (std::convertible_to<std::ranges::range_reference_t<R>, heap_section>)
	resource_descriptor_heap(const allocator_usage& allocator, R&& sections)
		: buffer_cpu_to_gpu(),
		prop_(make_resource_heap_property(GetDescriptorHeapProperties(allocator.get_physical_device()))), subranges_(
			std::from_range, sections | std::views::transform([](const heap_section& s) -> storage_subrange{
				return {.max_capacity = s.max_capacity};
			})){

		auto totalSize = prop_.min_reserved_size + vk::calculate_section_offsets(prop_, std::forward<R>(sections), (subranges_ | std::views::transform(&storage_subrange::start_index)).begin());

		if(totalSize > prop_.max_heap_size){
			throw std::bad_alloc{};
		}

		this->buffer_cpu_to_gpu::operator=(buffer_cpu_to_gpu{
			allocator, totalSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT
		});
	}

	[[nodiscard]] VkDeviceSize get_reserved_size() const{
		return prop_.min_reserved_size;
	}

	[[nodiscard]] VkDeviceSize get_user_space_size() const{
		return get_size() - get_reserved_size();
	}

	[[nodiscard]] VkBindHeapInfoEXT get_bind_info() const noexcept {
		return VkBindHeapInfoEXT{
			.sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
			.pNext = nullptr,
			.heapRange = {
				.address = get_address(),
				.size = get_size()
			},
			.reservedRangeOffset = 0, // 规范强制要求必须为 0
			.reservedRangeSize = prop_.min_reserved_size
		};
	}

};

}