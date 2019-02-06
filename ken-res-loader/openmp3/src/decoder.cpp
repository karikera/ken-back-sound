#include "../include/openmp3.h"

#include "types.h"
#include "tables.h"
#include "requantize.h"
#include "stereo.h"
#include "synthesis.h"
#include "huffman.h"





//nsa debug

#define C_PI 3.14159265358979323846




//
//private

struct OpenMP3::Decoder::Private
{
	static bool ReadSideInfo(Decoder & self, const Frame & header);

	static bool ReadMain(Decoder & self, const Frame & header);

	static void DecodeFrame(Decoder & self, const Frame & header, Float32 out576[2][1152]);

	static void ReadBytes(Decoder & self, UInt no_of_bytes, UInt data_vec[]);

	static const UInt8 * ReadBytes(Decoder & self, UInt nyytes);
};




//
//

OpenMP3::Library::Library()
{
	//for (UInt i = 0; i < 64; i++) for (UInt j = 0; j < 32; j++) m_sbs_n_win[i][j] = Float32(cos(Float64((16 + i) * (2 * j + 1)) * (C_PI / 64.0)));
}




//
//impl

OpenMP3::Decoder::Decoder(const Library & library)
	: library(library)
{	
	Reset();
}

void OpenMP3::Decoder::Reset()
{
	MemClear(&m_br, sizeof(Reservoir));

	MemClear(&m_framedata, sizeof(FrameData));

	MemClear(m_hs_store, sizeof(m_hs_store));

	MemClear(m_sbs_v_vec, sizeof(m_sbs_v_vec));

	//hsynth_init = synth_init = 1;
}

OpenMP3::UInt OpenMP3::Decoder::ProcessFrame(const Frame & frame, Float32 output[2][1152])
{
	ASSERT(frame.m_ptr);

	Reservoir & br = m_br;

	m_stream_ptr = frame.m_ptr;

	if (!Private::ReadSideInfo(*this, frame)) return 0;

	if (Private::ReadMain(*this, frame))	//returns false if not enough data in the bit reservoir
	{
		Private::DecodeFrame(*this, frame, output);

		return frame.m_length;		//constant 1152, if not first Info frame
	}
	else
	{
		return 0;
	}
}




//
//private impl

bool OpenMP3::Decoder::Private::ReadSideInfo(Decoder & self, const Frame & frame)
{
	FrameData & sideinfo = self.m_framedata;

	auto GetSideBits = [](UInt n, const UInt8 * & ptr, UInt & idx)->UInt32 //TODO unify with resorvoir ReadBits
	{
		UInt32 a;	// = *reinterpret_cast<const UInt32*>(ptr);

		UInt8 * p = reinterpret_cast<UInt8*>(&a);

		p[0] = ptr[3];
		p[1] = ptr[2];
		p[2] = ptr[1];
		p[3] = ptr[0];

		a = a << idx;		//Remove bits already used

		a = a >> (32 - n);	//Remove bits after the desired bits

		ptr += (idx + n) >> 3;

		idx = (idx + n) & 0x07;

		return a;
	};

	bool mono = frame.m_mode == OpenMP3::kModeMono;

	UInt nch = (mono ? 1 : 2);

	UInt sideinfo_size = Frame::Private::GetSideInforSize(frame);
	
	const UInt8 * ptr = ReadBytes(self, sideinfo_size);

	UInt idx = 0;

	sideinfo.main_data_begin = GetSideBits(9, ptr, idx);

	GetSideBits(mono ? 5 : 3, ptr, idx);	//skip private bits

												//scale factor selection information
	for (auto & channel : sideinfo.channels)
	{
		for (unsigned & scfsi: channel.scfsi)
		{
			scfsi = GetSideBits(1, ptr, idx);
		}
	}

	for (auto & granules : sideinfo.granules)
	{
		for (auto & granule : granules)
		{
			granule.part2_3_length = GetSideBits(12, ptr, idx);

			granule.big_values = GetSideBits(9, ptr, idx);

			if (granule.big_values > 288) return false;

			granule.global_gain = Float32(GetSideBits(8, ptr, idx));

			granule.scalefac_compress = GetSideBits(4, ptr, idx);

			granule.window_switching = GetSideBits(1, ptr, idx) == 1;

			if (granule.window_switching)
			{
				granule.block_type = GetSideBits(2, ptr, idx);

				granule.mixed_block = GetSideBits(1, ptr, idx) == 1;

				for (UInt region = 0; region < 2; region++)
				{
					granule.table_select[region] = GetSideBits(5, ptr, idx);
				}

				for (UInt window = 0; window < 3; window++)
				{
					granule.subblock_gain[window] = Float32(GetSideBits(3, ptr, idx));
				}

				if ((granule.block_type == 2) && (!granule.mixed_block))
				{
					granule.region0_count = 8; /* Implicit */
				}
				else
				{
					granule.region0_count = 7; /* Implicit */
				}

				/* The standard is wrong on this!!! */   /* Implicit */
				granule.region1_count = 20 - granule.region0_count;
			}
			else
			{
				for (UInt region = 0; region < 3; region++)
				{
					granule.table_select[region] = GetSideBits(5, ptr, idx);
				}

				granule.region0_count = GetSideBits(4, ptr, idx);

				granule.region1_count = GetSideBits(3, ptr, idx);

				granule.block_type = 0;  /* Implicit */
			}

			granule.preflag = GetSideBits(1, ptr, idx);

			granule.scalefac_scale = GetSideBits(1, ptr, idx);

			granule.count1table_select = GetSideBits(1, ptr, idx);
		}
	}

	//UInt n = ptr - start;

	return true;
}

