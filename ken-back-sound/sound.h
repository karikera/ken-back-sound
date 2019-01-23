#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>

#ifdef __EMSCRIPTEN__
	typedef char fchar_t;
#define KEN_EXTERNAL
#else
#include <wchar.h>
	typedef wchar_t fchar_t;

#ifdef __IS_KEN_BACKEND_SOUND_DLL
#define KEN_EXTERNAL __declspec(dllexport)
#else
#define KEN_EXTERNAL __declspec(dllimport)
#endif

#endif
	typedef struct _kr_backend_wave_format
	{
		uint16_t formatTag;
		uint16_t channels;
		uint32_t samplesPerSec;
		uint32_t bytesPerSec;
		uint16_t blockAlign;
		uint16_t bitsPerSample;
		uint16_t size;
	} kr_backend_wave_format;

	typedef struct _kr_backend_sound_info
	{
		kr_backend_wave_format format;
		double duration;
		uint32_t totalBytes;
	} kr_backend_sound_info;

	typedef struct _kr_backend_sound_callback
	{
		const fchar_t * extension;
		short* (*start)(struct _kr_backend_sound_callback * _this, struct _kr_backend_sound_info * _info);
	} kr_backend_sound_callback;

	bool KEN_EXTERNAL kr_backend_sound_load_from_stream(kr_backend_sound_callback * callback, void * param, void(*readStream)(void * param, void * dest, size_t destSize));
	bool KEN_EXTERNAL kr_backend_sound_load_from_file(kr_backend_sound_callback * callback, FILE * file);
	bool KEN_EXTERNAL kr_backend_sound_load_from_memory(kr_backend_sound_callback * callback, const void * memory, size_t size);

#ifdef __cplusplus
}
#endif