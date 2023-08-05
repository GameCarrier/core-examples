#ifndef GAME_CARRIER_POSIX_H_INCLUDED
#define GAME_CARRIER_POSIX_H_INCLUDED

/* endian.h */

#define htobe32(x) _byteswap_ulong(x)
#define be32toh(x) _byteswap_ulong(x)
#define htobe64(x) _byteswap_uint64(x)



/* string.h */

#define strncasecmp(x,y,z) _strnicmp(x,y,z)
#define strdup(x) _strdup(x)



/* getopt.h */

#define no_argument        0
#define required_argument  1
#define optional_argument  2

struct option {
    const char *name;
    int has_arg;
    int *flag;
    int val;
};

int getopt(
	int argc, char * const argv[],
	const char *optstring);

int getopt_long(
	int argc, char * const argv[],
	const char *optstring,
    const struct option *longopts,
	int *longindex);

int getopt_long_only(
	int argc, char * const argv[],
	const char *optstring,
    const struct option *longopts,
	int *longindex);

extern char *optarg;
extern int optind, opterr, optopt;

#endif // #ifndef GAME_CARRIER_POSIX_H_INCLUDED