bool OpenMP3::Decoder::Private::ReadMain(Decoder & self, const Frame & frame)
{
	Reservoir & br = self.m_br;

	FrameData & data = self.m_framedata;

	struct Local
	{
		static bool ReadMainData(Decoder & self, Reservoir & br, UInt main_data_begin, UInt main_data_size)
		{
			if (main_data_begin > br.main_data_top)
			{
				/* No,there is not,so we skip decoding this frame,but we have to
				* read the main_data bits from the bitstream in case they are needed
				* for decoding the next frame. */
				ReadBytes(self, main_data_size, &(br.main_data_vec[br.main_data_top]));

				/* Set up pointers */
				br.main_data_ptr = &(br.main_data_vec[0]);
				br.main_data_idx = 0;
				br.main_data_top += main_data_size;

				return false;    /* This frame cannot be decoded! */
			}

			/* Copy data from previous frames */
			for (UInt i = 0; i < main_data_begin; i++) br.main_data_vec[i] = br.main_data_vec[br.main_data_top - main_data_begin + i];

			/* Read the main_data from file */
			ReadBytes(self, main_data_size, br.main_data_vec + main_data_begin);

			/* Set up pointers */
			br.main_data_ptr = &(br.main_data_vec[0]);
			br.main_data_idx = 0;
			br.main_data_top = main_data_begin + main_data_size;

			return true;
		}
	};


	UInt nch = (frame.m_mode == OpenMP3::kModeMono ? 1 : 2);

	UInt sideinfo_size = Frame::Private::GetSideInforSize(frame);

	UInt main_data_size = frame.m_datasize - sideinfo_size;


	//Assemble the main data buffer with data from this frame and the previous two frames into a local buffer used by the Get_Main_Bits function
	//main_data_begin indicates how many bytes from previous frames that should be used
	if (!Local::ReadMainData(self, br, data.main_data_begin, main_data_size)) return false; //This could be due to not enough data in reservoir

	br.hack_bits_read = 0;


	UInt sfb;

	for (UInt gr = 0; gr < 2; gr++)
	{
		auto & granules = data.granules[gr];
		for (UInt ch = 0; ch < nch; ch++)
		{
			auto & granule = granules[ch];
			auto & channel = data.channels[ch];
			size_t part_2_start = br.GetPosition();

			/* Number of bits in the bitstream for the bands */

			//UInt slen1 = kScaleFactorSizes[data.scalefac_compress[gr][ch]][0];

			//UInt slen2 = kScaleFactorSizes[data.scalefac_compress[gr][ch]][1];

			auto scalefactors = kScaleFactorSizes[granule.scalefac_compress];

			UInt slen1 = scalefactors[0];

			UInt slen2 = scalefactors[1];

			if (granule.window_switching && (granule.block_type == 2))
			{
				if (granule.mixed_block)
				{
					for (sfb = 0; sfb < 8; sfb++)
					{
						granule.scalefac_l[sfb] = br.ReadBits(slen1);
					}
				}

				//for (sfb = 0; sfb < 12; sfb++)
				//{
				//	UInt nbits = (sfb < 6) ? slen1 : slen2; //TODO optimise, slen1 for band 3-5, slen2 for 6-11

				//	for (UInt win = 0; win < 3; win++) scalefac_s_gr[ch][sfb][win] = br.ReadBits(nbits);
				//}

				for (sfb = 0; sfb < 6; sfb++)
				{
					for (UInt win = 0; win < 3; win++)
					{
						granule.scalefac_s[sfb][win] = br.ReadBits(slen1);
					}
				}

				for (sfb = 6; sfb < 12; sfb++)
				{
					for (UInt win = 0; win < 3; win++)
					{
						granule.scalefac_s[sfb][win] = br.ReadBits(slen2);
					}
				}
			}
			else
			{
				auto & granule0 = data.granules[0][ch];
				auto & granule1 = data.granules[1][ch];
				/* block_type == 0 if winswitch == 0 */
				/* Scale factor bands 0-5 */
				if ((channel.scfsi[0] == 0) || (gr == 0))
				{
					for (sfb = 0; sfb < 6; sfb++) granule.scalefac_l[sfb] = br.ReadBits(slen1);
				}
				else if ((channel.scfsi[0] == 1) && (gr == 1))
				{
					/* Copy scalefactors from granule 0 to granule 1 */
					MemCopy(granule1.scalefac_l, granule0.scalefac_l, 6 * sizeof(unsigned));
				}

				/* Scale factor bands 6-10 */
				if ((channel.scfsi[1] == 0) || (gr == 0))
				{
					for (sfb = 6; sfb < 11; sfb++) granule.scalefac_l[sfb] = br.ReadBits(slen1);
				}
				else if ((channel.scfsi[1] == 1) && (gr == 1))
				{
					/* Copy scalefactors from granule 0 to granule 1 */
					MemCopy(granule1.scalefac_l+6, granule0.scalefac_l+6, 5 * sizeof(unsigned));
				}

				/* Scale factor bands 11-15 */
				if ((channel.scfsi[2] == 0) || (gr == 0))
				{
					for (sfb = 11; sfb < 16; sfb++) granule.scalefac_l[sfb] = br.ReadBits(slen2);
				}
				else if ((channel.scfsi[2] == 1) && (gr == 1))
				{
					/* Copy scalefactors from granule 0 to granule 1 */
					MemCopy(granule1.scalefac_l + 11, granule0.scalefac_l + 11, 5 * sizeof(unsigned));
				}

				/* Scale factor bands 16-20 */
				if ((channel.scfsi[3] == 0) || (gr == 0))
				{
					for (sfb = 16; sfb < 21; sfb++) granule.scalefac_l[sfb] = br.ReadBits(slen2);
				}
				else if ((channel.scfsi[3] == 1) && (gr == 1))
				{
					/* Copy scalefactors from granule 0 to granule 1 */
					MemCopy(granule1.scalefac_l + 16, granule0.scalefac_l + 16, 5 * sizeof(unsigned));
				}
			}

			granule.count1 = ReadHuffman(br, frame.m_sr_index, granule, part_2_start);
		}
	}

	//TODO read ancillary data here

	//UInt bytes = (br.hack_bits_read / 8) + (br.hack_bits_read & 7 ? 1 : 0);

	//const UInt8 * ptr = frame.m_ptr + sideinfo_size + data.main_data_begin + bytes;

	//size_t size = (frame.m_ptr + frame.m_datasize) - ptr;

	return true;
}

