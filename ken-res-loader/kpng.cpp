#include "kpng.h"
#include <assert.h>

extern "C" {
#include "pnglibconf.h"
#include "pngconf.h"
#include "png.h"
#include "pnginfo.h"
}

#ifdef _DEBUG
#pragma comment(lib, "libpng16d.lib")
#else
#pragma comment(lib, "libpng16.lib")
#endif

bool kr::backend::Png::load(krb_image_callback_t* callback, krb_file_t * file) noexcept
{
	png_infop				info_ptr;
	png_structp				png_ptr;
	krb_image_info_t imginfo;

	int						bit_depth, color_type, interlace_type;
	int						BltBits;

	// Allocate/initialize the memory for image readpointerstruct...
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (png_ptr == nullptr)
	{
		return false;
	}

	// Allocate/initialize the memory for image information...
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == nullptr)
	{
		png_destroy_read_struct(&png_ptr, (png_infopp)nullptr, (png_infopp)nullptr);
		return false;
	}

	// Set error handling if you are using the setjmp/longjmp method (this is
	// the normal method of doing things with libpng).  REQUIRED unless you
	// set up your own error handlers in the png_create_read_struct() earlier.
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)nullptr);
		return false;
	}

	// Set up the input control if you are using standard C streams...
	// png_init_io(png_ptr, file);
	png_set_read_fn(png_ptr, file, [](png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead)
		{
			krb_file_t* file = (krb_file_t*)png_get_io_ptr(png_ptr);
			if (file == nullptr) return;
			krb_fread(file, outBytes, byteCountToRead);
		});
	
	// The call to png_read_info() gives us all of the information from the
	// Png file before the first IDAT (image data chunk).
	png_read_info(png_ptr, info_ptr);

	// Get the Headerinfo...
	png_get_IHDR(png_ptr, info_ptr, &imginfo.width, &imginfo.height, &bit_depth, &color_type,
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
		png_set_gray_to_rgb(png_ptr);
		png_set_expand(png_ptr);
		break;

	case PNG_COLOR_TYPE_PALETTE:
		png_set_expand(png_ptr);
		break;
	}

	// flip the RGB pixels to BGR...
	png_set_bgr(png_ptr);

	// Some last checks...
	if (bit_depth == 16)	png_set_strip_16(png_ptr);
	if (bit_depth < 8)		png_set_packing(png_ptr);

	// Update the PNGLibLoader...

	png_read_update_info(png_ptr, info_ptr);

	size_t uRowBytes = png_get_rowbytes(png_ptr, info_ptr);

	switch (BltBits)
	{
	case 32:
	{
		imginfo.pixelformat = PixelFormatARGB8;
		size_t uRowBytes = png_get_rowbytes(png_ptr, info_ptr);
		break;
	}
	case 24:
	{
		imginfo.pixelformat = PixelFormatRGB8;
		size_t uRowBytes = png_get_rowbytes(png_ptr, info_ptr);
		break;
	}
	default:
		assert(!"Not implemented Yet");
		return false;
	}

	{
		uint8_t* surf = (uint8_t*)callback->start(callback, &imginfo);

		uint32_t H = imginfo.height;
		png_bytep* row_pointers = new png_bytep[H];
		png_bytep *p = row_pointers;
		while (H--)
		{
			*p++ = surf;
			surf += imginfo.width;
		}
		png_read_image(png_ptr, row_pointers);
		delete [] row_pointers;
	}

	// clean up after the read, and free any memory allocated...
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)nullptr);
	return true;
}
bool kr::backend::Png::save(const krb_image_info_t* info, const void* pixelData, krb_file_t* file) noexcept
{
	assert(!"Not Implemented Yet");
	return false;
}
