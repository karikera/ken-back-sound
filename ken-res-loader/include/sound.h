#pragma once

#include <stdio.h>
#include <string.h>
#include <memory.h>

#include "common.h"

_KR_C_MODE_BEGIN

typedef struct _kr_backend_wave_format_t kr_backend_wave_format_t;
typedef struct _kr_backend_sound_info_t kr_backend_sound_info_t;
typedef struct _kr_backend_sound_callback_t kr_backend_sound_callback_t;

struct _kr_backend_wave_format_t
{
	uint16_t formatTag;
	uint16_t channels;
	uint32_t samplesPerSec;
	uint32_t bytesPerSec;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
	uint16_t size;
};

struct _kr_backend_sound_info_t
{
	kr_backend_wave_format_t format;
	double duration;
	uint32_t totalBytes;
};

struct _kr_backend_sound_callback_t
{
	const fchar_t* extension;
	short* (*start)(kr_backend_sound_callback_t* _this, kr_backend_sound_info_t* _info);
};

bool KEN_EXTERNAL krb_sound_load(kr_backend_sound_callback_t * callback, krb_file_t* file);

_KR_C_MODE_END
