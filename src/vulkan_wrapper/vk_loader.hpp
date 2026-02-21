#pragma once

#include <cassert>

#define LoadFuncPtr(instance, name) \
	[&](){\
	auto ptr = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name));\
	assert(ptr != nullptr);\
	return ptr;\
}()\

#define DEFINE_FUNC_PTR(name) std::add_pointer_t<decltype(name)> _PFN_##name = nullptr

#define LoadFuncPtrByName(instance, name) [&]{ \
	using Ty = decltype(name); /*used for IDE hint*/ \
	_PFN_##name = LoadFuncPtr(instance, name); \
}()