void OpenMP3::Decoder::Private::DecodeFrame(Decoder & self, const Frame & header, float out[2][1152])
{
	FrameData & data = self.m_framedata;

	UInt sfreq = header.m_sr_index;

	auto & store = self.m_hs_store;

	auto & v_vec = self.m_sbs_v_vec;

	if (header.m_mode == OpenMP3::kModeMono)
	{
		for (UInt gr = 0; gr < 2; gr++)
		{
			auto & granule = data.granules[gr][0];

			Requantize(sfreq, granule);

			Reorder(sfreq, granule);

			Antialias(granule);

			HybridSynthesis(granule, store[0]);

			FrequencyInversion(granule.is);

			SubbandSynthesis(data, granule.is, v_vec[0], out[0] + (576 * gr));
		}

		memcpy(out[1], out[0], 1152 * sizeof(Float32));
	}
	else
	{
		UInt8 joint_stereo_mode = header.m_mode_extension;

		bool stereo_decoding = (header.m_mode == 1) && (joint_stereo_mode != 0);	//joint_stereo & (Intensity stereo | MS stereo)

		for (UInt gr = 0; gr < 2; gr++)
		{
			for (UInt ch = 0; ch < 2; ch++)
			{
				auto & granule = data.granules[gr][ch];

				Requantize(sfreq, granule);

				Reorder(sfreq, granule);
			}

			if (stereo_decoding) Stereo(sfreq, joint_stereo_mode, data, gr);

			for (UInt ch = 0; ch < 2; ch++)
			{
				auto & granule = data.granules[gr][ch];

				Antialias(granule);

				HybridSynthesis(granule, store[ch]);

				FrequencyInversion(granule.is);

				SubbandSynthesis(data, granule.is, v_vec[ch], out[ch] + (576 * gr));
			}
		}
	}
}

void OpenMP3::Decoder::Private::ReadBytes(Decoder & self, UInt no_of_bytes, UInt data_vec[])
{
	//TODO this should return pointer to bytes, not upscale to UInt32

	for (UInt i = 0; i < no_of_bytes; i++)
	{
		data_vec[i] = *self.m_stream_ptr++;
	}
}

FORCEINLINE const OpenMP3::UInt8 * OpenMP3::Decoder::Private::ReadBytes(Decoder & self, UInt nbytes)
{
	const UInt8 * ptr = self.m_stream_ptr;

	self.m_stream_ptr += nbytes;

	return ptr;
}