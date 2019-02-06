#include "requantize.h"

#include "types.h"
#include "tables.h"




//
//internal

namespace OpenMP3
{

	Float32 Requantize_Pow_43(Float32 is_pos);

	void RequantizeLong(FrameData::Granule & granule, UInt is_pos, UInt sfb);

	void RequantizeShort(FrameData::Granule & granule, UInt is_pos, UInt sfb, UInt win);


	extern const Float32 kPreTab[21];

}




//
//impl

void OpenMP3::Requantize(UInt sfreq, FrameData::Granule & granule)
{
	UInt sfb /* scalefac band index */, next_sfb /* frequency of next sfb */, i, j;

	/* Determine type of block to process */
	if (granule.window_switching && (granule.block_type == 2))	//Short blocks
	{ 
		/* Check if the first two subbands (=2*18 samples = 8 long or 3 short sfb's) uses long blocks */
		if (granule.mixed_block)
		{ 
			/* 2 longbl. sb  first */
			/* First process the 2 long block subbands at the start */

			sfb = 0;

			next_sfb = kScaleFactorBandIndices[sfreq].l[sfb + 1];

			for (i = 0; i < 36; i++) 
			{
				if (i == next_sfb) 
				{
					sfb++;

					next_sfb = kScaleFactorBandIndices[sfreq].l[sfb + 1];
				}

				RequantizeLong(granule, i, sfb);
			}
		
			/* And next the remaining,non-zero,bands which uses short blocks */
			sfb = 3;
			
			next_sfb = kScaleFactorBandIndices[sfreq].s[sfb + 1] * 3;
			
			UInt win_len = kScaleFactorBandIndices[sfreq].s[sfb + 1] - kScaleFactorBandIndices[sfreq].s[sfb];

			for (i = 36; i < granule.count1; /* i++ done below! */)
			{
				/* Check if we're into the next scalefac band */
				if (i == next_sfb)
				{
					ASSERT(sfb < 14);

					sfb++;

					next_sfb = kScaleFactorBandIndices[sfreq].s[sfb + 1] * 3;

					win_len = kScaleFactorBandIndices[sfreq].s[sfb + 1] - kScaleFactorBandIndices[sfreq].s[sfb];
				} 

				for (UInt win = 0; win < 3; win++) for (j = 0; j < win_len; j++) RequantizeShort(granule, i++, sfb, win);
			}
		}
		else //Only short blocks
		{ 
			sfb = 0;

			next_sfb = kScaleFactorBandIndices[sfreq].s[sfb + 1] * 3;

			UInt win_len = kScaleFactorBandIndices[sfreq].s[sfb + 1] - kScaleFactorBandIndices[sfreq].s[sfb];

			for (i = 0; i < granule.count1; /* i++ done below! */)
			{
				/* Check if we're into the next scalefac band */
				if (i == next_sfb) 
				{
					ASSERT(sfb < 14);

					sfb++;
					
					next_sfb = kScaleFactorBandIndices[sfreq].s[sfb + 1] * 3;
					
					win_len = kScaleFactorBandIndices[sfreq].s[sfb + 1] - kScaleFactorBandIndices[sfreq].s[sfb];
				}

				for (UInt win = 0; win < 3; win++) for (j = 0; j < win_len; j++) RequantizeShort(granule, i++, sfb, win);
			}
		}
	}
	else //Only long blocks
	{ 
		sfb = 0;

		next_sfb = kScaleFactorBandIndices[sfreq].l[sfb + 1];

		UInt n = granule.count1;

		for (i = 0; i < n; i++) 
		{
			if (i == next_sfb) 
			{
				ASSERT(sfb < 23);

				sfb++;

				next_sfb = kScaleFactorBandIndices[sfreq].l[sfb + 1];
			}

			RequantizeLong(granule, i, sfb);
		}
	}
}


void OpenMP3::Reorder(UInt sfreq, FrameData::Granule & granule)
{
	auto & is = granule.is;
	OpenMP3::Float32 re[576];	//TODO use working buffer

	if (granule.window_switching && (granule.block_type == 2))	//Only reorder short blocks
	{
		/* Check if the first two subbands (=2*18 samples = 8 long or 3 short sfb's) uses long blocks */

		auto s = kScaleFactorBandIndices[sfreq].s;

		UInt sfb = granule.mixed_block ? 3 : 0; /* 2 longbl. sb  first */

		UInt count1 = granule.count1;

		UInt next_sfb = s[sfb + 1] * 3;

		UInt win_len = s[sfb + 1] - s[sfb];

		for (UInt i = ((sfb == 0) ? 0 : 36); i < 576; /* i++ done below! */)
		{
			/* Check if we're into the next scalefac band */
			if (i == next_sfb)
			{
				/* Copy reordered data back to the original vector */
				for (UInt j = 0; j < 3 * win_len; j++) is[3 * s[sfb] + j] = re[j];

				/* Check if this band is above the rzero region,if so we're done */
				if (i >= count1) return;

				sfb++;

				next_sfb = s[sfb + 1] * 3;

				win_len = s[sfb + 1] - s[sfb];
			}

			//Do the actual reordering

			for (UInt win = 0; win < 3; win++) for (UInt j = 0; j < win_len; j++) re[j * 3 + win] = is[i++];
		}

		/* Copy reordered data of last band back to original vector */
		for (UInt j = 0; j < 3 * win_len; j++) is[3 * s[12] + j] = re[j];
	}
}

inline float OpenMP3::Requantize_Pow_43(Float32 f_is_pos)
{
	int is_pos = int(f_is_pos);

	return powf((float)is_pos, 4.0f / 3.0f);
}

//requantize sample in subband that uses long blocks
void OpenMP3::RequantizeLong(FrameData::Granule & granule, UInt is_pos, UInt sfb)
{
	ASSERT(is_pos < 576);

	auto & is = granule.is;
	
	float sf_mult = granule.scalefac_scale ? 1.0f : 0.5f;
	
	float pf_x_pt = granule.preflag * kPreTab[sfb];
	
	Float64 tmp1 = pow(2.0, -(sf_mult *(granule.scalefac_l[sfb] + pf_x_pt)));
	
	Float64 tmp2 = pow(2.0, 0.25 * (granule.global_gain - 210.0));

	Float64 tmp3;

	if (is[is_pos] < 0.0)
	{
		tmp3 = -Requantize_Pow_43(-is[is_pos]);
	}
	else
	{
		tmp3 = Requantize_Pow_43(is[is_pos]);
	}

	is[is_pos] = Float32(tmp1 * tmp2 * tmp3);
}

//requantize sample in subband that uses short blocks
void OpenMP3::RequantizeShort(FrameData::Granule & granule, UInt is_pos, UInt sfb, UInt win)
{
	ASSERT(is_pos < 576);

	auto & is = granule.is;

	Float32 sf_mult = granule.scalefac_scale ? 1.0f : 0.5f;

	Float64 tmp1 = pow(2.0, -(sf_mult * granule.scalefac_s[sfb][win]));

	Float64 tmp2 = pow(2.0, 0.25 * (granule.global_gain - 210.0f - 8.0f * granule.subblock_gain[win]));

	Float64 tmp3 = (is[is_pos] < 0.0) ? -Requantize_Pow_43(-is[is_pos]) : Requantize_Pow_43(is[is_pos]);

	is[is_pos] = Float32(tmp1 * tmp2 * tmp3);
}




//
//

const float OpenMP3::kPreTab[21] = { 0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,3,3,3,2 };
