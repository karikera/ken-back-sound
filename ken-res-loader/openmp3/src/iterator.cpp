#include "../include/openmp3.h"

#include "types.h"
#include "tables.h"



//
//

struct OpenMP3::Iterator::Private
{
	static size_t GetPosition(const Iterator & itr)
	{
		return (size_t)krb_ftell(itr.m_file);
	}

	template <class TYPE> static const TYPE Read(Iterator & itr)
	{
		TYPE type;
		size_t readed = krb_fread(itr.m_file, &type, sizeof(TYPE));
		if (readed != sizeof(TYPE)) type = 0;
		return type;
	}

	static UInt32 ReadWord(Iterator & itr)	//TODO optimise
	{
		UInt32 word = Read<UInt32>(itr);

		UInt8 * bytes = (UInt8*)(&word);

		UInt b1 = bytes[0];
		UInt b2 = bytes[1];
		UInt b3 = bytes[2];
		UInt b4 = bytes[3];

		return (b1 << 24) | (b2 << 16) | (b3 << 8) | (b4 << 0);
	}
};


struct OpenMP3::Frame::Private
{
	static UInt GetSideInforSize(const Frame & frame)
	{
		bool mono = frame.m_mode == OpenMP3::kModeMono;
		return (mono ? 17 : 32);
		switch (frame.m_version)
		{
		case kVersionMPEG1:
			return (mono ? 17 : 32);
		default:
			return (mono ? 9 : 17);
		}
	}
};


//
//

OpenMP3::Frame::Frame()
	: m_ptr(0)
{
	MemClear(this, sizeof(Frame));
}
OpenMP3::Frame::~Frame()
{
	free(m_ptr);
}

OpenMP3::UInt OpenMP3::Frame::GetBitRate() const
{
	return kVersions[m_version].kBitRates[m_layer-1][m_bitrate_index];
}

OpenMP3::UInt OpenMP3::Frame::GetSampleRate() const
{
	return kVersions[m_version].kSampleRates[m_sr_index];
}

OpenMP3::Mode OpenMP3::Frame::GetMode() const
{
	return m_mode;
}

OpenMP3::UInt OpenMP3::Frame::GetLength() const
{
	return m_length;
}

void OpenMP3::Frame::SkipFirst() noexcept
{
	UInt sideinfo_size = Frame::Private::GetSideInforSize(*this);

	const UInt8* ptr = m_ptr + sideinfo_size;

	if (*reinterpret_cast<const UInt32*>(ptr) == reinterpret_cast<const UInt32&>(kInfo[0])) m_length = 0;
}


//
//

OpenMP3::Iterator::Iterator(const Library & library, krb_file_t* file)
	: m_file(file)
{
}

OpenMP3::Result OpenMP3::Iterator::GetNext(Frame & frame)
{
	if (frame.m_ptr)
	{
		free(frame.m_ptr);
		frame.m_ptr = 0;
	}

	//find next frame

	UInt32 word = Private::ReadWord(*this);

	// if ((word & 0xffe00000) != 0xffe00000) return kResultInvalidFrame;
	while ((word & 0xffe00000) != 0xffe00000)
	{
		krb_fseek_cur(m_file, -3);

		word = Private::ReadWord(*this);
	}
	


	//extract values

	frame.m_version = Version((word & 0x001c0000) >> 19);

	frame.m_layer = Layer((word & 0x00060000) >> 17);

	UInt protection_bit = (word & 0x00010000) >> 16;

	frame.m_bitrate_index = (word & 0x0000f000) >> 12;

	frame.m_sr_index = (word & 0x00000c00) >> 10;

	UInt padding_bit = (word & 0x00000200) >> 9;

	//UInt8 private_bit = (word & 0x00000100) >> 8;

	frame.m_mode = OpenMP3::Mode((word & 0x000000c0) >> 6);

	frame.m_mode_extension = (word & 0x00000030) >> 4;

	//UInt8 copyright = (word & 0x00000008) >> 3;

	//UInt8 original_or_copy = (word & 0x00000004) >> 2;

	//UInt8 emphasis = (word & 0x00000003) >> 0;


	
	//check for invalid values

	if (frame.m_version == kVersionReserved) return kResultInvalidVersion;

	if (frame.m_bitrate_index == 0 || frame.m_bitrate_index > 14) return kResultInvalidBitRates;

	if (frame.m_sr_index > 2) return kResultInvalidSampleRates;

	if (frame.m_layer == kLayerReserved) return kResultInvalidLayer;

	
	//skip to end of frame

	if (!protection_bit)
	{
		krb_fseek_cur(m_file, 2);
	}

	
	//bool mono = header.mode == kModeMono;

	//UInt nch = (mono ? 1 : 2);

	//m_ptr += sideinfo_size;

	frame.m_length = kVersions[frame.m_version].kSamplesPerFrame[frame.m_layer-1];
	
	UInt framesize = (frame.GetLength() / 8 * frame.GetBitRate()) / frame.GetSampleRate() + padding_bit;

	framesize -= 4;	//total framesize includes headerword
	
	if (frame.m_ptr) free(frame.m_ptr);
	frame.m_ptr = (UInt8*)malloc(framesize);
	krb_fread(m_file, frame.m_ptr, framesize);

	frame.m_datasize = framesize - (protection_bit ? 0 : 2);

	//correct solution is to actually read ancillary data after huffman table
	//but exact ancillary position is difficult to deduce with current implementation
	//solution: refactor ReadMain code?
	
	// UInt main_data_begin = ((frame.m_ptr[1] & 0x80) << 1) | frame.m_ptr[0];
	return kResultOk;
}
