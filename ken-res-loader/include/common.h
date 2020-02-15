#pragma once

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

namespace kr
{
#ifdef _MSC_VER
#include <wchar.h>
	typedef wchar_t fchar_t;
#define _TF(x) L##x
#define fstrcmp(a,b) wcscmp(a,L##b)

#ifdef __IS_KEN_BACKEND_SOUND_DLL
#define KEN_EXTERNAL __declspec(dllexport)
#else
#define KEN_EXTERNAL __declspec(dllimport)
#endif
#else
	typedef char fchar_t;
#define _TF(x) x
#define fstrcmp(a,b) strcmp(a,b)
#define KEN_EXTERNAL

#endif

	class KrbFile;

	struct KrbFileVFTable
	{
		void (*write)(KrbFile* _this, const void* data, size_t size);
		size_t(*read)(KrbFile* _this, void* data, size_t size);
		uint64_t(*tell)(KrbFile* _this);
		void (*seek_set)(KrbFile* _this, uint64_t pos);
		void (*seek_cur)(KrbFile* _this, uint64_t pos);
		void (*seek_end)(KrbFile* _this, uint64_t pos);
		void (*close)(KrbFile* _this);
	};

	class KrbFile
	{
	public:
		const KrbFileVFTable* vftable;
		void* param; // user-defined, krb_set_std_file() will use it as FILE*

		inline void write(const void* data, size_t size) noexcept
		{
			return vftable->write(this, data, size);
		}
		inline size_t read(void* data, size_t size) noexcept
		{
			return vftable->read(this, data, size);
		}
		inline uint64_t tell() noexcept
		{
			return vftable->tell(this);
		}
		inline void seek_set(uint64_t pos) noexcept
		{
			return vftable->seek_set(this, pos);
		}
		inline void seek_cur(uint64_t pos) noexcept
		{
			return vftable->seek_cur(this, pos);
		}
		inline void seek_end(uint64_t pos) noexcept
		{
			return vftable->seek_end(this, pos);
		}
		inline void close() noexcept
		{
			return vftable->close(this);
		}
	};

	bool KEN_EXTERNAL krb_fopen(KrbFile* fp, const fchar_t* path, const fchar_t* mode);

#define KRB_EXTENSION(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))
	enum class KrbExtension:uint32_t
	{
		Invalid = 0,
		ImageBmp = KRB_EXTENSION('B', 'M', 'P', '\0'),
		ImageJpg = KRB_EXTENSION('J', 'P', 'G', '\0'),
		ImageJpeg = KRB_EXTENSION('J', 'P', 'E', 'G'),
		ImagePng = KRB_EXTENSION('P', 'N', 'G', '\0'),
		ImageTga = KRB_EXTENSION('T', 'G', 'A', '\0'),

		SoundWav = KRB_EXTENSION('W', 'A', 'V', '\0'),
		SoundOgg = KRB_EXTENSION('O', 'G', 'G', '\0'),
		SoundOpus = KRB_EXTENSION('O', 'P', 'U', 'S'),
		SoundMp3 = KRB_EXTENSION('M', 'P', '3', '\0'),

		CompressZip = KRB_EXTENSION('Z', 'I', 'P', '\0'),
		Compress7z = KRB_EXTENSION('7', 'Z', '\0', '\0'),
		CompressLzma = KRB_EXTENSION('L', 'Z', 'M', 'A'),
	};

	template <typename C>
	KrbExtension krb_make_extension(const C* extension) noexcept
	{
		uint32_t v = 0;
		uint32_t offset = 0;
		for (;;)
		{
			char chr = *extension++;
			if (chr == (C)'\0') break;
			if ((C)'a' <= chr && chr <= (C)'z')
			{
				chr += (C)'A' - (C)'a';
			}
			else if ((C)'A' <= chr && chr <= (C)'Z')
			{
			}
			else if ((C)'0' <= chr && chr <= (C)'9')
			{
			}
			else
			{
				return KrbExtension::Invalid;
			}
			v |= (uint32_t)chr << offset;
			offset += 8;
			if (offset >= 32) return KrbExtension::Invalid;
		}
		return (KrbExtension)v;
	}

	template <typename C>
	KrbExtension krb_make_extension_from_path(const C* path) noexcept
	{
		const C* p = path;
		for (;;)
		{
			C chr = *p++;
			if (chr == (C)'\0') break;
		}

		for (;;)
		{
			C chr = *--p;
			if (chr == (C)'.') break;
			if (p == path) return KrbExtension::Invalid;
		}
		return krb_make_extension(p+1);
	}

}