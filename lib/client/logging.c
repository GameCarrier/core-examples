#include <gc-client-helper.h>

#include <stdlib.h>
#include <stdio.h>

#define DEF_BUF_SIZE 1024

void gcl_err(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    gc_vlog(GCL_ERR, fmt, args);
    va_end(args);
}

void gcl_warn(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    gc_vlog(GCL_WARN, fmt, args);
    va_end(args);
}

void gcl_user(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    gc_vlog(GCL_USER, fmt, args);
    va_end(args);
}

void gcl_notice(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    gc_vlog(GCL_NOTICE, fmt, args);
    va_end(args);
}

void gcl_info(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    gc_vlog(GCL_DEBUG, fmt, args);
    va_end(args);
}

void gcl_debug(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    gc_vlog(GCL_DEBUG, fmt, args);
    va_end(args);
}

void gc_vlog(int level, const char * fmt, va_list args)
{
    char buf[DEF_BUF_SIZE];

    const size_t buf_sz = sizeof(buf);
    int printed1 = vsnprintf(buf, buf_sz, fmt, args);
    if (printed1 < 0) {
        gc_log_message(GCL_ERR, "Assertion failed, vsnprintf return negative value.");
        return;
    }

    size_t msg_chars = printed1;
    if (likely(msg_chars < buf_sz)) {
        gc_log_message(level, buf);
        return;
    }

    size_t abuf_sz = msg_chars + 1;
    char * abuf = malloc(abuf_sz);
    if (abuf == NULL) {
        gc_log_message(GCL_ERR, "OOM in vlog: cannot allocate memory for abuf.");
        return;
    }

    int printed2 = vsnprintf(abuf, abuf_sz, fmt, args);
    if (printed1 != printed2) {
        gc_log_message(GCL_ERR, "Assertion failed in vlog: printed1 != printed2.");
        return;
    }

    gc_log_message(level, abuf);
    free(abuf);
}
