#pragma once



class AnyPointer
{
private:
	void* const m_ptr;

public:
	AnyPointer(void* ptr) noexcept;

	template <typename T>
	operator T* () noexcept
	{
		return (T*)m_ptr;
	}
};

class Library
{
public:
#ifdef WIN32
	Library(const wchar_t* dllname) noexcept;
private:
	bool isLoaded() noexcept;

protected:
	AnyPointer get(const char* funcname) noexcept;

	template <typename T>
	static T* _getInstance() noexcept
	{
		static T instance;
		if (!instance.isLoaded()) return nullptr;
		return &instance;
	}

private:
	void* m_module;
#else
	Library() noexcept;
#endif
};

#ifdef WIN32
#define KRL_BEGIN_COMMON(clsname, dllname) class clsname:public Library{ \
	public: clsname() noexcept:Library(dllname){}\
	static clsname* getInstance() { return _getInstance<clsname>(); }
#define KRL_IMPORT(name) decltype(::name)* const name = get(#name);
#else

#define KRL_BEGIN_COMMON(clsname, dllname) class clsname:public Library{ \
	public: clsname() noexcept{}\
	static const clsname instance;
#define KRL_IMPORT(name) decltype(::name)* const name = ::name;

#endif

#ifdef _DEBUG
#define KRL_BEGIN(clsname, debug, release) KRL_BEGIN_COMMON(clsname, debug)
#else
#define KRL_BEGIN(clsname, debug, release) KRL_BEGIN_COMMON(clsname, release)
#endif

#define KRL_END() };

#ifdef WIN32

#define KRL_USING(libname, name, error_return) libname* const name = libname::getInstance(); if (!name) return error_return;

#else

#define KRL_USING(libname, name, error_return) const libname* const name = &libname::instance;

#endif