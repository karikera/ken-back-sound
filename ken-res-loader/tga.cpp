#include "tga.h"
#include "readstream.h"

#include <assert.h>
#include <stdlib.h>
#include <memory.h>

#define TGA_PALETTE_ONE_SIZE (3)
#define TGA_PALETTE_SIZE	(TGA_PALETTE_ONE_SIZE*256)

using namespace kr;

namespace
{

	struct tga_head_t
	{
		uint8_t idsize;
		uint8_t colormaptype;  // is _palette?
		uint8_t imagetype;

		//
		// word colormapstart; 
		// word colormaplength
		//
		// `*to ignore VC word align!!!
		// `8bit image를 위한 것이므로 중요하지 않음!
		//
		//word colormapstart; 
		uint8_t colormapstart1;
		uint8_t colormapstart2;
		//word colormaplength; // *to ignore VC word align!!!
		uint8_t colormaplength1;
		uint8_t colormaplength2;

		uint8_t colormapbits;
		uint16_t xstart;
		uint16_t ystart;
		uint16_t width;
		uint16_t height;
		uint8_t bpp;   // bit per pixel
		uint8_t descriptor;
	};
}

struct color3bytes_t
{
	uint8_t b, g, r;

	bool operator ==(const color3bytes_t& other) const
	{
		return b == other.b && g == other.g && r == other.r;
	}
	bool operator !=(const color3bytes_t& other) const
	{
		return b != other.b || g != other.g || r != other.r;
	}
};

struct ColorInfos
{
	kr_pixelformat_t pf;
	size_t size;
	void (*memcpy_rev)(uint8_t* dest, uint8_t* src, size_t bytes);
	uint8_t* (*fill_line)(uint8_t* dest, void* src, uint8_t bytes);
	void (*tga_compress)(krb_file_t* file, void* src, size_t total_bytes);
};


template <typename PX>
void tga_compress(krb_file_t* file, void* src_void, size_t total_bytes) noexcept
{
	PX* src = (PX*)src_void;
	// encoding..
	// line단위로 하지 않는다.
	uint8_t chunk = 0;
	uint8_t same = 0, diff = 0; // {a b}이면 diff = 1, {a a}이면 same = 1

	PX* src_end = (PX*)((uint8_t*)src + total_bytes);
	PX * pixelDiff = (PX*)malloc(total_bytes);
	PX pixelPrev;
	PX pixelSame;

	pixelPrev = *src++;

	while (src != src_end)
	{
		PX pixelNext = *src++;

		if (pixelPrev == pixelNext)
		{
			if (diff > 0)
			{
				chunk = diff - 1;
				krb_fwrite(file, &chunk, sizeof(chunk));
				krb_fwrite(file, pixelDiff, diff * sizeof(PX));
				diff = 0;
			}

			pixelSame = pixelNext;
			same++;

			if (same == 128)
			{
				chunk = same + 127;
				krb_fwrite(file, &chunk, sizeof(chunk));
				krb_fwrite(file, &pixelSame, sizeof(pixelSame));
				same = 0;
			}
		}
		else
		{
			if (same > 0)
			{
				chunk = same + 127;
				krb_fwrite(file, &chunk, sizeof(chunk));
				krb_fwrite(file, &pixelSame, sizeof(pixelSame));
				same = 0;
			}

			pixelDiff[diff] = pixelPrev;
			diff++;
			if (diff == 128)
			{
				chunk = diff - 1;
				krb_fwrite(file, &chunk, sizeof(chunk));
				krb_fwrite(file, pixelDiff, diff * sizeof(PX));
				diff = 0;
			}
			pixelPrev = pixelNext;
		}
	}

	if (same > 0)
	{
		chunk = 127 + same + 1;
		krb_fwrite(file, &chunk, sizeof(chunk));
		krb_fwrite(file, &pixelSame, sizeof(PX));
	}
	if (diff > 0)
	{
		chunk = diff - 1 + 1;
		krb_fwrite(file, &chunk, sizeof(chunk));
		krb_fwrite(file, pixelDiff, diff * sizeof(PX));

		//
		// a, b 로 끝날 경우 p_diffbuf에는 a까지만 들어간 상태이므로 b를 넣어줘야 한다. (p_pixebufNext)
		//
		krb_fwrite(file, &pixelPrev, sizeof(PX));
	}
	free(pixelDiff);
}

