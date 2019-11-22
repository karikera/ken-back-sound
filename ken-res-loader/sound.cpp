#include "include/sound.h"
#include <limits.h>

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include "openmp3/openmp3.h"
#include "readstream.h"
#include "util.h"

#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>

#ifdef _DEBUG
#pragma comment(lib, "libvorbisd.lib")
#pragma comment(lib, "libvorbisfiled.lib")
#else
#pragma comment(lib, "libvorbis.lib")
#pragma comment(lib, "libvorbisfile.lib")
#endif

using namespace kr;

constexpr uint16_t WAVE_FORMAT_TAG = 1;

namespace
{
	typedef struct _kr_backend_wave_format_base {
		uint16_t formatTag;
		uint16_t channels;
		uint32_t samplesPerSec;
		uint32_t avgBytesPerSec;
		uint16_t blockAlign;
	} kr_backend_wave_format_base;

	bool loadFromOgg(KrbSoundCallback * callback, OggVorbis_File* vf) noexcept
	{
		vorbis_info *  vi;
		vi = ov_info(vf, -1);
		if (vi == nullptr) return false;

		
		KrbSoundInfo info;
		info.format.formatTag = WAVE_FORMAT_TAG;
		info.format.channels = vi->channels;
		info.format.bitsPerSample = 16;
		info.format.blockAlign = info.format.channels * info.format.bitsPerSample / 8;
		info.format.samplesPerSec = vi->rate;
		info.format.bytesPerSec = info.format.samplesPerSec * info.format.blockAlign;
		info.format.size = sizeof(info.format);
		info.duration = ov_time_total(vf, -1);
		info.totalBytes = (uint32_t)(info.format.samplesPerSec * info.duration);
		char * dest = (char*)callback->start(callback, &info);
		if (dest == nullptr) return false;
		uint32_t left = info.totalBytes;

		while (left)
		{
			int nBitStream;
			int readed = ov_read(vf, dest, left, 0, 2, 1, &nBitStream);
			if (readed < 0)
			{
				memset(dest, 0, left);
				break;
			}
			dest += readed;
			left -= readed;
		}
		return true;
	}
}

extern "C" bool KEN_EXTERNAL krb_sound_load(KrbExtension extension, KrbSoundCallback * callback, KrbFile* file)
{
	kr::backend::ReadStream is(file);

	switch (extension)
	{
	case KrbExtension::SoundOpus:
		break;
	case KrbExtension::SoundOgg:
	{

		ov_callbacks callbacks = {
			[](void* buffer, size_t elementSize, size_t elementCount, void* fp)->size_t {
			return ((KrbFile*)fp)->read(buffer, elementSize * elementCount);
		},
			[](void* fp, ogg_int64_t offset, int whence)->int {
				switch (whence)
				{
				case SEEK_SET: ((KrbFile*)fp)->seek_set(offset); break;
				case SEEK_CUR: ((KrbFile*)fp)->seek_cur(offset); break;
				case SEEK_END: ((KrbFile*)fp)->seek_end(offset); break;
				}
				return 0;
			},
			[](void* fp)->int { return 0; },
			[](void* fp)->long { return (long)((KrbFile*)fp)->tell(); }
		};
		OggVorbis_File vf;
		int res = ov_open_callbacks((void*)file, &vf, nullptr, 0, callbacks);
		if (res < 0)
		{
			switch (res) ////에러처리
			{
			case OV_EREAD: break;
			case OV_ENOTVORBIS: break;
			case OV_EVERSION: break;
			case OV_EBADHEADER: break;
			case OV_EFAULT: break;
			}
			return false;
		}
		bool ret = loadFromOgg(callback, &vf);
		ov_clear(&vf);
		return ret;
	}
	case KrbExtension::SoundMp3:
		try
		{
			uint64_t file_start_pos = file->tell();

			OpenMP3::Library openmp3;
			OpenMP3::Iterator itr(openmp3, file);

			constexpr size_t SAMPLE_PER_FRAME = 1152;
			OpenMP3::Frame frame;
			bool mono = true;
			uint32_t totalSampleCount = 0;
			int maxSampleRate = 0;
			OpenMP3::Result res;

			{
				bool first = false;
				while (!(res = itr.GetNext(frame)))
				{
					if (!first)
					{
						first = true;
						frame.SkipFirst();
					}

					totalSampleCount++;
					if (mono&& frame.GetMode() != OpenMP3::kModeMono)
					{
						mono = false;
					}
					int sampleRate = frame.GetSampleRate();
					if (sampleRate > maxSampleRate) maxSampleRate = sampleRate;
				}
				totalSampleCount *= SAMPLE_PER_FRAME;
			}

			file->seek_set(file_start_pos);


			KrbSoundInfo info;
			info.format.bitsPerSample = 16;
			info.format.channels = mono ? 1 : 2;
			info.format.samplesPerSec = maxSampleRate;
			info.format.blockAlign = info.format.channels * info.format.bitsPerSample / 8;
			info.duration = (double)totalSampleCount / maxSampleRate;
			info.totalBytes = totalSampleCount * info.format.blockAlign;
			short* dest = callback->start(callback, &info);

			float readed[2][SAMPLE_PER_FRAME];
			OpenMP3::Decoder decoder(openmp3);

			if (mono)
			{
				bool first = false;
				while (!(res = itr.GetNext(frame)))
				{
					if (!first)
					{
						first = true;
						frame.SkipFirst();
					}

					int nsamples = decoder.ProcessFrame(frame, readed);
					int sampleRate = frame.GetSampleRate();
					if (sampleRate != maxSampleRate)
					{
						float* src = readed[0];
						float* src_end = readed[0] + nsamples;
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
						float* src = readed[0];
						float* src_end = readed[0] + nsamples;
						while (src != src_end)
						{
							*dest++ = (short)lroundf((*src++) * SHRT_MAX);
						}
					}
				}
			}
			else
			{
				bool first = false;
				while (!(res = itr.GetNext(frame)))
				{
					if (!first)
					{
						first = true;
						frame.SkipFirst();
					}

					OpenMP3::UInt nsamples = decoder.ProcessFrame(frame, readed);
					uint32_t rate = frame.GetSampleRate();
					assert(rate == maxSampleRate);

					float* src1 = readed[0];
					float* src2 = readed[1];
					float* src_end = readed[0] + nsamples;
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
	case KrbExtension::SoundWav:
	{
		if (!is.testSignature("RIFF"_sig)) return false;
		uint32_t fullSize = is.read32();
		if (!is.testSignature("WAVE"_sig)) return false;

		uint32_t formatSize = is.findChunk("fmt "_sig);
		if (formatSize == -1) return false;
		if (formatSize < sizeof(KrbWaveFormat)) return false;

		KrbSoundInfo info;
		is.readStructure(&info.format, sizeof(info.format), formatSize);
		if (info.format.formatTag != WAVE_FORMAT_TAG) return false;

		uint32_t dataSize = is.findChunk("data"_sig);
		if (dataSize == -1) return false;

		info.totalBytes = dataSize;
		info.duration = (double)dataSize / info.format.bytesPerSec;
		short* buffer = callback->start(callback, &info);
		if (buffer != nullptr)
		{
			file->read(buffer, dataSize);
		}
		return true;
	}
	default:
		return false;
	}
	assert(!"Not supported yet");
	return false;
}
