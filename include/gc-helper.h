#ifndef GAME_CARRIER_HELPER_H_INCLUDED
#define GAME_CARRIER_HELPER_H_INCLUDED

#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
#define restrict
#endif // #ifdef __cplusplus

#ifdef _MSC_VER
#include <gc-posix.h>
#else
#ifdef __APPLE__
#include <machine/endian.h>
#include <libkern/OSByteOrder.h>
#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)
#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
#else
#include <endian.h>
#endif // __APPLE__
#include <getopt.h>
#endif

#if defined __has_attribute
#  if __has_attribute (format)
#      define GCL_LOG_1_2 __attribute__ ((format (printf, 1, 2)))
#      define GCL_LOG_2_3 __attribute__ ((format (printf, 2, 3)))
#  endif
#endif

#ifndef GCL_LOG_1_2
#define GCL_LOG_1_2
#define GCL_LOG_2_3
#endif


#if defined(WIN32) || defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#define IS_WIN 1
#else
#define IS_WIN 0
#endif

#if IS_WIN
#define DIR_SEPARATOR '\\'
#else
#define DIR_SEPARATOR '/'
#endif

#define FILENAME (strrchr(__FILE__, DIR_SEPARATOR) ? strrchr(__FILE__, DIR_SEPARATOR) + 1 : __FILE__)
#define GC_LOC __func__, FILENAME, __LINE__
#define GC_LOCFMT " in %s %s:%d."

#define likely(x) (x)

int check_sha256(const void * data, int len);
void write_sha256(void * restrict data, int len);

uint64_t gc_hrtime(void);
void gc_sleep(uint64_t ns);

char * gc_vlprintf(int * len, const char * fmt, va_list args);
char * gc_vprintf(const char * fmt, va_list args);
char * gc_lprintf(int * len, const char * fmt, ...) GCL_LOG_2_3;
char * gc_printf(const char * fmt, ...) GCL_LOG_1_2;

void gc_vlog(int level, const char * fmt, va_list args);
void gcl_err(const char * fmt, ...) GCL_LOG_1_2;
void gcl_warn(const char * fmt, ...) GCL_LOG_1_2;
void gcl_user(const char * fmt, ...) GCL_LOG_1_2;
void gcl_notice(const char * fmt, ...) GCL_LOG_1_2;
void gcl_info(const char * fmt, ...) GCL_LOG_1_2;
void gcl_debug(const char * fmt, ...) GCL_LOG_1_2;

#endif // #ifndef GAME_CARRIER_HELPER_H_INCLUDED
