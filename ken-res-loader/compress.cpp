#include "include/compress.h"
#include "kzip.h"
#include "lzma.h"
#include "7zlib.h"

bool KEN_EXTERNAL kr::krb_load_compress(KrbExtension extension, KrbCompressCallback* callback, KrbFile* file) noexcept
{
	switch (extension)
	{
	case KrbExtension::CompressZip:
		return kr::backend::Zip::load(callback, file);
	case KrbExtension::Compress7z:
		return kr::backend::_7z::load(callback, file);
	default:
		break;
	}
	return false;
}
//bool KEN_EXTERNAL kr::krb_save_compress(KrbExtension extension, const KrbImageSaveInfo* info, KrbFile* file)
//{
//	switch (extension)
//	{
//	case KrbExtension::ImageZip:
//		return kr::backend::Zip::save(info, file);
//	case KrbExtension::ImageLzma:
//		return kr::backend::Lzma::save();
//	case KrbExtension::Image7z:
//		return kr::backend::_7z::save(info, file);
//	}
//	return false;
//}
