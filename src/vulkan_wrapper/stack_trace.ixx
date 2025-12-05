export module mo_yanxi.stack_trace;

import std;

//TODO shits
export namespace mo_yanxi{
	void print_stack_trace(std::ostream& ss, unsigned skip = 1, const std::stacktrace& currentStacktrace = std::stacktrace::current());
}
