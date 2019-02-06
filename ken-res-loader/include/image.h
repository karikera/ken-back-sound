#pragma once

#include "common.h"

_KR_C_MODE_BEGIN

typedef struct _krb_image_info_t krb_image_info_t;
typedef struct _krb_image_palette_t krb_image_palette_t;
typedef struct _krb_image_callback_t krb_image_callback_t;

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

struct _krb_image_info_t
{
	kr_pixelformat_t pixelformat;
	uint32_t width;
	uint32_t height;
};

struct _krb_image_palette_t
{
	uint32_t color[256]; // ARGB8 // [B] [G] [R] [A] // 0xAARRGGBB
};

struct _krb_image_callback_t
{
	const fchar_t* extension;
	void* (*start)(_krb_image_callback_t* _this, krb_image_info_t* _info);

	krb_image_palette_t* palette;
};

bool KEN_EXTERNAL krb_load_image(krb_image_callback_t* callback, krb_file_t* file);

_KR_C_MODE_END
