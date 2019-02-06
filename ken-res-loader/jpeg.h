#pragma once

#include "include/image.h"

namespace kr
{
	namespace backend
	{
		class Jpeg
		{
		public:
			static constexpr int QUALITY_MAX = 100;
			static bool load(krb_image_callback_t* callback, krb_file_t* file) noexcept;
			static bool save(const krb_image_info_t* info, const void* pixelData, int quality, krb_file_t* file) noexcept;
		};
	}
}
