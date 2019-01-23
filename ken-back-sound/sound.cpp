#define __IS_KEN_BACKEND_SOUND_DLL
#include "sound.h"
#include <limits.h>

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include "openmp3/openmp3.h"

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

#ifdef __EMSCRIPTEN__
#define _TF(x) x
#define fstrcmp(a,b) strcmp(a,b)
#else
#include <wchar.h>
#define _TF(x) L##x
#define fstrcmp(a,b) wcscmp(a,L##b)
#endif

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

	bool loadFromOgg(kr_backend_sound_callback * callback, OggVorbis_File* vf) noexcept
	{
		vorbis_info *  vi;
		vi = ov_info(vf, -1);
		if (vi == nullptr) return false;

		
		_kr_backend_sound_info info;
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

	constexpr uint32_t makeSignature(const char * str, size_t size)
	{
		return size == 0 ? 0 : ((makeSignature(str + 1, size - 1) << 8) | *str);
	}
	constexpr uint32_t operator ""_sig(const char * str, size_t size)
	{
		return makeSignature(str, size);
	}

	class TempBuffer
	{
	private:
		void * m_buffer;
		size_t m_size;

	public:
		TempBuffer(size_t reserve) noexcept
			:m_size(reserve), m_buffer(nullptr)
		{
		}
		~TempBuffer() noexcept
		{
			if (m_buffer) free(m_buffer);
		}

		void * operator()(size_t size) noexcept
		{
			if (m_buffer != nullptr)
			{
				if (m_size >= size) return m_buffer;
				free(m_buffer);
				m_buffer = malloc(m_size = size);
				return m_buffer;
			}
			else
			{
				m_buffer = malloc(size);
				return m_buffer;
			}
		}

	};
}

extern "C" bool KEN_EXTERNAL kr_backend_sound_load_from_stream(kr_backend_sound_callback * callback, void * param, void(*readStream)(void * param, void * dest, size_t destSize))
{
	if (fstrcmp(callback->extension, "wav") == 0)
	{
		TempBuffer temp(4096);

		auto read32 = [&] {
			uint32_t sig;
			readStream(param, &sig, 4);
			return sig;
		};
		auto testSignature = [&](uint32_t signature){
			return read32() == signature;
		};
		auto findChunk = [&](uint32_t signature){
			while (!testSignature(signature))
			{
				uint32_t size = read32();
				readStream(param, temp(size), size);
			}
			return read32();
		};

		auto readStructure = [&](void* value, uintptr_t size, uintptr_t srcsize) {
			char* pRead = (char*)value;
			if (srcsize < size)
			{
				readStream(param, pRead, srcsize);
			}
			else
			{
				readStream(param, pRead, size);
			}
		};

		if (!testSignature("RIFF"_sig)) return false;
		uint32_t fullSize = read32();
		if (!testSignature("WAVE"_sig)) return false;

		uint32_t formatSize = findChunk("fmt "_sig);
		if (formatSize == -1) return false;
		if (formatSize < sizeof(kr_backend_wave_format)) return false;

		kr_backend_sound_info info;
		readStructure(&info.format, sizeof(info.format), formatSize);
		if (info.format.formatTag != WAVE_FORMAT_TAG) return false;

		uint32_t dataSize = findChunk("data"_sig);
		if (dataSize == -1) return false;

		info.totalBytes = dataSize;
		info.duration = (double)dataSize / info.format.bytesPerSec;
		short * buffer = callback->start(callback, &info);
		if (buffer != nullptr)
		{
			readStream(param, buffer, dataSize);
		}
		return true;
	}
	if (fstrcmp(callback->extension, "mp3") == 0)
	{
		assert(!"Not supported yet");
		return false;
	}
	if (fstrcmp(callback->extension, "ogg") == 0)
	{
		struct pack_t
		{
			void * param;
			void(*readStream)(void * param, void * dest, size_t destSize);
		} pack = {param, readStream};

		ov_callbacks callbacks = {
			[](void * buffer, size_t elementSize, size_t elementCount, void * fp)->size_t {
			pack_t * pack = (pack_t*)fp;
			size_t readed = elementSize * elementCount;
			pack->readStream(pack->param, buffer, readed);
			return readed;
		},
			nullptr,
			[](void * fp)->int {
			return 0;
		},
			nullptr
		};
		OggVorbis_File vf;
		int res = ov_open_callbacks((void *)&pack, &vf, nullptr, 0, callbacks);
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
	if (fstrcmp(callback->extension, "opus") == 0)
	{
		assert(!"Not supported yet");
		return false;
	}
	assert(!"Not supported yet");
	return false;
}
extern "C" bool KEN_EXTERNAL kr_backend_sound_load_from_file(kr_backend_sound_callback * callback, FILE * file)
{
	if (fstrcmp(callback->extension, "ogg") == 0)
	{
		OggVorbis_File vf;
		int res = ov_open(file, &vf, nullptr, 0);
		if (res < 0)
		{
			switch (res)
			{
			case OV_EREAD: break;
			case OV_ENOTVORBIS: break;
			case OV_EVERSION: break;
			case OV_EBADHEADER: break;
			case OV_EFAULT: break;
			}
			fclose(file);
			return false;
		}
		if (loadFromOgg(callback, &vf)) return true;
		fclose(file);
		return false;
	}
	if (fstrcmp(callback->extension, "opus") == 0)
	{
		assert(!"Not supported yet");
		return false;
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
	if (fstrcmp(callback->extension, "mp3") == 0)
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

			kr_backend_sound_info info;
			info.format.bitsPerSample = 16;
			info.format.channels = mono ? 1 : 2;
			info.format.samplesPerSec = maxSampleRate;
			info.format.blockAlign = info.format.channels * info.format.bitsPerSample / 8;
			info.duration = (double)totalSampleCount / maxSampleRate;
			info.totalBytes = totalSampleCount * info.format.blockAlign;
			short * dest = callback->start(callback, &info);

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
							*dest++ = (short)lroundf((*src++) * SHRT_MAX);
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
	if (fstrcmp(callback->extension, "opus") == 0)
	{
		assert(!"Not supported yet");
		return false;
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
