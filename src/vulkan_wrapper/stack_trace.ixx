module;

#include <version>

export module mo_yanxi.stack_trace;

import std;

export namespace mo_yanxi{
#if defined(__cpp_lib_stacktrace)
	void print_stack_trace(
		std::ostream& ss,
		unsigned skip = 1,
		const std::stacktrace& currentStacktrace = std::stacktrace::current()
	);
#else
	inline void print_stack_trace(std::ostream& ss) {}
#endif
}
