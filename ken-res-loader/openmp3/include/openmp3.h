#pragma once

#include <stddef.h>
#include <stdlib.h>
#include "../../include/common.h"


//
//declarations

namespace OpenMP3
{

	//enums

	enum Version
	{
		kVersionMPEG2_5,
		kVersionReserved,
		kVersionMPEG2,
		kVersionMPEG1,
	};

	enum Layer
	{
		kLayerReserved,
		kLayer3,
		kLayer2,
		kLayer1
	};

	enum Mode
	{
		kModeStereo,
		kModeJointStereo,
		kModeDualMono,
		kModeMono
	};

	enum Result
	{
		kResultOk,
		kResultEofAtFrameHeader,
		kResultInvalidFrame,
		kResultInvalidVersion,
		kResultInvalidBitRates,
		kResultInvalidSampleRates,
		kResultInvalidLayer,
		kResultEofAtProtectionBits,
		kResultEofAtFrameData,
	};



	//classes

	class Library;

	class Iterator;					//iterate an in memory mp3 file

	//TODO class StreamIterator;	//has internal buffer, used for real-time streaming

	class Decoder;

	class Frame;

	struct Reservoir;

	struct FrameData;



	//integral types

	typedef unsigned char UInt8;

	typedef unsigned short UInt16;

	typedef unsigned int UInt32;

	typedef UInt32 UInt;


	typedef float Float32;

};


//
//library

class OpenMP3::Library
{
public:

	Library();



private:

	friend Decoder;

};





//
//iterator

class OpenMP3::Iterator
{
public:

	//lifetime

	Iterator(const Library & library, kr::KrbFile * file);



	//access

	Result GetNext(Frame & frame);


private:

	struct Private;
	
	kr::KrbFile* m_file;
};



//
//reservoir

struct OpenMP3::Reservoir
{
	void SetPosition(size_t bit_pos);

	size_t GetPosition();

	UInt ReadBit();

	UInt ReadBits(UInt n);

	UInt main_data_vec[2 * 1024];	//Large data	TODO bytes
	UInt *main_data_ptr;			//Pointer into the reservoir
	size_t main_data_idx;				//Index into the current byte(0-7)
	UInt main_data_top;				//Number of bytes in reservoir(0-1024)

	UInt hack_bits_read;
};




//
//framedata

struct OpenMP3::FrameData
{
	unsigned main_data_begin;         /* 9 bits */


									  //side
	struct Granule
	{
		unsigned part2_3_length;    /* 12 bits */
		unsigned big_values;        /* 9 bits */
		Float32 global_gain;       /* 8 bits */
		unsigned scalefac_compress; /* 4 bits */

		bool window_switching;
		bool mixed_block;

		unsigned block_type;        /* 2 bits */
		unsigned table_select[3];   /* 5 bits */
		Float32 subblock_gain[3];  /* 3 bits */
								   /* table_select[] */
		unsigned region0_count;     /* 4 bits */
		unsigned region1_count;     /* 3 bits */
									/* end */
		unsigned preflag;           /* 1 bit */
		unsigned scalefac_scale;    /* 1 bit */
		unsigned count1table_select;/* 1 bit */

		unsigned count1;            //calc: by huff.dec.!

									//main
		unsigned scalefac_l[21];    /* 0-4 bits */
		unsigned scalefac_s[12][3]; /* 0-4 bits */
		float is[576];               //calc: freq lines
	};

	struct Channel
	{
		unsigned scfsi[4];             /* 1 bit */
	};
	Channel channels[2];

	Granule granules[2][2]; // [gr][ch]


};




//
//decoder

class OpenMP3::Decoder
{
public:

	//lifetime
	
	Decoder(const Library & library);

	
	
	//access
	
	void Reset();															//Call before processing new song, resets internal buffers

	UInt ProcessFrame(const Frame & frame, Float32 output[2][1152]);		//return: number samples extracted from frame (0 or 1152)



private:

	struct Private;


	const Library & library;

	Reservoir m_br;

	FrameData m_framedata;

	Float32 m_hs_store[2][32][18];

	Float32 m_sbs_v_vec[2][1024];


	const UInt8 * m_stream_ptr;

	UInt m_stream_remainder;

};




//
//frame

class OpenMP3::Frame
{
public:

	//lifetime

	Frame();
	~Frame();



	//access

	//TODO UInt GetPosition() const;

	UInt GetBitRate() const;

	UInt GetSampleRate() const;

	Mode GetMode() const;

	UInt GetLength() const;

	void SkipFirst() noexcept;

private:

	struct Private;

	friend Iterator;

	friend Decoder;

	Version m_version;

	Layer m_layer;

	UInt8 m_bitrate_index;

	UInt8 m_sr_index;
	
	Mode m_mode;

	UInt8 m_mode_extension;

	UInt8 * m_ptr;		//pointer to data area

	UInt m_datasize;			//size of whole frame, minus headerword + check

	UInt m_length;				//for Info frame skipping

};
