#include <gc-helper.h>

#include <stdio.h>

char * gc_vlprintf(int * len, const char * fmt, va_list args)
{
    int dummy;
    if (len == NULL) {
        /* Do not worry about NULL from this point */
        len = &dummy;
    }

    int needed = *len = vsnprintf(NULL, 0, fmt, args);
    if (needed < 0) {
        return NULL;
    }

    char * result = malloc(needed + 1);
    if (result == NULL) {
        *len = -1;
        return NULL;
    }

    *len = vsnprintf(result, needed + 1, fmt, args);
    return result;
}

char * gc_vprintf(const char * fmt, va_list args)
{
    int dummy;
    return gc_vlprintf(&dummy, fmt, args);
}

char * gc_lprintf(int * len, const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char * result = gc_vlprintf(len, fmt, args);
    va_end(args);
    return result;
}

char * gc_printf(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char * result = gc_vprintf(fmt, args);
    va_end(args);
    return result;
}
