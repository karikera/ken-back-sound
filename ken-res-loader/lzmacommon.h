#pragma once

#include <KR3/main.h>
#include <KRThird/lzma-sdk/C/7zTypes.h>

inline ATTR_NORETURN void lzmaError(int res)
{
	switch(res)
	{
	case SZ_ERROR_DATA:
		throw kr::InvalidSourceException();
	case SZ_ERROR_MEM: 
		kr::notEnoughMemory();
	case SZ_ERROR_CRC:
		throw kr::InvalidSourceException();
	case SZ_ERROR_UNSUPPORTED:
		throw kr::UnsupportedException();
	case SZ_ERROR_INPUT_EOF:
		throw kr::EofException();
	case SZ_ERROR_OUTPUT_EOF:
		throw kr::NotEnoughSpaceException();
	case SZ_ERROR_PARAM: debug();
		throw kr::ThrowAbort();
	case SZ_ERROR_READ:
	case SZ_ERROR_WRITE:
	case SZ_ERROR_NO_ARCHIVE:
	case SZ_ERROR_ARCHIVE:
	case SZ_ERROR_THREAD:
	case SZ_ERROR_FAIL:
	case SZ_ERROR_PROGRESS:
		throw kr::ThrowAbort();
	default:
		throw kr::Error();
	}
}