static const ColorInfos colorInfos[4] = {
	{
		PixelFormatIndex,
		1,
		[](uint8_t* dest ,uint8_t* src, size_t bytes) {
			uint8_t* dest_end = dest + bytes;
			uint8_t* src_ptr = src + bytes - 1;
			while (dest != dest_end)
			{
				*dest++ = *src_ptr--;
			}
		},
		[](uint8_t* dest, void* src, uint8_t bytes) {
			memset(dest, *(uint8_t*)src, bytes);
			dest += bytes;
			return dest;
		},
		tga_compress<uint8_t>,
	},
	{
		PixelFormatX1RGB5,
		2,
		[](uint8_t* dest ,uint8_t* src, size_t bytes) {
			uint16_t* dest_ptr = (uint16_t*)dest;
			uint16_t* dest_end = (uint16_t*)(dest + bytes);
			uint16_t* src_ptr = (uint16_t*)(src + bytes - 2);
			while (dest_ptr != dest_end)
			{
				*dest_ptr++ = *src_ptr--;
			}
		},
		[](uint8_t* dest, void* src, uint8_t count) {
			uint16_t v = *(uint16_t*)src;
			uint16_t* dest_ptr = (uint16_t*)dest;
			uint16_t* dest_end = dest_ptr + count;
			while (dest_ptr != dest_end)
			{
				*dest_ptr++ = v;
			}
			return (uint8_t*)dest_end;
		},
		tga_compress<uint16_t>,
	},
	{
		PixelFormatRGB8,
		3,
		[](uint8_t* dest ,uint8_t* src, size_t bytes) {
			uint8_t* dest_end = dest + bytes;
			uint8_t* src_ptr = src + bytes - 3;
			while (dest != dest_end)
			{
				*dest++ = *src_ptr++;
				*dest++ = *src_ptr++;
				*dest++ = *src_ptr;
				src_ptr -= 5;
			}
		},
		[](uint8_t* dest, void* src, uint8_t bytes) {
			uint8_t v[3];
			v[0] = ((uint8_t*)src)[0];
			v[1] = ((uint8_t*)src)[1];
			v[2] = ((uint8_t*)src)[2];
			uint8_t* dest_ptr = (uint8_t*)dest;
			uint8_t* dest_end = dest_ptr + bytes;
			while (dest_ptr != dest_end)
			{
				*dest_ptr++ = v[0];
				*dest_ptr++ = v[1];
				*dest_ptr++ = v[2];
			}
			return (uint8_t*)dest_end;
		},
		tga_compress<color3bytes_t>,
	},
	{
		PixelFormatARGB8,
		4,
		[](uint8_t* dest ,uint8_t* src, size_t bytes) {
			uint32_t* dest_ptr = (uint32_t*)dest;
			uint32_t* dest_end = (uint32_t*)(dest + bytes);
			uint32_t* src_ptr = (uint32_t*)(src + bytes - 4);
			while (dest_ptr != dest_end)
			{
				*dest_ptr++ = *src_ptr--;
			}
		},
		[](uint8_t * dest, void* src, uint8_t bytes) {
			uint32_t v = *(uint32_t*)src;
			uint32_t* dest_ptr = (uint32_t*)dest;
			uint32_t* dest_end = (uint32_t*)(dest + bytes);
			while (dest_ptr != dest_end)
			{
				*dest_ptr++ = v;
			}
			return (uint8_t*)dest_end;
		},
		tga_compress<uint32_t>,
	},
};

