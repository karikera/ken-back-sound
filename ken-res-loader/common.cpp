
#define __USE_FILE_OFFSET64
#define __USE_LARGEFILE64
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BIT 64

#include "include/common.h"

#ifdef _DEBUG
#pragma comment(lib, "zlibd.lib")
#else
#pragma comment(lib, "zlib.lib")
#endif

const krb_file_vtable_t vtable = {
	[](krb_file_t * fp, const void* data, size_t size) {
		fwrite(data, 1, size, (FILE*)fp->param);
	},
	[](krb_file_t * fp, void* data, size_t size)->size_t {
		return fread(data, 1, size, (FILE*)fp->param);
	},
	[](krb_file_t * fp)->uint64_t {
#ifdef _MSC_VER
		return _ftelli64((FILE*)fp->param);
#else
		return ftello64((FILE*)fp->param);
#endif
	},
	[](krb_file_t * fp, uint64_t pos) {
#ifdef _MSC_VER
		_fseeki64((FILE*)fp->param, pos, SEEK_SET);
#else
		fseeko64((FILE*)fp->param, pos, SEEK_SET);
#endif
	},
	[](krb_file_t * fp, uint64_t pos) {
#ifdef _MSC_VER
		_fseeki64((FILE*)fp->param, pos, SEEK_CUR);
#else
		fseeko64((FILE*)fp->param, pos, SEEK_CUR);
#endif
	},
	[](krb_file_t * fp, uint64_t pos) {
#ifdef _MSC_VER
		_fseeki64((FILE*)fp->param, pos, SEEK_END);
#else
		fseeko64((FILE*)fp->param, pos, SEEK_END);
#endif
	},
	[](krb_file_t * fp){
		fclose((FILE*)fp->param);
	}
};

bool KEN_EXTERNAL krb_fopen_std_file(krb_file_t* fp, const fchar_t* path, const fchar_t* mode)
{
	fp->param = nullptr;
#ifdef _MSC_VER
	_wfopen_s((FILE **)&fp->param, path, mode);
#else
	fp->param = fopen64(path, mode);
#endif
	if (!fp->param) return false;
	fp->vtable = &vtable;
	return true;
}
