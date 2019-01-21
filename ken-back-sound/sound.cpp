#define __IS_KEN_BACKEND_SOUND_DLL
#include "sound.h"

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include "openmp3/openmp3.h"

#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>

extern "C" bool KEN_EXTERNAL kr_backend_sound_load_from_stream(kr_backend_sound_callback * callback, void * param, void(*readStream)(void * param, void * dest, size_t destSize))
{
	if (fstrcmp(callback->extension, L"mp3") == 0)
	{
		assert(!"Not supported yet");
		return false;
	}
}
extern "C" bool KEN_EXTERNAL kr_backend_sound_load_from_file(kr_backend_sound_callback * callback, FILE * file)
{
	if (fstrcmp(callback->extension, L"ogg") == 0)
	{
		// ogg
	}
	else if (fstrcmp(callback->extension, L"opus") == 0)
	{
		// opus
	}

	// read as memory
	fseek(file, 0L, SEEK_END);
	size_t size = ftell(file);
	rewind(file);

	void * data = malloc(size);
	fread(data, 1, size, file);
	bool res = kr_backend_sound_load_from_memory(callback, data, size);
	free(data);
	return res;
}
extern "C" bool KEN_EXTERNAL kr_backend_sound_load_from_memory(kr_backend_sound_callback * callback, const void * memory, size_t size)
{
	if (fstrcmp(callback->extension, L"mp3") == 0)
	{
		try
		{
			OpenMP3::Library openmp3;
			OpenMP3::Iterator itr(openmp3, (OpenMP3::UInt8*)memory, size);

			constexpr size_t SAMPLE_PER_FRAME = 1152;
			OpenMP3::Frame frame;
			bool mono = true;
			uint32_t totalSampleCount = 0;
			int maxSampleRate = 0;
			OpenMP3::Result res;

			{
				OpenMP3::Iterator itr2 = itr;
				while (!(res = itr2.GetNext(frame)))
				{
					totalSampleCount++;
					if (mono && frame.GetMode() != OpenMP3::kModeMono)
					{
						mono = false;
					}
					int sampleRate = frame.GetSampleRate();
					if (sampleRate > maxSampleRate) maxSampleRate = sampleRate;
				}
				totalSampleCount *= SAMPLE_PER_FRAME;
			}

			short * dest = callback->start(callback, mono ? 1 : 2, 16, totalSampleCount);

			float readed[2][SAMPLE_PER_FRAME];
			OpenMP3::Decoder decoder(openmp3);

			if (mono)
			{
				while (!(res = itr.GetNext(frame)))
				{
					int nsamples = decoder.ProcessFrame(frame, readed);
					int sampleRate = frame.GetSampleRate();
					if (sampleRate != maxSampleRate)
					{
						float * src = readed[0];
						float * src_end = readed[0] + nsamples;
						for (int i = 0; i < nsamples; i++)
						{
							int idx_v = i * sampleRate;
							int idx = idx_v / maxSampleRate;
							int idx_off = idx_v % maxSampleRate;

							float res_f;
							if (idx_off == 0)
							{
								res_f = src[idx];
							}
							else
							{
								float pos = (float)idx_off / maxSampleRate;
								res_f = src[idx] * pos + src[idx + 1] * (1 - pos);
							}
							short sample = (short)lroundf(res_f * SHRT_MAX);
							*dest++ = sample;
							*dest++ = sample;
						}
					}
					else
					{
						float * src = readed[0];
						float * src_end = readed[0] + nsamples;
						while (src != src_end)
						{
							*dest++ = lroundf((*src++) * SHRT_MAX);
						}
					}
				}
			}
			else
			{
				while (!(res = itr.GetNext(frame)))
				{
					OpenMP3::UInt nsamples = decoder.ProcessFrame(frame, readed);
					uint32_t rate = frame.GetSampleRate();
					assert(rate == maxSampleRate);

					float * src1 = readed[0];
					float * src2 = readed[1];
					float * src_end = readed[0] + nsamples;
					if (frame.GetMode() == OpenMP3::kModeMono)
					{
						while (src1 != src_end)
						{
							*dest++ = (short)((*src1) * SHRT_MAX);
							*dest++ = (short)((*src1++) * SHRT_MAX);
						}
					}
					else
					{
						while (src1 != src_end)
						{
							*dest++ = (short)((*src1++) * SHRT_MAX);
							*dest++ = (short)((*src2++) * SHRT_MAX);
						}
					}
				}
			}
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	// read as stream
	struct pack_t {
		const char * memory;
		const char * end;
	} pack = {(char*)memory, (char*)memory + size};
	return kr_backend_sound_load_from_stream(callback, &pack, [](void * param, void * dest, size_t size) {
		pack_t* pack = ((pack_t*)param);
		size_t left = pack->end - pack->memory;
		if (left == 0) return;
		size_t readsize = (left < size) ? left : size;
		memcpy(dest, pack->memory, readsize);
		pack->memory += readsize;
	});
}
