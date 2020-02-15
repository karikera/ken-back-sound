
#include <string>
#include <vector>
#include <unordered_set>
#include <assert.h>

#include "kzip.h"
 #include "util.h"

#include <zconf.h>
#include <zlib.h>
 
#include "zlib_contrib/zip.h"
#include "zlib_contrib/unzip.h"

#include "filetime.h"

using namespace kr;

static voidpf ZCALLBACK fopen64_file_func_16(voidpf opaque, const void* filename, int mode);
static uLong ZCALLBACK fread_file_func(voidpf opaque, voidpf stream, void* buf, uLong size);
static uLong ZCALLBACK fwrite_file_func(voidpf opaque, voidpf stream, const void* buf, uLong size);
static ZPOS64_T ZCALLBACK ftell64_file_func(voidpf opaque, voidpf stream);
static long ZCALLBACK fseek64_file_func(voidpf  opaque, voidpf stream, ZPOS64_T offset, int origin);
static int ZCALLBACK fclose_file_func(voidpf opaque, voidpf stream);
static int ZCALLBACK ferror_file_func(voidpf opaque, voidpf stream);

static zlib_filefunc64_def s_filefunc64 = {
	fopen64_file_func_16,
	fread_file_func,
	fwrite_file_func,
	ftell64_file_func,
	fseek64_file_func,
	fclose_file_func,
	ferror_file_func,
};

class UnzipperFile :public KrbCompressEntry
{
public:
	unzFile m_file;

	size_t read(void* dest, size_t size) noexcept(false) override
	{
		assert(size <= 0xffffffff);
		int len = unzReadCurrentFile(m_file, dest, (unsigned int)size);
		if (len < 0) return 0;
		return len;
	}
};

struct Unzipper
{
	unzFile m_file;
	std::unordered_set<std::string> m_directoryCreated;
	KrbCompressCallback* m_callback;

	int extractCurrentFile() noexcept
	{
		char filename_inzip[256];
		int nError = UNZ_OK;
		unz_file_info file_info;

		nError = unzGetCurrentFileInfo(
			m_file,
			&file_info,
			filename_inzip,
			sizeof(filename_inzip),
			nullptr,
			0,
			nullptr,
			0);

		if (nError != UNZ_OK) return nError;

		std::string pathstr = filename_inzip;
		if (pathstr.empty()) return nError;

		UnzipperFile info;
		info.m_file = m_file;
		info.filename = filename_inzip;

		FILETIME localtime;
		if (DosDateTimeToFileTime((uint16_t)(file_info.dosDate >> 16), (uint16_t)file_info.dosDate, &localtime) &&
			LocalFileTimeToFileTime(&localtime, (FILETIME*)&info.filetime))
		{
		}
		else
		{
			GetSystemTimeAsFileTime((FILETIME*)&info.filetime);
		}

		char endschar = pathstr.back();
		if (endschar == '/' || endschar == '\\')
		{
			pathstr.pop_back();
			size_t filelen = pathstr.length();
			filename_inzip[filelen] = '\0';
			info.filenameLength = filelen;
			info.isDirectory = true;
			m_directoryCreated.insert(pathstr);
			m_callback->entry(m_callback, &info);
			return nError;
		}
		nError = unzOpenCurrentFile(m_file);
		if (nError != UNZ_OK) return nError;

		finally {
			unzCloseCurrentFile(m_file);
		};

		std::vector<size_t> pathes;
		for (;;)
		{
			size_t pos = pathstr.find_last_of("/\\");
			if (pos == -1) break;
			pathstr.resize(pos);
			/* some zipfile don't contain directory alone before file */
			auto iter = m_directoryCreated.find(pathstr);
			if (iter == m_directoryCreated.end())
			{
				info.isDirectory = true;
				m_directoryCreated.insert(pathstr);
				pathes.push_back(pos);
			}
		}
		if (!pathes.empty())
		{
			info.isDirectory = true;
			auto end = pathes.rend();
			for (auto iter = pathes.rbegin(); iter != end; ++iter)
			{
				size_t pos = *iter;
				char* sepchr = &filename_inzip[pos];
				char prev = *sepchr;
				*sepchr = '\0';
				info.filenameLength = pos;
				m_callback->entry(m_callback, &info);
				*sepchr = prev;
			}
		}

		info.filenameLength = pathstr.size();
		m_callback->entry(m_callback, &info);
		return UNZ_OK;
	}


};

bool backend::Zip::load(KrbCompressCallback* callback, KrbFile* file) noexcept
{
	Unzipper unzipper;
	unzipper.m_file = unzOpen2_64(file, &s_filefunc64);
	if (unzipper.m_file == nullptr) return false;

	unzipper.m_callback = callback;

	int nError;
	unz_global_info gi;

	nError = unzGetGlobalInfo(unzipper.m_file, &gi);
	if (nError != UNZ_OK) return false;

	for (uLong i = 0; i < gi.number_entry; i++)
	{
		nError = unzipper.extractCurrentFile();
		if (nError != UNZ_OK) return false;

		if ((i + 1) < gi.number_entry)
		{
			nError = unzGoToNextFile(unzipper.m_file);
		}
	}

	unzCloseCurrentFile(unzipper.m_file);
	unzClose(unzipper.m_file);
	return true;
}


static voidpf ZCALLBACK fopen64_file_func_16(voidpf opaque, const void* filename, int mode)
{
	size_t modeidx = -1;
	if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER) == ZLIB_FILEFUNC_MODE_READ)
		modeidx = 0;
	else if (mode & ZLIB_FILEFUNC_MODE_EXISTING)
		modeidx = 1;
	else if (mode & ZLIB_FILEFUNC_MODE_CREATE)
		modeidx = 2;
	return (voidpf)filename;
}

static uLong ZCALLBACK fread_file_func(voidpf opaque, voidpf stream, void* buf, uLong size)
{
	size_t readed = ((KrbFile*)stream)->read(buf, size);
	assert(readed <= 0xffffffff);
	return (uLong)readed;
}

static uLong ZCALLBACK fwrite_file_func(voidpf opaque, voidpf stream, const void* buf, uLong size)
{
	((KrbFile*)stream)->write(buf, size);
	return size;
}

static ZPOS64_T ZCALLBACK ftell64_file_func(voidpf opaque, voidpf stream)
{
	return ((KrbFile*)stream)->tell();
}

static long ZCALLBACK fseek64_file_func(voidpf  opaque, voidpf stream, ZPOS64_T offset, int origin)
{
	switch (origin)
	{
	case ZLIB_FILEFUNC_SEEK_CUR:
		((KrbFile*)stream)->seek_cur(offset);
		return 0;
	case ZLIB_FILEFUNC_SEEK_END:
		((KrbFile*)stream)->seek_end(offset);
		return 0;
	case ZLIB_FILEFUNC_SEEK_SET:
		((KrbFile*)stream)->seek_set(offset);
		return 0;
	default: return -1;
	}
	return 0;
}

static int ZCALLBACK fclose_file_func(voidpf opaque, voidpf stream)
{
	((KrbFile*)stream)->close();
	return 0;
}

static int ZCALLBACK ferror_file_func(voidpf opaque, voidpf stream)
{
	return 0;
}