bool backend::Tga::load(krb_image_callback_t* callback, krb_file_t* file) noexcept
{
	ReadStream is(file);
	tga_head_t head;

	// read TGA head
	if (!is.read(&head, sizeof(tga_head_t))) return false;

	// 1: 압축되지 않은 팔레트 중심 이미지 (8bit)
	// 2: 압축되지 않은 RGB 이미지
	// 3: 압축되지 않은 Monochrome 이미지
	// 9: RLE된 팔레트 중심 이미지 (8bit)
	// 10: RLE된 RGB 이미지
	// 11: RLE된 Monochrome 이미지
	switch (head.imagetype)
	{
	case 1: case 2: case 9: case 10:
		break;
	default:
		return false;
	}

	if (head.idsize != 0 || head.xstart != 0 || head.ystart != 0)
		return false;

	switch (head.bpp)
	{
	case 8:
		color3bytes_t tripal[256];
		if (is.read(tripal, sizeof(tripal)) == 0)
			return false;

		assert(callback->palette);
		for (size_t i = 0; i < 256; i++)
		{
			color3bytes_t& src = tripal[i];
			uint32_t& dest = callback->palette->color[i];
			dest = 0xff000000 | (src.r << 16) | (src.g << 8) | (src.b);
		}

	case 16:
	case 24:
	case 32:
		break;
	default:
		return false;
	}

	int pixel_byte = head.bpp / 8;
	const ColorInfos & cinfo = colorInfos[pixel_byte - 1];
	int size = head.width * head.height;
	int total_byte = pixel_byte * size;

	uint8_t* pixels = (uint8_t * )malloc(total_byte);
	uint8_t* dest = pixels;

	if (head.imagetype == 9 || head.imagetype == 10)
	{
		// decoding RLE
		uint8_t chunk;
		uint8_t* dest_end = dest + total_byte;
		TempBuffer temp(4096);

		while (dest != dest_end)
		{
			is.read(&chunk, sizeof(chunk));

			if (chunk < 128)
			{
				chunk++;
				is.read(dest, pixel_byte * chunk);
				dest += chunk * pixel_byte;
			}
			else
			{
				chunk -= 127;
				void * pixelData = temp(pixel_byte);
				is.read(pixelData, pixel_byte);
				dest = cinfo.fill_line(dest, pixelData, chunk * pixel_byte);
			}
		}
	}
	else
	{
		// Uncompressed..
		is.read(pixels, total_byte);
	}

	krb_image_info_t imginfo;
	imginfo.width = head.width;
	imginfo.height = head.height;
	imginfo.pixelformat = cinfo.pf;
	dest = (uint8_t*)callback->start(callback, &imginfo);

	// check the descriptor

	if (!(head.descriptor & 0x20)) // reverse vertical
	{
		if (head.descriptor & 0x10) // reverse horizontal
		{
			cinfo.memcpy_rev(dest, pixels, total_byte);
		}
		else
		{
			size_t pitch = pixel_byte * head.width;
			uint8_t * src = pixels + total_byte - pitch;
			uint8_t* dest_end = dest + total_byte;
			while (dest != dest_end)
			{
				memcpy(dest, src, pitch);
				dest += pitch;
				src -= pitch;
			}
		}
	}
	else
	{
		if (head.descriptor & 0x10) // reverse horizontal
		{
			size_t pitch = pixel_byte * head.width;
			uint8_t* src = pixels + total_byte - pitch;
			uint8_t* dest_end = dest + total_byte;
			while (dest != dest_end)
			{
				cinfo.memcpy_rev(dest, src, pitch);
				dest += pitch;
				src -= pitch;
			}
		}
		else
		{
			memcpy(dest, pixels, total_byte);
		}
	}
	free(pixels);

	return dest;
}

bool backend::Tga::save(const krb_image_info_t* info, const void* pixelData, const uint32_t* palette, bool blCompress, krb_file_t* file) noexcept
{
	// from nova1492

	tga_head_t head;
	head.imagetype = 1;
	if (blCompress)
	{
		switch (head.imagetype) // ????
		{
		case 1:
			head.imagetype = 9;
			break;
		case 2:
			head.imagetype = 10;
			break;
		}
	}

	// 뒤집히지 않은 상태로 저장한다. 이것은 다시 불러올 때 뒤집는 처리를 하지 않아서 좋다.
	head.descriptor = 0x20;
	krb_fwrite(file, &head, sizeof(head));

	const ColorInfos* cinfo;
	switch (info->pixelformat)
	{
	case PixelFormatIndex:
	{
		const uint32_t* palette_end = palette;
		while (palette != palette_end)
		{
			uint32_t color = *palette++;
			color3bytes_t trib;
			trib.r = (uint8_t)(color >> 16);
			trib.g = (uint8_t)(color >> 8);
			trib.b = (uint8_t)(color >> 0);
			krb_fwrite(file, &trib, 3);
		}
		cinfo = &colorInfos[0];
		break;
	}
	case PixelFormatX1RGB5:
		cinfo = &colorInfos[1];
		break;
	case PixelFormatBGR8:
		cinfo = &colorInfos[2];
		break;
	case PixelFormatABGR8:
		cinfo = &colorInfos[3];
		break;
	default:
		assert(!"unsupported format");
		return false;
	}

	if (head.imagetype == 9 || head.imagetype == 10)
	{
	}
	else
	{
		krb_fwrite(file, pixelData, info->width * info->height * cinfo->size);
	}

	//
	// Photoshop 7.0에서는 20x20이 안되면 읽지 못한다. 물론 여기 save루틴으로 한 것이 그렇다는 것이다.
	// 분석해본 결과, 포토샵에서 tga를 RLE하는 방식은 조금 다른거 같은데 그것과는 무관한거 같다.
	// 또, 포토샵으로 저장하면 어떤 목적인지 몰라도 마지막에 8 uint8_t의 더미를 넣는다.
	// 20x20이 안되는 이미지는 반드시 더미가 있어야 읽혀진다. ACDsee에서는 더미와 상관없이 잘 읽혀졌다.
	//
	uint32_t dummy = 0;
	krb_fwrite(file, &dummy, 4);
	krb_fwrite(file, &dummy, 4);
	return true;
}
