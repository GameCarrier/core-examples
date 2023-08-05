#include <gc-client-helper.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char * app_name;
int opt_verbose;

extern void init_hrtime(void);
extern void done_hrtime(void);

int gc_client_helper_initialize(void)
{
    int level = GCL_ERR | GCL_WARN | GCL_NOTICE | GCL_USER;
    int flags = GCL_TO_STDERR | GCL_PRINT_THREAD_ID | GCL_AUTO_FLUSH;

    int extra_levels[] = { GCL_INFO, GCL_DEBUG, -1 };
    for (int i=0; i<opt_verbose; ++i) {
        int bit = extra_levels[i];
        if (bit < 0) {
            break;
        }
        level |= bit;
    }

    // Log file name is {app_name}.%p.log\0
    int needed = strlen(app_name) + 8;
    char * log_fn = malloc(needed);
    if (log_fn == NULL) {
        return ENOMEM;
    }

    snprintf(log_fn, needed, "%s.%%p.log", app_name);
    gc_log_setopt(level, log_fn, flags);
    free(log_fn);

    init_hrtime();

    return 0;
}

void gc_client_helper_finalize(void)
{
    done_hrtime();
    gc_clients_cleanup();
}

static int read_proto(const char * s, int len)
{
    struct proto_str
    {
        const char * str;
        int mode;
    } *ptr, proto_strs[] = {
        { "wss", GC_PROTOCOL_WSS },
        { "ws", GC_PROTOCOL_WS },
        { NULL, -1 }
    };

    for (ptr = proto_strs; ptr->str; ++ptr) {
        if (strlen(ptr->str) != len) {
            continue;
        }
        if (strncasecmp(ptr->str, s, len) == 0) {
            return ptr->mode;
        }
    }
    return -1;
}

struct gc_uri * parse_uri(const char * uri, int def_proto, int def_port)
{
    int proto = def_proto;
    char * host = NULL;
    int port = def_port;
    char * app = NULL;

    int proto_len = -1;
    const char * ptr = strstr(uri, "://");
    if (ptr != NULL) {
        proto_len = ptr - uri;
        ptr += 3;
    } else {
        ptr = uri;
    }

    if (proto_len >= 0) {
        proto = read_proto(uri, proto_len);
        if (proto < 0) {
            gcl_warn("Protocol “%*.*s” is not found.", proto_len, proto_len, uri);
            goto bad;
        }
    }

    const char * ptr2 = strstr(ptr, ":");
    if (ptr2 != NULL) {
        int host_sz = ptr2 - ptr;

        char * ptr3;
        port = strtol(ptr2 + 1, &ptr3, 10);
        if (ptr3 == ptr2 + 1) {
            gcl_err("Port number is expected for uri “%s”" GC_LOCFMT, uri, GC_LOC);
            goto bad;
        }

        if (*ptr3 != '/' && *ptr3 != '\0') {
            gcl_err("Invalid port number for uri “%s”" GC_LOCFMT, uri, GC_LOC);
            goto bad;
        }

        host = malloc(host_sz + 1);
        if (host == NULL) {
            gcl_err("Cannot allocate %d bytes for host" GC_LOCFMT, host_sz + 1, GC_LOC);
            goto bad;
        }

        strncpy(host, ptr, host_sz);
        host[host_sz] = '\0';

        if (ptr3[0] == '/') {
            app = strdup(ptr3 + 1);
        }

    } else {
        const char * ptr2 = strstr(ptr, "/");
        if (ptr2 != NULL) {
            int host_sz = ptr2 - ptr;

            host = malloc(host_sz + 1);
            if (host == NULL) {
                gcl_err("Cannot allocate %d bytes for host" GC_LOCFMT, host_sz + 1, GC_LOC);
                goto bad;
            }

            strncpy(host, ptr, host_sz);
            host[host_sz] = '\0';
            app = strdup(ptr2 + 1);
        } else {
            host = strdup(ptr);
        }
    }

    struct gc_uri * me = malloc(sizeof(struct gc_uri));
    if (me == NULL) {
        gcl_err("Cannot allocate struct gc_uri" GC_LOCFMT, GC_LOC);
        goto bad;
    }

    me->proto = proto;
    me->host = host;
    me->port = port;
    me->app = app;
    return me;

bad:
    free(host);
    free(app);
    return NULL;
}

void destroy_uri(struct gc_uri * me)
{
    if (me == NULL) {
        return;
    }

    free(me->host);
    free(me->app);
    free(me);
}
