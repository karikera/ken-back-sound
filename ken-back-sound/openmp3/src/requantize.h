#pragma once

#include "types.h"




//
//decl

namespace OpenMP3
{

	void Requantize(UInt sfreq, FrameData::Granule & granule);

	void Reorder(UInt sfreq, FrameData::Granule & granule);

}

