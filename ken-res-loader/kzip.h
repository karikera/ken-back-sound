#pragma once

#include "include/compress.h"

namespace kr
{
	namespace backend
	{
		class Zip
		{
		public:
			static bool load(KrbCompressCallback* callback, KrbFile* file) noexcept;
			// static bool save(const KrbImageSaveInfo* info, KrbFile* file) noexcept;
		};
	}
}
