#pragma once

#include "include/common.h"
#include "include/image.h"

namespace kr
{
	namespace backend
	{
		class Tga
		{
		public:
			static bool load(KrbImageCallback* callback, KrbFile* file) noexcept;
			static bool save(const KrbImageSaveInfo* info, KrbFile* file) noexcept;
		};
	}
}
