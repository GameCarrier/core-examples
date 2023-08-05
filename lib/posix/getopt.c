#include <gc-posix.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


char *optarg = NULL;
int optind = 1;
int opterr = 1;
int optopt = '?';
static char *optnext = NULL;
static int posixly_correct = -1;
static int handle_nonopt_argv = 0;

static void getopt_error(const char *optstring, const char *format, ...);
static void check_gnu_extension(const char *optstring);
static int getopt_internal(int argc, char * const argv[], const char *optstring, const struct option *longopts, int *longindex, int long_only);
static int getopt_shortopts(int argc, char * const argv[], const char *optstring, int long_only);
static int getopt_longopts(int argc, char * const argv[], char *arg, const char *optstring, const struct option *longopts, int *longindex, int *long_only_flag);

static void getopt_error(const char *optstring, const char *format, ...)
{
    if (opterr && optstring[0] != ':') {
        va_list ap;
        va_start(ap, format);
        vfprintf(stderr, format, ap);
        va_end(ap);
    }
}

static void check_gnu_extension(const char *optstring)
{
    if (optstring[0] == '+' || getenv("POSIXLY_CORRECT") != NULL) {
        posixly_correct = 1;
    } else {
        posixly_correct = 0;
    }
    if (optstring[0] == '-') {
        handle_nonopt_argv = 1;
    } else {
        handle_nonopt_argv = 0;
    }
}

static int is_option(const char *arg)
{
    return arg[0] == '-' && arg[1] != '\0';
}

int getopt(int argc, char * const argv[], const char *optstring)
{
    return getopt_internal(argc, argv, optstring, NULL, NULL, 0);
}

int getopt_long(int argc, char * const argv[], const char *optstring, const struct option *longopts, int *longindex)
{
    return getopt_internal(argc, argv, optstring, longopts, longindex, 0);
}

int getopt_long_only(int argc, char * const argv[], const char *optstring, const struct option *longopts, int *longindex)
{
    return getopt_internal(argc, argv, optstring, longopts, longindex, 1);
}

static int getopt_internal(int argc, char * const argv[], const char *optstring, const struct option *longopts, int *longindex, int long_only)
{
    static int start, end;

    if (optopt == '?') {
        optopt = 0;
    }

    if (posixly_correct == -1) {
        check_gnu_extension(optstring);
    }

    if (optind == 0) {
        check_gnu_extension(optstring);
        optind = 1;
        optnext = NULL;
    }

    switch (optstring[0]) {
    case '+':
    case '-':
        optstring++;
    }

    if (optnext == NULL && start != 0) {
        int last_pos = optind - 1;

        optind -= end - start;
        if (optind <= 0) {
            optind = 1;
        }
        while (start < end--) {
            int i;
            char *arg = argv[end];

            for (i = end; i < last_pos; i++) {
                ((char **)argv)[i] = argv[i + 1];
            }
            ((char const **)argv)[i] = arg;
            last_pos--;
        }
        start = 0;
    }

    if (optind >= argc) {
        optarg = NULL;
        return -1;
    }
    if (optnext == NULL) {
        const char *arg = argv[optind];
        if (!is_option(arg)) {
            if (handle_nonopt_argv) {
                optarg = argv[optind++];
                start = 0;
                return 1;
            } else if (posixly_correct) {
                optarg = NULL;
                return -1;
            } else {
                int i;

                start = optind;
                for (i = optind + 1; i < argc; i++) {
                    if (is_option(argv[i])) {
                        end = i;
                        break;
                    }
                }
                if (i == argc) {
                    optarg = NULL;
                    return -1;
                }
                optind = i;
                arg = argv[optind];
            }
        }
        if (strcmp(arg, "--") == 0) {
            optind++;
            return -1;
        }
        if (longopts != NULL && arg[1] == '-') {
            return getopt_longopts(argc, argv, argv[optind] + 2, optstring, longopts, longindex, NULL);
        }
    }

    if (optnext == NULL) {
        optnext = argv[optind] + 1;
    }
    if (long_only) {
        int long_only_flag = 0;
        int rv = getopt_longopts(argc, argv, optnext, optstring, longopts, longindex, &long_only_flag);
        if (!long_only_flag) {
            optnext = NULL;
            return rv;
        }
    }

    return getopt_shortopts(argc, argv, optstring, long_only);
}

static int getopt_shortopts(int argc, char * const argv[], const char *optstring, int long_only)
{
    int opt = *optnext;
    const char *os = strchr(optstring, opt);

    if (os == NULL) {
        optarg = NULL;
        if (long_only) {
            getopt_error(optstring, "%s: unrecognized option '-%s'\n", argv[0], optnext);
            optind++;
            optnext = NULL;
        } else {
            optopt = opt;
            getopt_error(optstring, "%s: invalid option -- '%c'\n", argv[0], opt);
            if (*(++optnext) == 0) {
                optind++;
                optnext = NULL;
            }
        }
        return '?';
    }
    if (os[1] == ':') {
        if (optnext[1] == 0) {
            optind++;
            optnext = NULL;
            if (os[2] == ':') {
                /* optional argument */
                optarg = NULL;
            } else {
                if (optind == argc) {
                    optarg = NULL;
                    optopt = opt;
                    getopt_error(optstring, "%s: option requires an argument -- '%c'\n", argv[0], opt);
                    if (optstring[0] == ':') {
                        return ':';
                    } else {
                        return '?';
                    }
                }
                optarg = argv[optind];
                optind++;
            }
        } else {
            optarg = optnext + 1;
            optind++;
        }
        optnext = NULL;
    } else {
        optarg = NULL;
        if (optnext[1] == 0) {
            optnext = NULL;
            optind++;
        } else {
            optnext++;
        }
    }
    return opt;
}

static int getopt_longopts(int argc, char * const argv[], char *arg, const char *optstring, const struct option *longopts, int *longindex, int *long_only_flag)
{
    char *val = NULL;
    const struct option *opt;
    size_t namelen;
    int idx;

    for (idx = 0; longopts[idx].name != NULL; idx++) {
        opt = &longopts[idx];
        namelen = strlen(opt->name);
        if (strncmp(arg, opt->name, namelen) == 0) {
            switch (arg[namelen]) {
            case '\0':
                switch (opt->has_arg) {
                case required_argument:
                    optind++;
                    if (optind == argc) {
                        optarg = NULL;
                        optopt = opt->val;
                        getopt_error(optstring, "%s: option '--%s' requires an argument\n", argv[0], opt->name);
                        if (optstring[0] == ':') {
                            return ':';
                        } else {
                            return '?';
                        }
                    }
                    val = argv[optind];
                    break;
                }
                goto found;
            case '=':
                if (opt->has_arg == no_argument) {
                    const char *hyphens = (argv[optind][1] == '-') ? "--" : "-";

                    optind++;
                    optarg = NULL;
                    optopt = opt->val;
                    getopt_error(optstring, "%s: option '%s%s' doesn't allow an argument\n", argv[0], hyphens, opt->name);
                    return '?';
                }
                val = arg + namelen + 1;
                goto found;
            }
        }
    }
    if (long_only_flag) {
        *long_only_flag = 1;
    } else {
        getopt_error(optstring, "%s: unrecognized option '%s'\n", argv[0], argv[optind]);
        optind++;
    }
    return '?';
found:
    optarg = val;
    optind++;
    if (opt->flag) {
        *opt->flag = opt->val;
    }
    if (longindex) {
        *longindex = idx;
    }
    return opt->flag ? 0 : opt->val;
}
