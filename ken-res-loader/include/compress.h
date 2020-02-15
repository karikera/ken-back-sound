#pragma once

#include "common.h"

namespace kr
{
	// same with Windows FILETIME
	using filetime_t = uint64_t;

	class KrbCompressEntry
	{
	public:
		const char* filename;
		size_t filenameLength;
		bool isDirectory;
		filetime_t filetime;

		virtual size_t read(void* dest, size_t size) noexcept(false) = 0;
	};
	class KrbCompressCallback
	{
	public:
		void (*entry)(KrbCompressCallback* _this, KrbCompressEntry* _info);
	};

	/*class KrbCompress
	{
	public:
		virtual void addFile(const char* filename, KrbFile* file) noexcept = 0;
	};*/

	bool KEN_EXTERNAL krb_load_compress(KrbExtension extension, KrbCompressCallback* callback, KrbFile* file) noexcept;
	// KrbCompress* KEN_EXTERNAL krb_save_compress(KrbExtension extension, KrbFile* file);
}
