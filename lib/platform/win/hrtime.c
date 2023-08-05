#include <gc-helper.h>

#include <windows.h>

static uint64_t frequency;

void init_hrtime(void)
{
    LARGE_INTEGER tmp;
    BOOL ok = QueryPerformanceFrequency(&tmp);
    if (!ok) {
        gcl_err("QueryPerformanceFrequency failed" GC_LOCFMT, GC_LOC);
        return;
    }

    frequency = tmp.QuadPart;
}

void done_hrtime(void)
{
}

uint64_t gc_hrtime(void)
{
    if (frequency == 0) {
        gcl_err("frequency is not initialized" GC_LOCFMT, GC_LOC);
    }

    LARGE_INTEGER timestamp;
    BOOL ok = QueryPerformanceCounter(&timestamp);
    if (!ok) {
        gcl_err("QueryPerformanceCounter failed" GC_LOCFMT, GC_LOC);
        return 0;
    }

    return 1000 * 1000 * 1000 * timestamp.QuadPart / frequency;
}

void gc_sleep(uint64_t ms)
{
    Sleep(ms);
}
