#include "pch.h"
#include "CppUnitTest.h"

#include "../ken-res-loader/include/compress.h"
#include <vector>
using namespace kr;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace test
{
	TEST_CLASS(test)
	{
	public:
		
		TEST_METHOD(loadzip)
		{
			struct Entry
			{
				std::string name;
				bool isdir;
			};
			struct CbWithParam:KrbCompressCallback
			{
				std::vector<Entry> entries;
			};
			CbWithParam cb;
			cb.entry = [](KrbCompressCallback* _this, KrbCompressFileInfo* _info){
				static_cast<CbWithParam*>(_this)->entries.push_back({
					_info->filename,
					_info->isDirectory
					});
			};
			{
				KrbFile file;
				bool file_open = krb_fopen(&file, L"../../../test/test.zip", L"rb");
				Assert::IsTrue(file_open, L"test.zip not found");

				bool compressLoaded = krb_load_compress(KrbExtension::CompressZip, &cb, &file);
				Assert::IsTrue(compressLoaded, L"Compress not Loaded");
				Assert::AreEqual(cb.entries.size(), (size_t)4, L"entry count");

				Assert::IsTrue(cb.entries[0].isdir, L"dir is not dir");
				Assert::AreEqual(cb.entries[0].name.c_str(), "dir", L"dir name");
				Assert::IsFalse(cb.entries[1].isdir, L"test1.txt is dir");
				Assert::AreEqual(cb.entries[1].name.c_str(), "dir/test1.txt", L"name1");
				Assert::IsFalse(cb.entries[2].isdir, L"test2.txt is dir");
				Assert::AreEqual(cb.entries[2].name.c_str(), "dir/test2.txt", L"name2");
				Assert::IsFalse(cb.entries[3].isdir, L"test3.txt is dir");
				Assert::AreEqual(cb.entries[3].name.c_str(), "dir/test3.txt", L"name3");
			}

		}
	};
}
