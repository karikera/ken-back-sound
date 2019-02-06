#pragma once

#include <stdint.h>

constexpr uint32_t makeSignature(const char* str, size_t size)
{
	return size == 0 ? 0 : ((makeSignature(str + 1, size - 1) << 8) | *str);
}
constexpr uint32_t operator ""_sig(const char* str, size_t size)
{
	return makeSignature(str, size);
}
