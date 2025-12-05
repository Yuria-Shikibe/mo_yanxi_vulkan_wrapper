#pragma once

#include <cassert>

#define LoadFuncPtr(instance, name) \
	[&](){\
	auto ptr = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name));\
	assert(ptr != nullptr);\
	return ptr;\
}()\

