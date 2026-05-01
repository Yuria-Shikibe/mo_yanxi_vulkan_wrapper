module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.util.uniform;

export import mo_yanxi.math.matrix3;
import std;

namespace mo_yanxi::vk{
template <typename T, typename... Ty>
struct Combinable : std::bool_constant<requires{
		requires std::conjunction_v<std::is_same<T, Ty>...> ||
		(std::conjunction_v<std::is_scalar<Ty>...> && std::is_scalar_v<T> && ((sizeof(Ty) == 4) && ... && (sizeof(T) ==
			4)));
	}>{
};

template <std::size_t size>
struct UniformPadding{
	std::array<std::byte, size> padding{};
};

template <>
struct UniformPadding<0>{
};

consteval std::size_t nextMultiplesOf(const std::size_t cur, const std::size_t base){
	if(const auto dst = cur % base){
		return cur + (base - dst);
	} else{
		return cur;
	}
}

template <typename... T>
	requires (std::conjunction_v<std::is_trivially_copy_assignable<T>...> && Combinable<T...>::value)
struct UniformValuePadder : UniformPadding<nextMultiplesOf(sizeof(std::tuple<T...>), 16) - sizeof(std::tuple<T...>)>{
	using value_type = std::tuple<T...>;
	value_type value{};

	[[nodiscard]] constexpr UniformValuePadder() = default;

	[[nodiscard]] constexpr UniformValuePadder(const T&... t) noexcept : value{t...}{
	}

	UniformValuePadder& operator=(const std::tuple_element_t<0, value_type>& other) requires (sizeof...(T) == 1){
		value = other;
		return *this;
	}
}; //TODO support structured binding


export
struct padded_mat3{
	math::mat3::vec3_t c1;
	math::mat3::floating_point_t p1;

	math::mat3::vec3_t c2;
	math::mat3::floating_point_t p2;

	math::mat3::vec3_t c3;
	math::mat3::floating_point_t p3;

	[[nodiscard]] constexpr padded_mat3() = default;

	[[nodiscard]] constexpr explicit(false) padded_mat3(const math::mat3& mat) :
	c1{mat.c1}, c2{mat.c2}, c3{mat.c3}{
	}

	constexpr friend bool operator==(const padded_mat3& lhs, const padded_mat3& rhs) noexcept{
		return lhs.c1 == rhs.c1
			&& lhs.c2 == rhs.c2
			&& lhs.c3 == rhs.c3;
	}

	constexpr explicit (false) operator math::mat3() const noexcept{
		return {c1, c2, c3};
	}
};

struct UniformProjectionBlock{
	padded_mat3 mat3{};
	float v{};

	constexpr friend bool operator==(const UniformProjectionBlock&, const UniformProjectionBlock&) noexcept = default;
	constexpr friend bool operator!=(const UniformProjectionBlock&, const UniformProjectionBlock&) noexcept = default;
};
}
