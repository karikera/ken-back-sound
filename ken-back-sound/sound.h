#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#ifdef __EMSCRIPTEN__
	typedef char fchar_t;
#define fstrcmp strcmp
#define KEN_EXTERNAL
#else
#include <wchar.h>
	typedef wchar_t fchar_t;
#define fstrcmp wcscmp

#ifdef __IS_KEN_BACKEND_SOUND_DLL
#define KEN_EXTERNAL __declspec(dllexport)
#else
#define KEN_EXTERNAL __declspec(dllimport)
#endif

#endif

	typedef struct _kr_backend_sound_callback
	{
		const fchar_t * extension;
		short* (*start)(struct _kr_backend_sound_callback * _this, uint16_t channel, uint16_t sampleBits, uint32_t sampleCount);
	} kr_backend_sound_callback;

	bool KEN_EXTERNAL kr_backend_sound_load_from_stream(kr_backend_sound_callback * callback, void * param, void(*readStream)(void * param, void * dest, size_t destSize));
	bool KEN_EXTERNAL kr_backend_sound_load_from_file(kr_backend_sound_callback * callback, FILE * file);
	bool KEN_EXTERNAL kr_backend_sound_load_from_memory(kr_backend_sound_callback * callback, const void * memory, size_t size) noexcept;

#ifdef __cplusplus
}
#endif