export module mo_yanxi.vk.util:concepts;

import std;

namespace mo_yanxi::vk{

	export
	template <typename Rng, typename Value>
	concept contigious_range_of = requires{
		requires std::ranges::sized_range<Rng>;
		requires std::ranges::contiguous_range<Rng>;
		requires std::convertible_to<Value, std::ranges::range_value_t<Rng>>;
	};

	export
	template <typename Rng, typename Value>
	concept range_of = requires{
		requires std::ranges::range<Rng>;
		requires std::same_as<Value, std::ranges::range_value_t<Rng>>;
	};
}
