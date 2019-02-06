//#pragma once
//
//#include "common.h"
//
//_KR_C_MODE_BEGIN
//
//struct tm_unz_s;
//
//KEN_EXTERNAL void unzip();
//
//namespace kr
//{
//	class Unzipper
//	{
//	public:
//		Unzipper();
//		virtual ~Unzipper();
//
//		bool open(pcstr16 szFileName);
//		int extract(pcstr16 szDestination);
//		void close();
//
//	private:
//		int _extractCurrentFile(pcstr16 dest);
//		int _writeFile(File* file) noexcept;
//		void _changeFileDate(pcstr16 filename, dword dosdate, const tm_unz_s* tmu_date);
//
//	private:
//		void* m_uzFile;
//	};
//}
//
//
//
//_KR_C_MODE_END
