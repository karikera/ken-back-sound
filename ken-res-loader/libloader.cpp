#include "libloader.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


AnyPointer::AnyPointer(void* ptr) noexcept
	:m_ptr(ptr)
{
}

Library::Library(const wchar_t* dllname) noexcept
	:m_module(LoadLibraryW(dllname))
{
}

AnyPointer Library::get(const char* funcname) noexcept
{
	return GetProcAddress((HMODULE)m_module, funcname);
}
bool Library::isLoaded() noexcept
{
	return m_module != nullptr;
}
