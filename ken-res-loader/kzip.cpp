//#include "zip.h"
//
//#include <KR3/wl/windows.h>
//#include <KR3/io/selfbufferedstream.h>
//#include <KRUtil/fs/file.h>
//#include <KRUtil/logger.h>
//
//#include <KR3/wl/prepare.h>
//#include <zlib.h>
//#include <KR3/wl/clean.h>
//
//#include "zlib_contrib/zip.h"
//#include "zlib_contrib/unzip.h"
//
//using namespace kr;
//
//#define WRITEBUFFERSIZE		8192
//
//using FIS = io::FIStream<char>*;
//const char AGENT_ZIP[] = "zip";
//
//static voidpf ZCALLBACK fopen64_file_func_16(voidpf opaque, const void* filename, int mode);
//static uLong ZCALLBACK fread_file_func(voidpf opaque, voidpf stream, void* buf, uLong size);
//static uLong ZCALLBACK fwrite_file_func(voidpf opaque, voidpf stream, const void* buf, uLong size);
//static ZPOS64_T ZCALLBACK ftell64_file_func(voidpf opaque, voidpf stream);
//static long ZCALLBACK fseek64_file_func(voidpf  opaque, voidpf stream, ZPOS64_T offset, int origin);
//static int ZCALLBACK fclose_file_func(voidpf opaque, voidpf stream);
//static int ZCALLBACK ferror_file_func(voidpf opaque, voidpf stream);
//
//static zlib_filefunc64_def s_filefunc64 = {
//	fopen64_file_func_16,
//	fread_file_func,
//	fwrite_file_func,
//	ftell64_file_func,
//	fseek64_file_func,
//	fclose_file_func,
//	ferror_file_func,
//};
//
//
//kr::Unzipper::Unzipper()
//{
//	m_uzFile = nullptr;
//}
//
//kr::Unzipper::~Unzipper()
//{
//}
//
//bool kr::Unzipper::open(pcstr16 szFileName)
//{
//	m_uzFile = unzOpen2_64((pcstr)szFileName, &s_filefunc64);
//	if (m_uzFile == nullptr)
//		return false;
//	return true;
//}
//
//int kr::Unzipper::extract(pcstr16 szDestination)
//{
//	int nError;
//	unz_global_info gi;
//
//	nError = unzGetGlobalInfo (m_uzFile, &gi);
//	if (nError != UNZ_OK)
//		return nError;
//
//	for (uLong i = 0; i < gi.number_entry; i++)
//	{
//		nError = _extractCurrentFile(szDestination);
//		if (nError != UNZ_OK)
//			return nError;
//		if ((i + 1) < gi.number_entry)
//		{
//			nError = unzGoToNextFile(m_uzFile);
//		}
//	}
//
//	return nError;
//}
//
//void kr::Unzipper::close()
//{
//	if (m_uzFile)
//	{
//		unzCloseCurrentFile(m_uzFile);
//		unzClose(m_uzFile);
//	}
//	m_uzFile = nullptr;
//}
//
//int kr::Unzipper::_writeFile(File* fout) noexcept
//{
//	finally{
//		delete fout;
//	};
//	int len;
//	TmpArray<byte> buf(WRITEBUFFERSIZE);
//	for (;;)
//	{
//		len = unzReadCurrentFile(m_uzFile, buf.begin(), WRITEBUFFERSIZE);
//		if (len < 0)
//		{
//			g_log.printf(AGENT_ZIP, "error %d: unzReadCurrentFile failed", len);
//			return len;
//		}
//		if (len == 0)
//			return 0;
//		fout->writeImpl(buf.begin(), len);
//	}
//}
//
//void kr::Unzipper::_changeFileDate(pcstr16 filename, dword dosdate, const tm_unz_s * tmu_date)
//{
//	HANDLE hFile;
//	FILETIME ftm, ftLocal, ftCreate, ftLastAcc, ftLastWrite;
//
//	hFile = CreateFile(wide(filename), GENERIC_READ | GENERIC_WRITE,
//		0, nullptr, OPEN_EXISTING, 0, nullptr);
//	GetFileTime(hFile, &ftCreate, &ftLastAcc, &ftLastWrite);
//	DosDateTimeToFileTime((WORD) (dosdate >> 16), (WORD)dosdate, &ftLocal);
//	LocalFileTimeToFileTime(&ftLocal, &ftm);
//	SetFileTime(hFile, &ftm, &ftLastAcc, &ftm);
//	CloseHandle(hFile);
//}
//
//int kr::Unzipper::_extractCurrentFile(pcstr16 dest)
//{
//	char filename_inzip[256];
//    int nError = UNZ_OK;
//	unz_file_info file_info;
//
//	nError = unzGetCurrentFileInfo(
//		m_uzFile,
//		&file_info,
//		filename_inzip,
//		sizeof(filename_inzip),
//		nullptr,
//		0,
//		nullptr,
//		0);
//
//	if (nError != UNZ_OK)
//	{
//		g_log.puts(AGENT_ZIP, "ExtractCurrentFile -> unzGetCurrentFileInfo failed");
//		return nError;
//	}
//
//	TSZ16 write_filename;
//	write_filename << dest << (AcpToUtf16)filename_inzip;
//	g_log.puts(AGENT_ZIP, kr::TSZ() << (Utf16ToAcp)write_filename);
//
//	char16 * filename_withoutpath;
//	char16 * p = filename_withoutpath = write_filename;
//	while (*p != '\0')
//	{
//		if (*p == '/' || *p == '\\')
//			filename_withoutpath = p + 1;
//		p++;
//	}
//
//	if ((*filename_withoutpath) == '\0')
//	{
//		File::createDirectory(write_filename);
//		return nError;
//	}
//	nError = unzOpenCurrentFile(m_uzFile);
//	if (nError != UNZ_OK)
//	{
//		g_log.puts(AGENT_ZIP, "unzOpenCurrentFile");
//		return nError;
//	}
//	finally{
//		unzCloseCurrentFile(m_uzFile);
//	};
//
//	File * file;
//	try
//	{
//		file = File::create(write_filename);
//		goto _write;
//	}
//	catch (Error&)
//	{
//	}
//	try
//	{
//		/* some zipfile don't contain directory alone before file */
//		if (filename_withoutpath == nullptr)
//			return GetLastError();
//
//		File::createFullDirectory(Text16(write_filename, filename_withoutpath-1));
//		file = File::create(write_filename);
//	}
//	catch (Error&)
//	{
//	}
//	try
//	{
//		nError = GetLastError();
//		if (!File::exists(write_filename.c_str()))
//			return nError;
//
//		File::toJunk(write_filename);
//		file = File::create(write_filename);
//	}
//	catch (Error&)
//	{
//		nError = GetLastError();
//		g_log.printf(AGENT_ZIP, "error %d: create file failed", nError);
//		return nError;
//	}
//
//_write:
//	nError = _writeFile(file);
//	if (nError != UNZ_OK)
//		return nError;
//	_changeFileDate(write_filename, file_info.dosDate, &file_info.tmu_date);
//	return UNZ_OK;
//}
//
//
//
//static voidpf ZCALLBACK fopen64_file_func_16(voidpf opaque, const void* filename, int mode)
//{
//	FILE* file = nullptr;
//	LPCWSTR mode_fopen = nullptr;
//	size_t modeidx = -1;
//	if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER) == ZLIB_FILEFUNC_MODE_READ)
//		modeidx = 0;
//	else if (mode & ZLIB_FILEFUNC_MODE_EXISTING)
//		modeidx = 1;
//	else if (mode & ZLIB_FILEFUNC_MODE_CREATE)
//		modeidx = 2;
//
//	if ((filename != nullptr) && (modeidx != -1))
//	{
//		if (sizeof(wchar_t) == sizeof(char16))
//		{
//			static const wchar_t * const modelist[] = {
//				L"rb",
//				L"r+b",
//				L"wb"
//			};
//			_wfopen_s(&file, (const wchar_t *)filename, modelist[modeidx]);
//		}
//		else
//		{
//			static const pcstr modelist[] = {
//				"rb",
//				"r+b",
//				"wb"
//			};
//			fopen_s(&file, TSZ() << toAcp((Text16)(pcstr16)filename), modelist[modeidx]);
//		}
//	}
//	return file;
//}
//
//static uLong ZCALLBACK fread_file_func(voidpf opaque, voidpf stream, void* buf, uLong size)
//{
//	uLong ret;
//	ret = (uLong)fread(buf, 1, (size_t)size, (FILE *)stream);
//	return ret;
//}
//
//static uLong ZCALLBACK fwrite_file_func(voidpf opaque, voidpf stream, const void* buf, uLong size)
//{
//	uLong ret;
//	ret = (uLong)fwrite(buf, 1, (size_t)size, (FILE *)stream);
//	return ret;
//}
//
//static ZPOS64_T ZCALLBACK ftell64_file_func(voidpf opaque, voidpf stream)
//{
//	ZPOS64_T ret;
//	ret = ftello64((FILE *)stream);
//	return ret;
//}
//
//static long ZCALLBACK fseek64_file_func(voidpf  opaque, voidpf stream, ZPOS64_T offset, int origin)
//{
//	int fseek_origin = 0;
//	long ret;
//	switch (origin)
//	{
//	case ZLIB_FILEFUNC_SEEK_CUR:
//		fseek_origin = SEEK_CUR;
//		break;
//	case ZLIB_FILEFUNC_SEEK_END:
//		fseek_origin = SEEK_END;
//		break;
//	case ZLIB_FILEFUNC_SEEK_SET:
//		fseek_origin = SEEK_SET;
//		break;
//	default: return -1;
//	}
//	ret = 0;
//
//	if (fseeko64((FILE *)stream, offset, fseek_origin) != 0)
//		ret = -1;
//
//	return ret;
//}
//
//static int ZCALLBACK fclose_file_func(voidpf opaque, voidpf stream)
//{
//	int ret;
//	ret = fclose((FILE *)stream);
//	return ret;
//}
//
//static int ZCALLBACK ferror_file_func(voidpf opaque, voidpf stream)
//{
//	int ret;
//	ret = ferror((FILE *)stream);
//	return ret;
//}
