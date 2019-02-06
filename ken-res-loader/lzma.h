#pragma once
//
//#include <KR3/main.h>
//#include <KR3/wl/windows.h>
//#include <KRUtil/fs/file.h>
//#include <KR3/io/selfbufferedstream.h>
//#include <tchar.h>
//
//#include <KRThird/lzma-sdk/C/LzmaDec.h>
//
//namespace kr
//{
//	class LzmaVOStream :private ISeqOutStream
//	{
//	public:
//		template <typename Derived, typename _Info>
//		inline LzmaVOStream(OutStream<Derived, void, _Info> * os) noexcept
//			:m_os(os)
//		{
//			Write = [](void *p, const void *buf, size_t size)->size_t {
//				try
//				{
//					LzmaVOStream* _this = (LzmaVOStream*)p;
//					auto * stream = (OutStream<Derived, void, _Info>*)_this->m_os;
//					stream->write((const char*)buf, size);
//					return size;
//				}
//				catch (...)
//				{
//					return 0;
//				}
//			};
//		}
//		ISeqOutStream * getLzmaStream() noexcept;
//
//	private:
//		void * m_os;
//	};
//	class LzmaVIStream :private ISeqInStream
//	{
//	public:
//		template <typename Derived, bool accessable, typename Super>
//		inline LzmaVIStream(InStream<Derived, void, StreamInfo<accessable, Super>> * is, filesize_t size) noexcept
//			:m_is(is), m_leftSize(size)
//		{
//			Read = [](void *p, void *buf, size_t *size)->SRes {
//				try
//				{
//					LzmaVIStream* _this = (LzmaVIStream*)p;
//					filesize_t lsz = _this->m_leftSize;
//					if (lsz == 0)
//					{
//						*size = 0;
//						return SZ_OK;
//					}
//					auto* stream = (InStream<Derived, void, StreamInfo<accessable, Super>> *)_this->m_is;
//
//					size_t sz = *size;
//					if (sz > lsz)
//						sz = (size_t)lsz;
//
//					sz = stream->read((char*)buf, sz);
//					((LzmaVIStream*)p)->m_leftSize -= sz;
//					*size = sz;
//					return SZ_OK;
//				}
//				catch (EofException&)
//				{
//					return SZ_ERROR_INPUT_EOF;
//				}
//				catch (...)
//				{
//					return SZ_ERROR_FAIL;
//				}
//				return SZ_OK;
//			};
//		}
//		ISeqInStream * getLzmaStream() noexcept;
//		filesize_t getLeftSize() noexcept;
//
//	private:
//		void * m_is;
//		filesize_t m_leftSize;
//	};
//
//	class LzmaOStream :public OutStream<LzmaOStream, void>
//	{
//	public:
//
//	private:
//		void * m_enc;
//	};
//	class LzmaIStream:public InStream<LzmaIStream, void>
//	{
//		static constexpr int IN_BUF_SIZE = 4096;
//	public:
//		LzmaIStream(io::VIStream<void> vis) noexcept;
//		~LzmaIStream() noexcept;
//
//		void reset() noexcept;
//		size_t readImpl(void * dest, size_t size);
//		void skipImpl(size_t size);
//
//	private:
//		io::VIStream<void> m_vis;
//		CLzmaDec m_state;
//		BYTE m_inBuf[IN_BUF_SIZE];
//		size_t m_inPos;
//		size_t m_inSize;
//
//	};
//	
//	void compressData(void *dest, size_t *destLen, const void *src, size_t srcLen);
//	void decompressData(void *dest, size_t *destLen, const void *src, size_t srcLen);
//
//	ABuffer decompressData(Buffer src);
//
//
//#pragma pack(push)
//#pragma pack(1)
//	struct LZMA_PROP
//	{
//		BYTE data[5];
//	};
//#pragma pack(pop)
//
//	void getLZMAProperty(LZMA_PROP * pDest);
//
//	/**
//	@throws {Error}
//	*/
//	void compressFile(pcstr16 strDest, pcstr16 strSource);
//
//	void compressStream(LzmaVOStream vo, LzmaVIStream vi);
//
//	void decompressStream(io::VOStream<void> vo, io::VIStream<void> vi, size_t inSize);
//
//}
