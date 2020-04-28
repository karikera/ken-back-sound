#pragma once

#include "../libloader.h"

extern "C"
{
#include "zlib.h"
}

KRL_BEGIN(ZLib, L"zlibd.dll", L"zlib.dll")
KRL_IMPORT(inflateEnd)
KRL_IMPORT(inflate)
KRL_IMPORT(inflateInit2_)
KRL_IMPORT(crc32)
KRL_IMPORT(deflateInit2_)
KRL_IMPORT(get_crc_table)
KRL_IMPORT(deflateEnd)
KRL_IMPORT(deflate)
KRL_END()
