#include "readstream.h"

#include <stdlib.h>
#include <memory.h>

kr::backend::TempBuffer::TempBuffer(size_t reserve) noexcept
	:m_size(reserve), m_buffer(nullptr)
{
}
kr::backend::TempBuffer::~TempBuffer() noexcept
{
	if (m_buffer) free(m_buffer);
}

void* kr::backend::TempBuffer::operator()(size_t size) noexcept
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
kr::backend::ReadStream::ReadStream(krb_file_t * file) noexcept
	:m_file(file)
{
}
uint32_t kr::backend::ReadStream::read32() noexcept
{
	uint32_t sig;
	krb_fread(m_file, &sig, 4);
	return sig;
}
bool kr::backend::ReadStream::testSignature(uint32_t signature) noexcept
{
	return read32() == signature;
}
uint32_t kr::backend::ReadStream::findChunk(uint32_t signature) noexcept
{
	while (!testSignature(signature))
	{
		uint32_t size = read32();
		krb_fseek_cur(m_file, size);
	}
	return read32();
}
bool kr::backend::ReadStream::read(void* value, uintptr_t size) noexcept
{
	size_t readed = krb_fread(m_file, value, size);
	return readed == size;
}

bool kr::backend::ReadStream::readStructure(void* value, uintptr_t size, uintptr_t sizeInFile) noexcept
{
	char* dest = (char*)value;
	if (sizeInFile < size)
	{
		size_t readed = krb_fread(m_file, dest, sizeInFile);
		if (readed != sizeInFile) return false;
		memset(dest + sizeInFile, 0, size - sizeInFile);
		return true;
	}
	else
	{
		size_t readed = krb_fread(m_file, dest, size);
		if (readed != sizeInFile) return false;
		krb_fseek_cur(m_file, sizeInFile - size);
		return true;
	}
}
