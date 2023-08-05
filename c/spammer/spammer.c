#include <gc-client-helper.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MODES         1024
#define MAX_URIES         1024

struct client
{
    intptr_t handle;
    int packets_left;
    int connected;
    int stopped;
};

int opt_help = 0;
int basta = 0;
int base_msg_sz = 128 * 1024;
int relax_time = 3000;
int sleep_time = 10;

int modes[MAX_MODES];
int mode_count = 0;
struct gc_uri * uries[MAX_URIES];
int uri_packets[MAX_URIES];
int uri_count = 0;
int qerrors = 0;
int opt_clients = 1;
struct client clients[MAX_URIES];

static int int_parse(const char * title, const char * s)
{
    char * endptr;
    long value = strtol(s, &endptr, 10);
    if (endptr == NULL || *endptr != '\0') {
        fprintf(stderr, "Invalid value for %s: not a valid integer.\n", title);
        return -1;
    }

    if (value < 0) {
        fprintf(stderr, "Value for %s might be a positive.\n", title);
        return -1;
    }

    return value;
}

static void push_mode(int mode)
{
    if (mode_count >= MAX_MODES) {
        fprintf(stderr, "Too many modes, please, increase MAX_MODES const in %s.\n", __FILE__);
        exit(1);
    }

    modes[mode_count++] = mode;
}

static void update_basta(void)
{
    if (basta) {
        return;
    }

    for (int i=0; i<uri_count; ++i) {
        if (!clients[i].stopped) {
            return;
        }
    }

    basta = 1;
}

struct client * get_client(GCT_INTPTR peer)
{
    if (peer < 0 || peer >= uri_count) {
        return NULL;
    }

    return clients + peer;
}

static struct opt_mode
{
    int value;
    const char * short_flag;
    const char * long_name;
    const char * hint;
} opt_modes[] = {
    { GC_MODE_PASSIVE, "-p", "--passive", "Add clients run in a passive mode." },
    {  GC_MODE_ACTIVE, "-a",  "--active", "Add clients run in an active mode." },
    {  GC_MODE_HYBRID, "-h",  "--hybrid", "Add clients run in a hybrid mode." },
    { -1, NULL, NULL, NULL }
};

void * make_message(int sz)
{
    if (sz < 64) {
        gcl_err("Too small message size %d" GC_LOCFMT, sz, GC_LOC);
        return NULL;
    }
    char * result = malloc(sz);
    if (result == NULL) {
        gcl_err("OOM: cannot allocate %d bytes for message" GC_LOCFMT, sz, GC_LOC);
        return NULL;
    }

    char * end = result + sz - 32;
    char * ptr = result;
    *ptr++ = 'M';
    *ptr++ = ':';
    *ptr++ = ' ';
    for(; ptr != end; ++ptr) {
        *ptr = 32 + rand() % 95;
    }

    /* Two first bytes of the message are excluded from control sum checking from
       historical and compatibility reasons only */
    write_sha256(result + 2, sz - 32 - 2);
    return result;
}

static void client_action(GCT_INTPTR peer)
{
    struct client * me = get_client(peer);
    if (me == NULL) {
        gcl_err("Invalid client peer %" PRId64 GC_LOCFMT, peer, GC_LOC);
        return;
    }

    int packets_left = me->packets_left--;
    if (packets_left <= 0) {
        gc_client_stop(me->handle);
        return;
    }

    int sz = 128 + base_msg_sz + rand() % 1024;
    void * msg = make_message(sz);

    gc_client_add_message(me->handle, msg, sz, 42);
    free(msg);
}

void on_client_connect(GCT_INTPTR peer)
{
    struct client * me = get_client(peer);
    if (me == NULL) {
        gcl_err("Invalid client peer %" PRId64 GC_LOCFMT, peer, GC_LOC);
        return;
    }

    me->connected = 1;
    client_action(peer);
}

void on_client_disconnect(GCT_INTPTR peer)
{
    struct client * me = get_client(peer);
    if (me == NULL) {
        gcl_err("Invalid client peer %" PRId64 GC_LOCFMT, peer, GC_LOC);
        return;
    }

    me->stopped = 1;
    update_basta();
}

void on_client_connection_error(GCT_INTPTR peer, GCT_CSTR reason)
{
    struct client * me = get_client(peer);
    if (me == NULL) {
        gcl_err("Invalid client peer %" PRId64 GC_LOCFMT, peer, GC_LOC);
        return;
    }

    me->stopped = 1;
    update_basta();
}

void on_client_message(GCT_INTPTR peer, GCT_PTR msg, GCT_SIZE len)
{
    char * data = msg;
    int status = check_sha256(data + 2, len - 2 - 32);
    if (status != 0) {
        gcl_err("SHA-256 mismatch for receviced message.");
        return;
    }

    int client_id = peer;
    gcl_user("#%d %.*s", client_id, len - 32, data);
    client_action(peer);
}

void on_client_message_sent(GCT_INTPTR peer, GCT_INTPTR cookie, GCT_SIZE len, int status)
{
}

static void relax(int mode, const char * reason)
{
    basta = 0;
    gcl_notice("Relax %s for %d ms.\n", reason, relax_time);

    uint64_t time_ns = 1000 * 1000 * relax_time;
    uint64_t start_ns = gc_hrtime();

    for (;;) {
        if (basta) {
            break;
        }

        if (mode != GC_MODE_ACTIVE) {
            gc_clients_service();
        }

        uint64_t in_relax_ns = gc_hrtime() - start_ns;
        if (in_relax_ns > time_ns) {
            break;
        }

        if (sleep_time > 0) {
            /* Emulate hard job here */
            gc_sleep(sleep_time);
        }
    }
}

