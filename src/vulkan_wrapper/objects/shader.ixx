module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk:shader;

import mo_yanxi.handle_wrapper;
import mo_yanxi.vk.util;
import mo_yanxi.io;
import std;

export namespace mo_yanxi::vk{
	struct shader_module : exclusive_handle<VkShaderModule>{
	private:
		exclusive_handle_member<VkDevice> device{};
		std::vector<std::uint32_t> binary{};
		std::string name{};

	public:
		template <typename T>
			requires (std::is_trivially_copyable_v<T>)
		[[nodiscard]] shader_module(VkDevice device, const std::span<const T> code) : device(device){
			createShaderModule(code);
		}

		[[nodiscard]] shader_module(VkDevice device, const std::filesystem::path& path) : device(device), name(path.stem().string()){
			createShaderModule(path);
		}

		[[nodiscard]] shader_module(VkDevice device, const std::string_view path) : shader_module(device, std::filesystem::path{path}){

		}

		[[nodiscard]] shader_module() = default;

		[[nodiscard]] std::string_view get_name() const noexcept{
			return name;
		}

		[[nodiscard]] VkDevice get_device() const noexcept{
			return device;
		}

		VkPipelineShaderStageCreateInfo get_create_info(
			VkShaderStageFlagBits stage,
			const std::string_view entry = "main",
			const VkSpecializationInfo* specializationInfo = nullptr
			) const{


			const VkPipelineShaderStageCreateInfo vertShaderStageInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = stage,
				.module = handle,
				.pName = entry.data(),
				.pSpecializationInfo = specializationInfo
			};

			return vertShaderStageInfo;
		}

		VkPipelineShaderStageCreateInfo get_create_info_compute(
			const VkSpecializationInfo* specializationInfo = nullptr,
			const std::string_view entry = "main"
			) const{

			return get_create_info(VK_SHADER_STAGE_COMPUTE_BIT, entry, specializationInfo);
		}


		~shader_module(){
			if(device)vkDestroyShaderModule(device, handle, nullptr);
		}

		shader_module(const shader_module& other) = delete;
		shader_module(shader_module&& other) noexcept = default;
		shader_module& operator=(const shader_module& other) = delete;
		shader_module& operator=(shader_module&& other) noexcept = default;

	private:

		void load(VkDevice device, const std::string_view path){
			if(handle && this->device)vkDestroyShaderModule(device, handle, nullptr);

			this->device = device;
			createShaderModule(path);
		}


		template <std::ranges::contiguous_range Rng>
		void createShaderModule(const Rng& code){
			auto spn = std::span{code.begin(), code.end()};
			std::span<const std::byte> bspan = std::as_bytes(spn);
			auto sz = bspan.size();
			auto u32sz = (sz + (sizeof(std::uint32_t) -1)) / sizeof(std::uint32_t);
			binary.resize(u32sz);
			std::memcpy(binary.data(), bspan.data(), bspan.size());

			VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
			createInfo.codeSize = bspan.size();
			createInfo.pCode = std::ranges::data(binary);

			if(auto rst = vkCreateShaderModule(device, &createInfo, nullptr, &handle)){
				throw vk_error(rst, "Failed to create shader module!");
			}
		}

		void createShaderModule(const std::filesystem::path& file){
			if(!std::filesystem::exists(file)){
				throw unqualified_error{"Shader File Not Fount"};
			}

			createShaderModule(io::read_bytes<std::uint32_t>(file).value());
		}

	public:
		[[nodiscard]] std::span<const std::uint32_t> get_binary() const noexcept{
			return binary;
		}

	};

	struct shader_chain{
		std::vector<VkPipelineShaderStageCreateInfo> chain{};

		void push(
			const std::initializer_list<const shader_module*> args,
			const std::initializer_list<VkShaderStageFlagBits> stage
			){
			chain.reserve(chain.size() + args.size());

			for (const auto & [arg, stg] : std::views::zip(args, stage)){
				chain.push_back(arg->get_create_info(stg));
			}
		}

		void push(const std::initializer_list<VkPipelineShaderStageCreateInfo> args){
			chain = args;
		}

		void push(const std::span<const VkPipelineShaderStageCreateInfo> args){
			chain.assign_range(args);
		}

		[[nodiscard]] shader_chain() = default;

		[[nodiscard]] explicit(false) shader_chain(
			const std::initializer_list<const shader_module*> args,
			const std::initializer_list<VkShaderStageFlagBits> stage
			){
			push(args, stage);
		}
		//
		// template <typename ...T>
		// 	requires (std::same_as<T, shader_module> && ...)
		// [[nodiscard]] explicit(false) shader_chain(
		// 	const T& ...args){
		// 	chain.resize(sizeof...(T));
		//
		// 	[&, this] <std::size_t ...Idx>(std::index_sequence<Idx...>){
		// 		((chain[Idx] = static_cast<const shader_module&>(args).get_create_info()), ...);
		// 	}(std::make_index_sequence<sizeof...(T)>{});
		// }

		[[nodiscard]] explicit(false) shader_chain(
			const std::initializer_list<VkPipelineShaderStageCreateInfo> args) : chain{args}{}


		template <std::ranges::input_range Rng>
			requires (std::convertible_to<std::ranges::range_value_t<Rng>, VkPipelineShaderStageCreateInfo>)
		[[nodiscard]] explicit(false) shader_chain(Rng&& args) : chain{std::from_range, std::forward<Rng>(args)}{}


		[[nodiscard]] const VkPipelineShaderStageCreateInfo* data() const noexcept{
			return chain.data();
		}

		[[nodiscard]] std::uint32_t size() const noexcept{
			return static_cast<std::uint32_t>(chain.size());
		}

		auto& operator[](this auto& self, std::size_t index) noexcept{
			return self.chain[index];
		}
	};
}
