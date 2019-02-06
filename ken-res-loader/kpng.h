#pragma once

#include "include/common.h"
#include "include/image.h"

namespace kr
{
	namespace backend
	{
		class Png
		{
		public:
			static bool load(krb_image_callback_t* callback, krb_file_t* file) noexcept;
			static bool save(const krb_image_info_t* info, const void* pixelData, krb_file_t* file) noexcept;
		};
	}
}
