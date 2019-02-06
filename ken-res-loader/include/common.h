#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
#define _KR_C_MODE_BEGIN extern "C" {
#define _KR_C_MODE_END }
#else
#define _KR_C_MODE_BEGIN
#define _KR_C_MODE_END
#endif

_KR_C_MODE_BEGIN

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

typedef struct _krb_file_vtable_t krb_file_vtable_t;
typedef struct _krb_file_t krb_file_t;

struct _krb_file_vtable_t
{
	void (*write)(krb_file_t* _this, const void* data, size_t size);
	size_t(*read)(krb_file_t* _this, void* data, size_t size);
	uint64_t (*tell)(krb_file_t* _this);
	void (*seek_set)(krb_file_t* _this, uint64_t pos);
	void (*seek_cur)(krb_file_t* _this, uint64_t pos);
	void (*seek_end)(krb_file_t* _this, uint64_t pos);
	void (*close)(krb_file_t* _this);
};

struct _krb_file_t
{
	const krb_file_vtable_t* vtable;
	void* param; // user-defined, krb_set_std_file() will use it as FILE*
};


bool KEN_EXTERNAL krb_fopen_std_file(krb_file_t* fp, const fchar_t* path, const fchar_t* mode);

inline void krb_fwrite(krb_file_t* file, const void* data, size_t size) {
	return file->vtable->write(file, data, size);
}
inline size_t krb_fread(krb_file_t* file, void* data, size_t size) {
	return file->vtable->read(file, data, size);
}
inline uint64_t krb_ftell(krb_file_t* file)
{
	return file->vtable->tell(file);
}
inline void krb_fseek_set(krb_file_t* file, uint64_t pos)
{
	return file->vtable->seek_set(file, pos);
}
inline void krb_fseek_cur(krb_file_t* file, uint64_t pos)
{
	return file->vtable->seek_cur(file, pos);
}
inline void krb_fseek_end(krb_file_t* file, uint64_t pos)
{
	return file->vtable->seek_end(file, pos);
}
inline void krb_fclose(krb_file_t* file) {
	return file->vtable->close(file);
}


#define KRB_EXTENSION(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))
enum krb_extension_t
{
	ExtensionInvalid = 0,
	ExtensionImageBmp = KRB_EXTENSION('B', 'M', 'P', '\0'),
	ExtensionImageJpg = KRB_EXTENSION('J', 'P', 'G', '\0'),
	ExtensionImageJpeg = KRB_EXTENSION('J', 'P', 'E', 'G'),
	ExtensionImagePng = KRB_EXTENSION('P', 'N', 'G', '\0'),
	ExtensionImageTga = KRB_EXTENSION('T', 'G', 'A', '\0'),

	ExtensionSoundWav = KRB_EXTENSION('W', 'A', 'V', '\0'),
	ExtensionSoundOgg = KRB_EXTENSION('O', 'G', 'G', '\0'),
	ExtensionSoundOpus = KRB_EXTENSION('O', 'P', 'U', 'S'),
	ExtensionSoundMp3 = KRB_EXTENSION('M', 'P', '3', '\0'),

	ExtensionCompressZip = KRB_EXTENSION('Z', 'I', 'P', '\0'),
	ExtensionCompress7z = KRB_EXTENSION('7', 'Z', '\0', '\0'),
	ExtensionCompressLzma = KRB_EXTENSION('L', 'Z', 'M', 'A'),
};

inline krb_extension_t krb_make_extension(const char* extension)
{
	uint32_t v = 0;
	uint32_t offset = 0;
	for (;;)
	{
		char chr = *extension++;
		if (chr == '\0') break;
		if ('a' <= chr && chr <= 'z')
		{
			chr += 'A' - 'a';
		}
		else if ('A' <= chr && chr <= 'Z')
		{
		}
		else if ('0' <= chr && chr <= '9')
		{
		}
		else
		{
			return ExtensionInvalid;
		}
		v |= (uint32_t)chr << offset;
		offset += 8;
		if (offset >= 32) return ExtensionInvalid;
	}
	return (krb_extension_t)v;
}

inline krb_extension_t krb_make_extension_16(const char16_t* extension)
{
	uint32_t v = 0;
	uint32_t offset = 0;
	for (;;)
	{
		char16_t chr = *extension++;
		if (chr == u'\0') break;
		if (u'a' <= chr && chr <= u'z')
		{
			chr += u'A' - u'a';
		}
		else if (u'A' <= chr && chr <= u'Z')
		{
		}
		else if (u'0' <= chr && chr <= u'9')
		{
		}
		else
		{
			return ExtensionInvalid;
		}
		v |= (uint32_t)chr << offset;
		offset += 8;
		if (offset >= 32) return ExtensionInvalid;
	}
	return (krb_extension_t)v;
}

_KR_C_MODE_END
