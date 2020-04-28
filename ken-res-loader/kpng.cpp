#include "kpng.h"
#include <assert.h>

extern "C" {
#include "pnglibconf.h"
#include "pngconf.h"
#include "png.h"
#include "pnginfo.h"
}

#include "libloader.h"
KRL_BEGIN(LibPng, L"libpng16d.dll", L"libpng16.dll")
KRL_IMPORT(png_create_read_struct)
KRL_IMPORT(png_create_info_struct)
KRL_IMPORT(png_destroy_read_struct)
KRL_IMPORT(png_set_read_fn)
KRL_IMPORT(png_read_info)
KRL_IMPORT(png_get_io_ptr)
KRL_IMPORT(png_get_IHDR)
KRL_IMPORT(png_read_update_info)
KRL_IMPORT(png_read_image)
KRL_IMPORT(png_set_gray_to_rgb)
KRL_IMPORT(png_set_expand)
KRL_IMPORT(png_set_bgr)
KRL_IMPORT(png_set_strip_16)
KRL_IMPORT(png_set_packing)
KRL_IMPORT(png_get_rowbytes)
KRL_IMPORT(png_set_longjmp_fn)
KRL_END()


bool kr::backend::Png::load(KrbImageCallback* callback, KrbFile * file) noexcept
{
	LibPng* libpng = LibPng::getInstance();
	if (libpng == nullptr) return false;

	png_infop				info_ptr;
	png_structp				png_ptr;
	KrbImageInfo imginfo;

	int						bit_depth, color_type, interlace_type;
	int						BltBits;

	// Allocate/initialize the memory for image readpointerstruct...
	png_ptr = libpng->png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (png_ptr == nullptr)
	{
		return false;
	}

	// Allocate/initialize the memory for image information...
	info_ptr = libpng->png_create_info_struct(png_ptr);
	if (info_ptr == nullptr)
	{
		libpng->png_destroy_read_struct(&png_ptr, (png_infopp)nullptr, (png_infopp)nullptr);
		return false;
	}

	// Set error handling if you are using the setjmp/longjmp method (this is
	// the normal method of doing things with libpng).  REQUIRED unless you
	// set up your own error handlers in the png_create_read_struct() earlier.
	if (setjmp((*libpng->png_set_longjmp_fn((png_ptr), longjmp, (sizeof(jmp_buf))))))
	{
		libpng->png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)nullptr);
		return false;
	}

	// Set up the input control if you are using standard C streams...
	// png_init_io(png_ptr, file);
	libpng->png_set_read_fn(png_ptr, file, [](png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead)
		{
			LibPng* libpng = LibPng::getInstance();
			if (libpng == nullptr) return;
			KrbFile* file = (KrbFile*)libpng->png_get_io_ptr(png_ptr);
			if (file == nullptr) return;
			file->read(outBytes, byteCountToRead);
		});
	
	// The call to png_read_info() gives us all of the information from the
	// Png file before the first IDAT (image data chunk).
	libpng->png_read_info(png_ptr, info_ptr);

	// Get the Headerinfo...
	libpng->png_get_IHDR(png_ptr, info_ptr, &imginfo.width, &imginfo.height, &bit_depth, &color_type,
		&interlace_type, nullptr, nullptr);

	if (info_ptr->channels*bit_depth == 32) BltBits = 32;
	else BltBits = 24;

	// Check the format...
	switch (color_type)
	{
	case PNG_COLOR_TYPE_RGB:
		break;

	case PNG_COLOR_TYPE_RGB_ALPHA:
		break;

	case PNG_COLOR_TYPE_GRAY:
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		libpng->png_set_gray_to_rgb(png_ptr);
		libpng->png_set_expand(png_ptr);
		break;

	case PNG_COLOR_TYPE_PALETTE:
		libpng->png_set_expand(png_ptr);
		break;
	}

	// flip the RGB pixels to BGR...
	libpng->png_set_bgr(png_ptr);

	// Some last checks...
	if (bit_depth == 16)	libpng->png_set_strip_16(png_ptr);
	if (bit_depth < 8)		libpng->png_set_packing(png_ptr);

	// Update the PNGLibLoader...

	libpng->png_read_update_info(png_ptr, info_ptr);

	imginfo.pitchBytes = (uint32_t)libpng->png_get_rowbytes(png_ptr, info_ptr);

	switch (BltBits)
	{
	case 32:
	{
		imginfo.pixelformat = PixelFormatARGB8;
		break;
	}
	case 24:
	{
		imginfo.pixelformat = PixelFormatRGB8;
		break;
	}
	default:
		assert(!"Not implemented Yet");
		return false;
	}
	uint8_t* surf = (uint8_t*)callback->start(callback, &imginfo);
	if (surf)
	{
		uint32_t H = imginfo.height;
		png_bytep* row_pointers = new png_bytep[H];
		png_bytep *p = row_pointers;
		png_bytep* p_end = p + H;
		while (p != p_end)
		{
			*p++ = surf;
			surf += imginfo.pitchBytes;
		}
		libpng->png_read_image(png_ptr, row_pointers);
		delete [] row_pointers;
	}

	// clean up after the read, and free any memory allocated...
	libpng->png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)nullptr);
	return true;
}
bool kr::backend::Png::save(const KrbImageSaveInfo* info, KrbFile* file) noexcept
{
	assert(!"Not Implemented Yet");
	return false;
}
