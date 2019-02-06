#pragma once

#include "types.h"


//
//declarations

namespace OpenMP3
{

	struct ScaleFactorBandIndices		//Scale factor band indices,for long and short windows
	{
		UInt l[23];
		UInt s[14];
	};

	struct VersionInfo
	{
		const UInt * kBitRates[3];
		UInt kSampleRates[3];
		UInt kSamplesPerFrame[3];
	};

	extern const VersionInfo kVersions[4];
	
	extern const UInt kBitRates_mpeg1_layer1[15];
	extern const UInt kBitRates_mpeg1_layer2[15];
	extern const UInt kBitRates_mpeg1_layer3[15];
	extern const UInt kBitRates_mpeg2_layer1[15]; // & MPEG-2.5
	extern const UInt kBitRates_mpeg2_layer2[15]; // & MPEG-2.5, Layer3


	extern const ScaleFactorBandIndices kScaleFactorBandIndices[3];

	extern const UInt kScaleFactorSizes[16][2];


	extern const UInt8 kInfo[4];

}