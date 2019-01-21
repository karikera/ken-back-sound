#pragma once

#include "../include/openmp3.h"




//
//external dependencies

#include <math.h>
#include <cstring>	//for memset, memcopy




//
//macros

#define CT_ASSERT(value) {int _ct_assert[value] = {0};}

#ifndef ASSERT
#define ASSERT(x)
#endif 

#ifndef FORCEINLINE
#ifdef __APPLE__
#define FORCEINLINE __attribute((always_inline)) inline
#elif _WIN32
#define FORCEINLINE __forceinline
#elif _WIN64
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE inline
#endif
#endif




//
//declarations

namespace OpenMP3
{

	typedef double Float64;
	


	//helper functions

	inline void MemClear(void * address, size_t size)
	{
		memset(address, 0, size);
	}

	inline void MemCopy(void * destination, const void * source, size_t size)
	{
		memcpy(destination, source, size);
	}

	inline unsigned Max(unsigned a, unsigned b)
	{
		return a > b ? a : b;
	}
	inline unsigned Min(unsigned a, unsigned b)
	{
		return a < b ? a : b;
	}
}




//
//impl

inline void OpenMP3::Reservoir::SetPosition(size_t bit_pos)
{
	main_data_ptr = &(main_data_vec[bit_pos >> 3]);

	main_data_idx = bit_pos & 0x7;
}

FORCEINLINE size_t OpenMP3::Reservoir::GetPosition()
{
	size_t pos = ((size_t)main_data_ptr) - ((size_t) &(main_data_vec[0]));

	return (pos * 2) + main_data_idx;
}

FORCEINLINE OpenMP3::UInt OpenMP3::Reservoir::ReadBit()
{
	hack_bits_read++;

	UInt tmp = main_data_ptr[0] >> (7 - main_data_idx);

	tmp &= 0x01;

	main_data_ptr += (main_data_idx + 1) >> 3;

	main_data_idx = (main_data_idx + 1) & 0x07;

	return tmp;
}

FORCEINLINE OpenMP3::UInt OpenMP3::Reservoir::ReadBits(UInt n)	//number_of_bits to read(max 24).  bits are returned in the LSB of the return value
{
	hack_bits_read += n;

	if (n == 0) return 0;

	/* Form a word of the next four bytes */
	UInt tmp = (main_data_ptr[0] << 24) | (main_data_ptr[1] << 16) | (main_data_ptr[2] << 8) | (main_data_ptr[3] << 0);

	/* Remove bits already used */
	tmp = tmp << main_data_idx;

	/* Remove bits after the desired bits */
	tmp = tmp >> (32 - n);

	/* Update pointers */
	main_data_ptr += (main_data_idx + n) >> 3;
	main_data_idx = (main_data_idx + n) & 0x07;

	return tmp;
}
