#pragma once

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#endif

#include "include/compress.h"

#ifndef WIN32

using FILETIME = kr::filetime_t;

bool DosDateTimeToFileTime(uint16_t fatdate, uint16_t fattime, FILETIME* ft) noexcept;
void GetSystemTimeAsFileTime(FILETIME* dest) noexcept;
bool LocalFileTimeToFileTime(const FILETIME* localft, FILETIME* utcft);

#endif
