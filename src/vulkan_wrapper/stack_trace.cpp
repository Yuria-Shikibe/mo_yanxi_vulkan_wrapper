module mo_yanxi.stack_trace;

import mo_yanxi.algo.string_parse;
import mo_yanxi.meta_programming;

void mo_yanxi::print_stack_trace(std::ostream& ss, unsigned skip, const std::stacktrace& currentStacktrace){
	std::println(ss, "Stack Trace: ");

	for (const auto & [i, stacktrace] : currentStacktrace
	| std::views::filter([](const std::stacktrace_entry& entry){
		const std::filesystem::path path = entry.source_file();
		return std::filesystem::exists(path) && !path.extension().empty();
	})
	| std::views::drop(skip)
	| std::views::enumerate){
		auto desc = stacktrace.description();
		auto descView = std::string_view{desc};

#if defined(_MSC_VER) || defined(_WIN32)
		//MSVC ONLY
		const auto begin = descView.find('!');
		descView.remove_prefix(begin + 1);
		const auto end = descView.rfind('+');
		descView.remove_suffix(descView.size() - end);
#endif

		std::print(ss, "[{:_>3}] {}({})\n\t", i, stacktrace.source_file(), stacktrace.source_line() - 1);
		std::print(ss, "{}", descView);

		// if(auto rst2 = algo::unwrap(descView, '`'); rst2.is_flat()){
		// 	std::print(ss, "{}", descView);
		// }else{
		// 	static constexpr std::string_view Splitter{"::"};
		// 	for (auto layers = rst2.get_clean_layers(); const auto & basic_string : layers){
		// 		bool spec = basic_string.ends_with(Splitter);
		//
		// 		auto splited = basic_string | std::views::split(Splitter);
		//
		// 		if(spec){
		// 			for(const auto& basic_string_view : splited
		// 				| std::views::transform(mo_yanxi::convert_to<std::string_view>{})
		// 				| std::views::drop_while(mo_yanxi::algo::begin_with_digit)
		// 				| std::views::take(1)
		// 			){
		// 				std::print(ss, "{}", basic_string_view, spec ? "->" : "");
		// 			}
		// 		} else{
		// 			for(const auto& basic_string_view : splited
		// 				| std::views::transform(mo_yanxi::convert_to<std::string_view>{})
		// 				| std::views::drop_while(mo_yanxi::algo::begin_with_digit)
		// 			){
		// 				std::print(ss, "::{}", basic_string_view, spec ? "->" : "");
		// 			}
		// 		}
		// 	}
		// }

		std::println(ss, "\n");
	}

	std::println(ss, "\n");
}