struct client_events events =
{
    .on_client_connect = on_client_connect,
    .on_client_connection_error = on_client_connection_error,
    .on_client_disconnect = on_client_disconnect,
    .on_client_message = on_client_message,
    .on_client_message_sent = on_client_message_sent,
};

void run(int mode)
{
    gcl_user("Run in mode %s\n", name_of_mode(mode));

    memset(clients, 0, sizeof(clients));

    for (int i=0; i<uri_count; ++i) {
        const struct gc_uri * uri = uries[i];
        gcl_notice("Run host: %s, app: %s, port: %d, proto: %s", uri->host, uri->app, uri->port, name_of_protocol(uri->proto));
        clients[i].handle = gc_client_start(i, uri->proto, uri->host, uri->port, uri->app, &events);
        clients[i].packets_left = uri_packets[i];
        gcl_notice("Run client #%d, handle %" PRId64 ", packages %d", i, clients[i].handle, clients[i].packets_left);
    }

    relax(mode, "after client creating");
}

static int parse_command_line(int argc, char * argv[])
{
    static const struct option long_options[] = {
        { "passive", no_argument, NULL, 'p' },
        { "active", no_argument, NULL, 'a' },
        { "hybrid", no_argument, NULL, 'h' },
        { "kbytes", required_argument, NULL, 'k' },
        { "sleep", required_argument, NULL, 's' },
        { "relax", required_argument, NULL, 'r' },
        { "verbose", no_argument, NULL, 'v' },
        { "help",  no_argument, &opt_help, 1},
        { NULL, 0, NULL, 0 }
    };

    for (;;) {
        int index = 0;
        const int c = getopt_long(argc, argv, "pahv", long_options, &index);
        if (c == -1) break;

        if (c != 0) {
            switch (c) {
                case 'p':
                    push_mode(GC_MODE_PASSIVE);
                    break;
                case 'a':
                    push_mode(GC_MODE_ACTIVE);
                    break;
                case 'h':
                    push_mode(GC_MODE_HYBRID);
                    break;
                case 'k':
                    base_msg_sz = 1024 * int_parse("base message size", optarg);
                    break;
                case 'r':
                    relax_time = int_parse("relax time", optarg);
                    break;
                case 's':
                    sleep_time = int_parse("sleep time", optarg);
                    break;
                case 'v':
                    ++opt_verbose;
                    break;
                 case '?':
                    fprintf(stderr, "Invalid option.\n");
                    return -1;
                default:
                    fprintf(stderr, "getopt_long returns unexpected char \\x%02X.\n", c);
                    return -1;
            }
        }

        int bad_values = 0
            || base_msg_sz < 0
            || relax_time < 0
            || sleep_time < 0
            ;

        if (bad_values) {
            return -1;
        }
    }

    return optind;
}

static void usage(int code)
{
    printf("Usage: %s [options] [uri1 [xN1] [uri2 [xN2] ... ]]\n", app_name);
    printf("Options:\n");
    static const char * const fmt = "  %2s %10s   %s\n";
    for (int i=0; opt_modes[i].value >= 0; ++i) {
        const struct opt_mode * om = opt_modes + i;
        printf(fmt, om->short_flag, om->long_name, om->hint);
    }
    printf(fmt, "-v", "--verbose", "Verbose level of logging.");
    printf(fmt,   "",    "--help", "Print help message and exit.");
    printf(fmt, "-k",  "--kbytes", "Base message len in kb.");
    printf(fmt, "-s",   "--sleep", "Sleep time in ms between gc_clients_service().");
    printf(fmt, "-r",   "--relax", "Relax time in ms.");
    exit(code);
}

int main(int argc, char * argv[])
{
    app_name = argv[0];

    int first_arg = parse_command_line(argc, argv);
    if (first_arg < 0) {
        usage(1);
    }

    if (opt_help) {
        usage(0);
    }

    if (mode_count == 0) {
        fprintf(stderr, "Mode is not set.\n");
        usage(1);
    }

    int status = gc_client_helper_initialize();
    if (status != 0) {
        fprintf(stderr,
            "gc_client_helper_initialize failed with code %d: %s\n",
            status, strerror(status));
        exit(1);
    }

    for (int i=first_arg; i<argc; ++i) {
        struct gc_uri * uri = parse_uri(argv[i], GC_PROTOCOL_WSS, 7681);
        if (uri == NULL) {
            exit(1);
        }

        if (uri_count >= MAX_URIES) {
            exit(1);
        }
        uries[uri_count] = uri;
        uri_packets[uri_count] = 1;

        if (i + 1 < argc) {
            const char * try_count = argv[i+1];
            char * end;
            long value = strtol(try_count, &end, 10);
            if (*end == '\0') {
                ++i;
                uri_packets[uri_count] = value;
            }
        }

        ++uri_count;
    }

    for (int i=0; i<uri_count; ++i) {
        struct gc_uri * uri = uries[i];
        gcl_user("Host #%d: name: %s, port: %d, protocol: %s, app: %s, packets: %d",
            i, uri->host, uri->port, name_of_protocol(uri->proto), uri->app, uri_packets[i]);
    }


    for (int i=0; i<mode_count; ++i) {
        int mode = modes[i];
        int status = gc_clients_init(mode);
        if (status != 0) {
            gcl_err("Cannot initialize clients, errno is %d", status);
            ++qerrors;
            break;
        }

        run(modes[i]);
        gc_client_helper_finalize();
    }

    for (int i=0; i<uri_count; ++i) {
        destroy_uri(uries[i]);
    }

    return !!qerrors;
}
