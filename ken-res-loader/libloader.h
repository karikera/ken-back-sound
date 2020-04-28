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
};

#define KRL_BEGIN_COMMON(clsname, dllname) class clsname:public Library{ \
	public: clsname() noexcept:Library(dllname){}\
	static clsname* getInstance() { return _getInstance<clsname>(); }

#ifdef _DEBUG
#define KRL_BEGIN(clsname, debug, release) KRL_BEGIN_COMMON(clsname, debug)
#else
#define KRL_BEGIN(clsname, debug, release) KRL_BEGIN_COMMON(clsname, release)
#endif

#define KRL_IMPORT(name) decltype(::name)* const name = get(#name);
#define KRL_END() };