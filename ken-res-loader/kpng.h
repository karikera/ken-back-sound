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
			static bool save(const krb_image_save_info_t* info, krb_file_t* file) noexcept;
		};
	}
}
