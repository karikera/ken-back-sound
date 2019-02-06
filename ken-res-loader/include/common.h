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

_KR_C_MODE_END
