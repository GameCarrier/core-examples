#include <gc-helper.h>

#include <time.h>

void init_hrtime(void)
{
}

void done_hrtime(void)
{
}

uint64_t gc_hrtime(void)
{
    struct timespec timespec;
    int status = clock_gettime(CLOCK_MONOTONIC, &timespec);
    if (status != 0) {
        gcl_err("clock_gettime(CLOCK_MONOTONIC, &timespec) failed" GC_LOCFMT, GC_LOC);
        return 0;
    }

    static const uint64_t milliard = 1000 * 1000 * 1000;
    return milliard * timespec.tv_sec + timespec.tv_nsec;
}

void gc_sleep(uint64_t ms)
{
    struct timespec timespec = {
        .tv_sec = ms / 1000,
        .tv_nsec = 1000 * 1000 * (ms % 1000),
    };

    int status = nanosleep(&timespec, &timespec);
    if (status != 0) {
        gcl_err("nanosleep(&timespec, &timespec) failed" GC_LOCFMT, GC_LOC);
    }
}
