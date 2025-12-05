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
		VkShaderStageFlagBits declStage{};
		std::vector<std::uint32_t> binary{};
		std::string name{};

	public:
		static constexpr std::array ShaderType{
			std::pair{std::string_view{".vert"}, VK_SHADER_STAGE_VERTEX_BIT},
			std::pair{std::string_view{".frag"}, VK_SHADER_STAGE_FRAGMENT_BIT},
			std::pair{std::string_view{".geom"}, VK_SHADER_STAGE_GEOMETRY_BIT},
			std::pair{std::string_view{".tesc"}, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT},
			std::pair{std::string_view{".tese"}, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT},
			std::pair{std::string_view{".comp"}, VK_SHADER_STAGE_COMPUTE_BIT},
			std::pair{std::string_view{".mesh"}, VK_SHADER_STAGE_MESH_BIT_EXT},
		};

		[[nodiscard]] shader_module(VkDevice device, const std::span<const char> code, VkShaderStageFlagBits stageFlagBits = {}) : device(device), declStage(stageFlagBits){
			createShaderModule(code);
		}

		[[nodiscard]] shader_module(VkDevice device, const std::filesystem::path& path, VkShaderStageFlagBits stageFlagBits = {}) : device(device), name(path.stem().string()), declStage(stageFlagBits){
			createShaderModule(path);
			declShaderStage(path);
		}

		[[nodiscard]] shader_module(VkDevice device, const std::string_view path, VkShaderStageFlagBits stageFlagBits = {}) : shader_module(device, std::filesystem::path{path}, stageFlagBits){

		}

		[[nodiscard]] shader_module() = default;

		[[nodiscard]] std::string_view get_name() const noexcept{
			return name;
		}

		[[nodiscard]] VkDevice get_device() const noexcept{
			return device;
		}

		VkPipelineShaderStageCreateInfo get_create_info(
			VkShaderStageFlagBits stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM,
			const std::string_view entry = "main",
			const VkSpecializationInfo* specializationInfo = nullptr
			) const{

			if(declStage){
				if(stage == VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM){
					stage = declStage;
				}else if(declStage != stage){
					throw vk_error(VK_ERROR_FEATURE_NOT_PRESENT, "Shader State Mismatch!");
				}
			}else{
				if(stage == VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM){
					throw vk_error(VK_ERROR_FEATURE_NOT_PRESENT, "Shader Stage must be explicit specified!");
				}
			}

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
			declShaderStage(path);
		}

		void declShaderStage(const std::filesystem::path& path){
			if(declStage)return;
			const auto fileName = path.filename().string();

			for (const auto & [type, stage] : ShaderType){
				if(fileName.contains(type)){
					declStage = stage;
					break;
				}
			}
		}

		template <std::ranges::contiguous_range Rng>
		void createShaderModule(const Rng& code){
			VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
			createInfo.codeSize = std::ranges::size(code) * sizeof(std::ranges::range_value_t<Rng>);
			createInfo.pCode = reinterpret_cast<const std::uint32_t*>(std::ranges::data(code));

			if(auto rst = vkCreateShaderModule(device, &createInfo, nullptr, &handle)){
				throw vk_error(rst, "Failed to create shader module!");
			}

			binary = std::vector<std::uint32_t>{std::from_range, code};
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

		void set_no_deduced_stage() noexcept{
			declStage = {};
		}

		void set_stage(VkShaderStageFlagBits stage) noexcept{
			declStage = stage;
		}
	};

	struct shader_chain{
		std::vector<VkPipelineShaderStageCreateInfo> chain{};

		void push(const std::initializer_list<const shader_module*> args){
			chain.reserve(chain.size() + args.size());

			for (const auto & arg : args){
				chain.push_back(arg->get_create_info());
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
			const std::initializer_list<const shader_module*> args){
			push(args);
		}

		template <typename ...T>
			requires (std::same_as<T, shader_module> && ...)
		[[nodiscard]] explicit(false) shader_chain(
			const T& ...args){
			chain.resize(sizeof...(T));

			[&, this] <std::size_t ...Idx>(std::index_sequence<Idx...>){
				((chain[Idx] = static_cast<const shader_module&>(args).get_create_info()), ...);
			}(std::make_index_sequence<sizeof...(T)>{});
		}

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
	};
}
