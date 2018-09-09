#ifndef timer_H__
#define timer_H__

#include <stdint.h>
#include <time.h>

static uint64_t timer_ns() {
#if defined(CLOCK_MONOTONIC_RAW)
	const clockid_t clockid = CLOCK_MONOTONIC_RAW;

#else
	const clockid_t clockid = CLOCK_MONOTONIC;

#endif
	timespec t;
	clock_gettime(clockid, &t);

	return 1000000000ULL * t.tv_sec + t.tv_nsec;
}

#endif // timer_H__
