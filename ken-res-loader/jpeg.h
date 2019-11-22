#pragma once

#include "include/image.h"

namespace kr
{
	namespace backend
	{
		class Jpeg
		{
		public:
			static bool load(KrbImageCallback* callback, KrbFile* file) noexcept;
			static bool save(const KrbImageSaveInfo* info, KrbFile* file) noexcept;
		};
	}
}
