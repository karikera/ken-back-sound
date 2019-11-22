#pragma once

#include "include/common.h"

namespace kr
{
	namespace backend
	{
		class TempBuffer
		{
		public:
			TempBuffer(size_t reserve) noexcept;
			~TempBuffer() noexcept;

			void* operator()(size_t size) noexcept;

		private:
			void* m_buffer;
			size_t m_size;

		};
		
		class ReadStream
		{
		public:
			ReadStream(KrbFile* file) noexcept;

			uint32_t read32() noexcept;

			bool testSignature(uint32_t signature) noexcept;

			uint32_t findChunk(uint32_t signature) noexcept;

			bool read(void* value, uintptr_t size) noexcept;
			bool readStructure(void* value, uintptr_t size, uintptr_t sizeInFile) noexcept;
			
		private:
			KrbFile* m_file;
		};
	}
}

