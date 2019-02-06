//#include "lzma.h"
//
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <new>
//
//#include <KRThird/lzma-sdk/C/LzmaLib.h>
//#include <KRThird/lzma-sdk/C/Alloc.h>
//#include <KRThird/lzma-sdk/C/7zFile.h>
//#include <KRThird/lzma-sdk/C/7zVersion.h>
//#include <KRThird/lzma-sdk/C/LzmaDec.h>
//#include <KRThird/lzma-sdk/C/LzmaEnc.h>
//#include <KR3/util/wide.h>
//
//#include "lzmacommon.h"
//
//
//#define PROP_SIZE 5
//
//namespace
//{
//	const CLzmaEncProps PROP =
//	{
//		9,
//		1 << 24,
//		3,
//		0,
//		2,
//		1,
//		32,
//		1,
//		4,
//		32,
//		0,
//		2
//	};
//
//	SRes encode(ISeqOutStream *outStream, ISeqInStream *inStream, UInt64 fileSize)
//	{
//		SRes res;
//
//		CLzmaEncHandle enc = LzmaEnc_Create(&g_Alloc);
//		if (enc == 0)
//			kr::notEnoughMemory();
//
//		res = LzmaEnc_SetProps(enc, &PROP);
//
//		if (res == SZ_OK)
//		{
//			Byte header[LZMA_PROPS_SIZE + 8];
//			size_t headerSize = LZMA_PROPS_SIZE;
//
//			res = LzmaEnc_WriteProperties(enc, header, &headerSize);
//			*(UInt64*)(header + headerSize) = fileSize;
//			headerSize += 8;
//			if (outStream->Write(outStream, header, headerSize) != headerSize)
//				res = SZ_ERROR_WRITE;
//			else
//			{
//				if (res == SZ_OK)
//					res = LzmaEnc_Encode(enc, outStream, inStream, NULL, &g_Alloc, &g_Alloc);
//			}
//		}
//		LzmaEnc_Destroy(enc, &g_Alloc, &g_Alloc);
//		return res;
//	}
//}
//
//kr::LzmaIStream::LzmaIStream(io::VIStream<void> vis) noexcept
//	:m_vis(move(vis))
//{
//	SRes res;
//	LZMA_PROP prop;
//	getLZMAProperty(&prop);
//
//	LzmaDec_Construct(&m_state);
//	res = LzmaDec_Allocate(&m_state, prop.data, LZMA_PROPS_SIZE, &g_Alloc);
//	if (res != SZ_OK)
//		notEnoughMemory();
//
//	reset();
//}
//kr::LzmaIStream::~LzmaIStream() noexcept
//{
//	LzmaDec_Free(&m_state, &g_Alloc);
//}
//void kr::LzmaIStream::reset() noexcept
//{
//	m_inPos = m_inSize = 0;
//	LzmaDec_Init(&m_state);
//}
//size_t kr::LzmaIStream::readImpl(void * dest, size_t size)
//{
//	size_t outsize = size;
//	SRes res;
//	if (size != 0) do
//	{
//		if (m_inPos == m_inSize)
//		{
//			m_inSize = m_vis.read(m_inBuf, IN_BUF_SIZE);
//			m_inPos = 0;
//		}
//
//		SizeT inProcessed = m_inSize - m_inPos;
//		SizeT outProcessed = size;
//		ELzmaStatus status;
//
//		res = LzmaDec_DecodeToBuf(&m_state, (BYTE*)dest, &outProcessed,
//			m_inBuf + m_inPos, &inProcessed, LZMA_FINISH_ANY, &status);
//
//		if (res != SZ_OK)
//			lzmaError(res);
//		if (inProcessed == 0 && outProcessed == 0)
//			lzmaError(SZ_ERROR_DATA);
//		m_inPos += inProcessed;
//		(byte*&)dest += outProcessed;
//		size -= outProcessed;
//	}
//	while (size != 0);
//	return outsize;
//}
//
//SRes LzmaDec_DecodeToSkip(CLzmaDec *p, SizeT *destLen, const Byte *src, SizeT *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status)
//{
//	SizeT outSize = *destLen;
//	SizeT inSize = *srcLen;
//	*srcLen = *destLen = 0;
//	for (;;)
//	{
//		SizeT inSizeCur = inSize, outSizeCur, dicPos;
//		ELzmaFinishMode curFinishMode;
//		SRes res;
//		if (p->dicPos == p->dicBufSize)
//			p->dicPos = 0;
//		dicPos = p->dicPos;
//		if (outSize > p->dicBufSize - dicPos)
//		{
//			outSizeCur = p->dicBufSize;
//			curFinishMode = LZMA_FINISH_ANY;
//		}
//		else
//		{
//			outSizeCur = dicPos + outSize;
//			curFinishMode = finishMode;
//		}
//
//		res = LzmaDec_DecodeToDic(p, outSizeCur, src, &inSizeCur, curFinishMode, status);
//		src += inSizeCur;
//		inSize -= inSizeCur;
//		*srcLen += inSizeCur;
//		outSizeCur = p->dicPos - dicPos;
//		outSize -= outSizeCur;
//		*destLen += outSizeCur;
//		if (res != 0)
//			return res;
//		if (outSizeCur == 0 || outSize == 0)
//			return SZ_OK;
//	}
//}
//
//void kr::LzmaIStream::skipImpl(size_t size)
//{
//	SRes res;
//	if (size == 0)
//		return;
//	do
//	{
//		if (m_inPos == m_inSize)
//		{
//			m_inSize = m_vis.read(m_inBuf, IN_BUF_SIZE);
//			m_inPos = 0;
//		}
//
//		SizeT inProcessed = m_inSize - m_inPos;
//		SizeT outProcessed = size;
//		ELzmaStatus status;
//		res = LzmaDec_DecodeToSkip(&m_state, &outProcessed,
//			m_inBuf + m_inPos, &inProcessed, LZMA_FINISH_ANY, &status);
//
//		if (res != SZ_OK) return lzmaError(res);
//		if (inProcessed == 0 && outProcessed == 0)
//		{
//			return lzmaError(SZ_ERROR_DATA);
//		}
//		m_inPos += inProcessed;
//		size -= outProcessed;
//	}
//	while (size != 0);
//	return lzmaError(res);
//}
//
//ISeqOutStream * kr::LzmaVOStream::getLzmaStream() noexcept
//{
//	return this;
//}
//ISeqInStream * kr::LzmaVIStream::getLzmaStream() noexcept
//{
//	return this;
//}
//kr::filesize_t kr::LzmaVIStream::getLeftSize() noexcept
//{
//	return m_leftSize;
//}
//
//void kr::compressData(void *dest, size_t *destLen, const void *src, size_t srcLen)
//{
//	size_t propsize = 5;
//	int err = LzmaCompress((LPBYTE)dest + propsize, destLen, (LPBYTE)src, srcLen, (LPBYTE)dest,
//		&propsize,/* *outPropsSize must be = 5 */
//		9,      /* 0 <= level <= 9, default = 5 */
//		1 << 24,  /* dictSize  default = (1 << 24) */
//		3,        /* 0 <= lc <= 8, default = 3  */
//		4,        /* 0 <= lp <= 4, default = 0  */
//		2,        /* 0 <= pb <= 4, default = 2  */
//		32,        /* 5 <= fb <= 273, default = 32 */
//		2); /* 1 or 2, default = 2 */
//
//	*destLen += PROP_SIZE;
//	lzmaError(err);
//}
//void kr::decompressData(void *dest, size_t *destLen, const void *src, size_t srcLen)
//{
//	int err = LzmaUncompress((LPBYTE)dest, destLen, (LPBYTE)src + PROP_SIZE, &srcLen, (LPBYTE)src, PROP_SIZE);
//	if (err != 0)
//		lzmaError(err);
//}
//kr::ABuffer kr::decompressData(Buffer src)
//{
//	CLzmaDec p;
//	SRes res;
//	SizeT inSize = src.size();
//	if (inSize < PROP_SIZE)
//		throw EofException();
//
//	byte * props = (byte*)src.begin();
//
//	size_t outSize;
//	{
//		qword outSize64 = *(qword*)(props + PROP_SIZE);
//		if (outSize64 >(size_t)-1)
//			throw TooBigException();
//		outSize = (size_t)outSize64;
//	}
//
//	byte * srcData = props + (PROP_SIZE + sizeof(qword));
//	size_t srcLen = inSize - (PROP_SIZE + sizeof(qword));
//
//
//	ABuffer dest;
//	dest.alloc(outSize);
//
//	LzmaDec_Construct(&p);
//	res = LzmaDec_AllocateProbs(&p, props, PROP_SIZE, &g_Alloc);
//	if (res != SZ_OK)
//		lzmaError(res);
//	finally{
//		LzmaDec_FreeProbs(&p, &g_Alloc);
//	};
//
//	ABuffer buffer;
//	p.dic = (byte*)dest.begin();
//	p.dicBufSize = outSize;
//
//	LzmaDec_Init(&p);
//
//	ELzmaStatus status;
//	res = LzmaDec_DecodeToDic(&p, outSize, srcData, &srcLen, LZMA_FINISH_ANY, &status);
//	if (res != SZ_OK)
//		lzmaError(res);
//	if (status == LZMA_STATUS_NEEDS_MORE_INPUT)
//		throw EofException();
//
//	return dest;
//}
//
//void kr::getLZMAProperty(kr::LZMA_PROP * pDest)
//{
//	CLzmaEncHandle enc;
//	SRes res;
//
//	enc = LzmaEnc_Create(&g_Alloc);
//	if (enc == 0)
//		notEnoughMemory();
//
//	size_t size = sizeof(LZMA_PROP);
//	res = LzmaEnc_SetProps(enc, &PROP);
//	if (res == SZ_OK)
//	{
//		res = LzmaEnc_WriteProperties(enc, pDest->data, &size);
//		if (size != LZMA_PROPS_SIZE)
//		{
//			DebugBreak(); // Different Header size
//		}
//	}
//	LzmaEnc_Destroy(enc, &g_Alloc, &g_Alloc);
//	if (res != SZ_OK)
//		lzmaError(res);
//}
//void kr::compressFile(pcstr16 strDest, pcstr16 strSource)
//{
//	CFileSeqInStream inStream;
//	CFileOutStream outStream;
//	int res;
//
//	FileSeqInStream_CreateVTable(&inStream);
//	File_Construct(&inStream.file);
//
//	FileOutStream_CreateVTable(&outStream);
//	File_Construct(&outStream.file);
//
//	if (InFile_OpenW(&inStream.file, wide(strSource)) != SZ_OK)
//	{
//		throw Error();
//	}
//
//	if (OutFile_OpenW(&outStream.file, wide(strDest)) != SZ_OK)
//	{
//		File_Close(&inStream.file);
//		throw Error();
//	}
//
//	UInt64 fileSize;
//	File_GetLength(&inStream.file, &fileSize);
//	res = ::encode(&outStream.s, &inStream.s, fileSize);
//
//	File_Close(&outStream.file);
//	File_Close(&inStream.file);
//
//	if (res != SZ_OK)
//		lzmaError(res);
//}
//void kr::compressStream(LzmaVOStream vos, LzmaVIStream vis)
//{
//	SRes res = ::encode(vos.getLzmaStream(), vis.getLzmaStream(), vis.getLeftSize());
//	if (res != SZ_OK)
//		lzmaError(res);
//}
//void kr::decompressStream(io::VOStream<void> vo, io::VIStream<void> vi, size_t inSize)
//{
//	struct
//	{
//		LZMA_PROP prop;
//		qword outSize;
//	} header;
//
//	if (inSize < sizeof(header))
//		throw InvalidSourceException();
//
//	{
//		size_t readed = sizeof(header);
//		vi.read(&header, sizeof(header));
//		inSize -= sizeof(header);
//	}
//
//	CLzmaDec state;
//	LzmaDec_Construct(&state);
//	SRes res = LzmaDec_Allocate(&state, header.prop.data, LZMA_PROPS_SIZE, &g_Alloc);
//	if (res != SZ_OK)
//		notEnoughMemory();
//
//	finally{
//		LzmaDec_Free(&state, &g_Alloc);
//	};
//
//	LzmaDec_Init(&state);
//
//	TmpArray<void> outbuffer((size_t)0, 4096);
//	TmpArray<void> buffer((size_t)0, 4096);
//	Byte * readptr = (Byte*)buffer.begin();
//
//	do
//	{
//		if (readptr == (Byte*)buffer.end())
//		{
//			buffer.clear();
//			inSize -= vi.read(&buffer, inSize);
//		}
//
//		SizeT inProcessed = (Byte*)buffer.end() - (byte*)readptr;
//		SizeT outProcessed = outbuffer.capacity();
//		ELzmaStatus status;
//
//		res = LzmaDec_DecodeToBuf(&state, (BYTE*)outbuffer.begin(), &outProcessed,
//			readptr, &inProcessed, LZMA_FINISH_ANY, &status);
//
//		if (res != SZ_OK)
//			lzmaError(res);
//		if (inProcessed == 0 && outProcessed == 0)
//			throw InvalidSourceException();
//
//		readptr += inProcessed;
//		inSize -= inProcessed;
//		header.outSize -= outProcessed;
//		vo.write(outbuffer.begin(), outProcessed);
//	}
//	while (header.outSize != 0);
//}
