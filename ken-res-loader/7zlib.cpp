#include "7zlib.h"

#include <stdio.h>
#include <string.h>

// #include "lzmacommon.h"

using namespace kr;

//
//namespace kr
//{
//	class Decompress7z
//	{
//	public:
//		void decompress(File* file);
//		virtual void onFileRead(uint readcount, uint fullcount) noexcept;
//
//	private:
//	};
//	void unzip7z(pcstr filename, Progressor * prog);
//	void unzip7z(pcstr16 filename, Progressor * prog);
//	void unzip7z(File * file, Progressor * prog);
//}
//
//
//extern "C"
//{
//	#include <KRThird/lzma-sdk/C/7z.h>
//	#include <KRThird/lzma-sdk/C/7zAlloc.h>
//	#include <KRThird/lzma-sdk/C/7zCrc.h>
//	#include <KRThird/lzma-sdk/C/7zFile.h>
//	#include <KRThird/lzma-sdk/C/7zVersion.h>
//}
//#include <KR3/wl/clean.h>
//
//void kr::Decompress7z::decompress(File* file)
//{
//	CFileInStream archiveStream;
//	Temp<CLookToRead> lookStream;
//	CSzArEx db;
//	SRes res;
//	ISzAlloc allocImp;
//	ISzAlloc allocTempImp;
//	char16_t *temp = NULL;
//	size_t tempSize = 0;
//
//	allocImp.Alloc = SzAlloc;
//	allocImp.Free = SzFree;
//
//	allocTempImp.Alloc = SzAllocTemp;
//	allocTempImp.Free = SzFreeTemp;
//
//	archiveStream.file.handle = file;
//
//	FileInStream_CreateVTable(&archiveStream);
//	LookToRead_CreateVTable(lookStream, False);
//
//	lookStream->realStream = &archiveStream.s;
//	LookToRead_Init(lookStream);
//
//	CrcGenerateTable();
//
//	SzArEx_Init(&db);
//	res = SzArEx_Open(&db, &lookStream->s, &allocImp, &allocTempImp);
//	if (res == SZ_OK)
//	{
//		UInt32 i;
//
//		/*
//		if you need cache, use these 3 variables.
//		if you use external function, you can make these variable as static.
//		*/
//		UInt32 blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
//		Byte *outBuffer = 0; /* it must be 0 before first call for each new archive. */
//		size_t outBufferSize = 0;  /* it can have any value before first call (if outBuffer = 0) */
//
//		for (i = 0; i < db.NumFiles; i++)
//		{
//			onFileRead(i, db.NumFiles); // TODO: get size
//			size_t offset = 0;
//			size_t outSizeProcessed = 0;
//			// const CSzFileItem *f = db.db.Files + i;
//			size_t len;
//			unsigned isDir = SzArEx_IsDir(&db, i);
//
//			len = SzArEx_GetFileNameUtf16(&db, i, NULL);
//			if (len > tempSize)
//			{
//				SzFree(NULL, temp);
//				tempSize = len;
//				temp = (char16_t *)SzAlloc(NULL, tempSize * sizeof(temp[0]));
//				if (temp == 0)
//				{
//					res = SZ_ERROR_MEM;
//					break;
//				}
//			}
//
//			SzArEx_GetFileNameUtf16(&db, i, (UInt16*)temp);
//			if (!isDir)
//			{
//				res = SzArEx_Extract(&db, &lookStream->s, i,
//					&blockIndex, &outBuffer, &outBufferSize,
//					&offset, &outSizeProcessed,
//					&allocImp, &allocTempImp);
//				if (res != SZ_OK)
//					break;
//			}
//			CSzFile outFile;
//			size_t processedSize;
//			size_t j;
//			wchar_t *name = (wchar_t *)temp;
//			const wchar_t *destPath = (const wchar_t *)name;
//			for (j = 0; name[j] != 0; j++)
//			{
//				if (name[j] == '/')
//				{
//					name[j] = 0;
//					CreateDirectoryW(name, NULL);
//					name[j] = CHAR_PATH_SEPARATOR;
//				}
//			}
//
//			if (isDir)
//			{
//				CreateDirectoryW(destPath, NULL);
//				continue;
//			}
//			else if (OutFile_OpenW(&outFile, destPath))
//			{
//				res = ERROR_INVALID_ACCESS;
//				break;
//			}
//			processedSize = outSizeProcessed;
//			if (File_Write(&outFile, outBuffer + offset, &processedSize) != 0 || processedSize != outSizeProcessed)
//			{
//				res = ERROR_INVALID_ACCESS;
//				break;
//			}
//			if (File_Close(&outFile))
//			{
//				res = SZ_ERROR_FAIL;
//				break;
//			}
//			if (SzBitWithVals_Check(&db.Attribs, i))
//			{
//				SetFileAttributesW(destPath, db.Attribs.Vals[i]);
//			}
//		}
//		IAlloc_Free(&allocImp, outBuffer);
//	}
//	SzArEx_Free(&db, &allocImp);
//	SzFree(NULL, temp);
//
//	lzmaError(res);
//}
//void kr::Decompress7z::onFileRead(uint readcount, uint fullcount) noexcept
//{
//
//}
//
//void kr::unzip7z(pcstr filename, Progressor * prog)
//{
//	Must<File> file = File::open(filename);
//	return unzip7z(file,prog);
//}
//void kr::unzip7z(pcstr16 filename, Progressor * prog)
//{
//	Must<File> file = File::open(filename);
//	return unzip7z(file, prog);
//}
//void kr::unzip7z(File * file, Progressor * prog)
//{
//	struct Decompress : kr::Decompress7z
//	{
//		Progressor * _this;
//		virtual void onFileRead(kr::uint readcount, kr::uint fullcount) noexcept override
//		{
//			_this->setProgress(readcount, fullcount);
//		}
//	};
//
//	Decompress de;
//	de._this = prog;
//	return de.decompress((kr::File*)file);
//}
//

bool backend::_7z::load(KrbCompressCallback* callback, KrbFile* file) noexcept
{
	return false;
}