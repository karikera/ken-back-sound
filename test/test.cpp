#include "pch.h"
#include "CppUnitTest.h"

#include "../ken-res-loader/include/compress.h"
#include "../ken-res-loader/include/image.h"
#include "../ken-res-loader/include/sound.h"
#include <vector>
using namespace kr;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

void loadImage(KrbExtension ext, const wchar_t* filepath) noexcept
{
	KrbFile file;
	bool file_open = krb_fopen(&file, filepath, L"rb");
	Assert::IsTrue(file_open, L"resource file not found");

	struct Loader : KrbImageCallback
	{
		void* data;
	};
	Loader loader;
	loader.palette = nullptr;
	loader.start = [](KrbImageCallback* _this, KrbImageInfo* _info)->void* {
		Assert::AreEqual((uint32_t)279, _info->width, L"width size not matched");
		Assert::AreEqual((uint32_t)71, _info->height, L"height size not matched");
		return ((Loader*)_this)->data = new uint32_t[_info->pitchBytes * _info->height];
	};
	bool res = krb_load_image(ext, &loader, &file);
	Assert::IsTrue(res, L"image Load failed");
	delete[] loader.data;
}

namespace test
{
	TEST_CLASS(test)
	{
	public:

		TEST_METHOD(loadogg)
		{
			KrbFile file;
			bool file_open = krb_fopen(&file, L"../../../test/ogg.ogg", L"rb");
			Assert::IsTrue(file_open, L"resource file not found");

			struct Loader : KrbSoundCallback
			{
				uint8_t* data;
			};
			Loader loader;
			loader.start = [](KrbSoundCallback* _this, KrbSoundInfo* _info)->short*{
				Assert::IsTrue(74.3 < _info->duration && _info->duration < 74.4, L"sound duration not matched");
				return (short*)(((Loader*)_this)->data = new uint8_t[_info->totalBytes]);
			};
			bool res = krb_load_sound(KrbExtension::SoundOgg, &loader, &file);
			Assert::IsTrue(res, L"sound Load failed");
			delete loader.data;
		}
		TEST_METHOD(loadpng)
		{
			loadImage(KrbExtension::ImagePng, L"../../../test/png.png");
		}
		TEST_METHOD(loadjpeg)
		{
			loadImage(KrbExtension::ImageJpg, L"../../../test/jpeg.jpg");
		}
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
			cb.entry = [](KrbCompressCallback* _this, KrbCompressEntry* _info){
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
