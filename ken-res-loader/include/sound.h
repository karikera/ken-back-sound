#pragma once

#include <stdio.h>
#include <string.h>
#include <memory.h>

#include "common.h"

namespace kr
{

#pragma pack(push, 1)
	struct KrbWaveFormat
	{
		uint16_t formatTag;
		uint16_t channels;
		uint32_t samplesPerSec;
		uint32_t bytesPerSec;
		uint16_t blockAlign;
		uint16_t bitsPerSample;
		uint16_t size;
	};
#pragma pack(pop)

	struct KrbSoundInfo
	{
		KrbWaveFormat format;
		double duration;
		uint32_t totalBytes;
	};

	struct KrbSoundCallback
	{
		const fchar_t* extension;
		short* (*start)(KrbSoundCallback* _this, KrbSoundInfo* _info);
	};

	bool KEN_EXTERNAL krb_sound_load(KrbExtension extension, KrbSoundCallback* callback, KrbFile* file);

}