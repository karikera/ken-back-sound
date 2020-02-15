
#include "filetime.h"


using namespace kr;

#ifdef WIN32

static_assert(sizeof(filetime_t) == sizeof(FILETIME), "file time unmatched");

#else 

#include <time.h>
#include <sys/time.h>



constexpr uint64_t SECSPERDAY = 86400;
constexpr uint64_t TICKSPERSEC = 10000000;
constexpr uint64_t SECS_1601_TO_1970 = ((369 * 365 + 89) * SECSPERDAY);
constexpr uint64_t TICKS_1601_TO_1970 = (SECS_1601_TO_1970 * TICKSPERSEC);
constexpr uint64_t EPOCH_DIFF = 11644473600LL;

bool DosDateTimeToFileTime(uint16_t fatdate, uint16_t fattime, FILETIME* ft) noexcept {
	tm newtm;
	newtm.tm_sec = (fattime & 0x1f) * 2;
	newtm.tm_min = (fattime >> 5) & 0x3f;
	newtm.tm_hour = (fattime >> 11);
	newtm.tm_mday = (fatdate & 0x1f);
	newtm.tm_mon = ((fatdate >> 5) & 0x0f) - 1;
	newtm.tm_year = (fatdate >> 9) + 80;
	time_t time1 = mktime(&newtm);
	tm* gtm = gmtime(&time1);
	time_t time2 = mktime(gtm);

	*ft = (2 * time1 - time2) * TICKSPERSEC + TICKS_1601_TO_1970;
	return true;
}

void GetSystemTimeAsFileTime(FILETIME* dest) noexcept {
	timeval tv;
	uint64_t result = EPOCH_DIFF;

	gettimeofday(&tv, NULL);
	result += tv.tv_sec;
	result *= 10000000LL;
	result += tv.tv_usec * 10;
	*dest = result;
}

static int TIME_GetBias(time_t utc, int* pdaylight) {
	static time_t last_utc = 0;
	static int last_bias;
	static int last_daylight;

	int ret;

	if (utc == last_utc) {
		*pdaylight = last_daylight;
		ret = last_bias;
	}
	else {
		tm* ptm = localtime(&utc);
		*pdaylight = last_daylight = ptm->tm_isdst; /* daylight for local timezone */
		ptm = gmtime(&utc);
		ptm->tm_isdst = *pdaylight; /* use local daylight, not that of Greenwich */
		last_utc = utc;
		ret = last_bias = (int)(utc - mktime(ptm));
	}
	return ret;
}

bool LocalFileTimeToFileTime(const FILETIME* localft, FILETIME* utcft) {

	time_t gmt = time(nullptr);

	int daylight;
	int bias = TIME_GetBias(gmt, &daylight);

	*utcft = *localft - bias * 10000000LL;

	return true;
}
#endif
