#include "include/image.h"
#include "kpng.h"
#include "jpeg.h"
#include "tga.h"
#include "util.h"

#include <assert.h>
#include <memory.h>
#include <stdlib.h>

#pragma pack(push, 1)
struct BMP_HEADER {
	uint16_t    bfType;
	uint32_t    bfSize;
	uint16_t    bfReserved1;
	uint16_t    bfReserved2;
	uint32_t    bfOffBits;
};

struct BITMAP_FILE
{
	uint32_t      biSize;
	int32_t       biWidth;
	int32_t       biHeight;
	uint16_t      biPlanes;
	uint16_t      biBitCount;
	uint32_t      biCompression;
	uint32_t      biSizeImage;
	int32_t       biXPelsPerMeter;
	int32_t       biYPelsPerMeter;
	uint32_t      biClrUsed;
	uint32_t      biClrImportant;

	void* getBits() const noexcept
	{
		return (uint8_t*)this + biSize;
	}
	uint32_t getLineBytes() const noexcept
	{
		return (biWidth * biBitCount + 31) >> 5 << 2;
	}
	const krb_image_palette_t* getPalette() const noexcept
	{
		return (krb_image_palette_t*)((uint8_t*)this + sizeof(BITMAP_FILE));
	}
};

#pragma pack(pop)




bool KEN_EXTERNAL krb_load_image(krb_image_callback_t* callback, krb_file_t* file)
{
	if (fstrcmp(callback->extension, "png") == 0)
	{
		kr::backend::Png::load(callback, file);
	}
	else if (fstrcmp(callback->extension, "jpg") == 0 || fstrcmp(callback->extension, "jpeg") == 0)
	{
		kr::backend::Jpeg::load(callback, file);
	}
	else if (fstrcmp(callback->extension, "tga") == 0)
	{
		kr::backend::Tga::load(callback, file);
	}
	else if (fstrcmp(callback->extension, "bmp") == 0)
	{
		BMP_HEADER bfh;
		krb_fread(file, &bfh, sizeof(bfh));
		if (bfh.bfType != "BM"_sig) return false;

		size_t tempBufferSize = bfh.bfOffBits - sizeof(bfh);
		uint8_t * tempBuffer = (uint8_t*)malloc(tempBufferSize);
		BITMAP_FILE * bi = (BITMAP_FILE*)tempBuffer;
		krb_fread(file, tempBuffer, tempBufferSize);

		size_t widthBytes = (bi->biWidth * bi->biBitCount + 7) / 8;
		widthBytes = (widthBytes + 3) & ~3;
		size_t totalBytes = widthBytes * bi->biHeight;
		if (bi->biSizeImage == 0)
			bi->biSizeImage = (int)totalBytes;

		uint8_t * imageBuffer = (uint8_t*)malloc(bi->biSizeImage);
		krb_fread(file, imageBuffer, bi->biSizeImage);

		krb_image_info_t info;
		info.width = bi->biWidth;
		info.height = bi->biHeight;

		switch (bi->biBitCount)
		{
		case 8:
			info.pixelformat = PixelFormatIndex;
			assert(callback->palette);
			memcpy(callback->palette, bi->getPalette(), sizeof(uint32_t) * 256);
			for (uint32_t& v : callback->palette->color)
			{
				((uint8_t*)& v)[3] = 0xff;
			}
			break;
		case 16:
			info.pixelformat = PixelFormatX1RGB5;
			break;
		case 24:
			info.pixelformat = PixelFormatRGB8;
			break;
		case 32:
			info.pixelformat = PixelFormatARGB8;
			break;
		default:
			assert(!"Not Supported Yet");
			free(imageBuffer);
			free(tempBuffer);
			return false;
		}

		uint8_t * dest = (uint8_t*)callback->start(callback, &info);

		size_t destPitch = info.width * bi->biBitCount / 8;
		uint8_t* src = imageBuffer + totalBytes * info.height - widthBytes;
		uint8_t* dest_end = dest + totalBytes;
		while (dest != dest_end)
		{
			memcpy(dest, src, destPitch);
			dest += destPitch;
			src -= widthBytes;
		}

		free(imageBuffer);
		free(tempBuffer);
		return true;
	}
	return false;
}
