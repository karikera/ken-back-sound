#pragma once

#include "common.h"

namespace kr
{
	class KrbImageInfo;
	class KrbImageSaveInfo;
	class KrbImagePalette;
	class KrbImageCallback;

	typedef enum _kr_pixelformat_t
	{
		PixelFormatInvalid = -1,
		PixelFormatIndex,		// 0xII [0xII]
		PixelFormatA8,			// 0xAA [0xAA]
		PixelFormatR5G6B5,		// 0bRRRRRGGGGGGBBBBB [0bGGGBBBBB, 0bRRRRRGGG]
		PixelFormatX1RGB5,		// 0bXRRRRRGGGGGBBBBB [0bGGGBBBBB, 0bXRRRRRGG]
		PixelFormatA1RGB5,		// 0bARRRRRGGGGGBBBBB [0bGGGBBBBB, 0bARRRRRGG]
		PixelFormatARGB4,		// 0xARGB [0xGB, 0xAR]
		PixelFormatRGB8,		// 0xRRGGBB [0xBB,0xGG,0xRR]
		PixelFormatXRGB8,		// 0xXXRRGGBB [0xBB,0xGG,0xRR,0xXX]
		PixelFormatARGB8,		// 0xAARRGGBB [0xBB,0xGG,0xRR,0xAA]
		PixelFormatBGR8,		// 0xBBGGRR [0xRR,0xGG,0xBB]
		PixelFormatXBGR8,		// 0xXXBBGGRR [0xRR,0xGG,0xBB,0xXX]
		PixelFormatABGR8,		// 0xAABBGGRR [0xRR,0xGG,0xBB,0xAA]
		PixelFormatRGBA32F,	// [0xRR,0xGG,0xBB,0xAA]
		PixelFormatCount,
	} kr_pixelformat_t;

	class KrbImageInfo
	{
	public:
		kr_pixelformat_t pixelformat;
		uint32_t width;
		uint32_t height;

		uint32_t pitchBytes; // in-out(default: recommended pitch)
	};

	class KrbImageSaveInfo
	{
	public:
		kr_pixelformat_t pixelformat;
		uint32_t width;
		uint32_t height;
		uint32_t pitchBytes;
		void* data;

		int jpegQuality; // max is 100
		bool tgaCompress;
		KrbImagePalette* palette;
	};

	class KrbImagePalette
	{
	public:
		uint32_t color[256]; // ARGB8 // [B] [G] [R] [A] // 0xAARRGGBB
	};

	class KrbImageCallback
	{
	public:
		void* (*start)(KrbImageCallback* _this, KrbImageInfo* _info);

		KrbImagePalette* palette;
	};

	bool KEN_EXTERNAL krb_load_image(KrbExtension extension, KrbImageCallback* callback, KrbFile* file);
	bool KEN_EXTERNAL krb_save_image(KrbExtension extension, const KrbImageSaveInfo* info, KrbFile* file);

}