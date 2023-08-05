#include <gc-server-helper.h>

#include <stdio.h>
#include <stdlib.h>

#define REPORT_MOD 1000

struct conn {
    GCT_INTPTR handle;
    int app_id;
};

static int qmessages = 0;

GC_ADAPTER_EVENT
GCT_INT adapter_initialize(
    GCT_INT id,
    struct core_api * api,
    GCT_CPTR load_path)
{
    int status = gc_server_helper_initialize(id, api);
    if (status != 0) {
        return status;
    }

    gcl_user("%s with id: %d, load_path: %s.", __func__, id, (const char *)load_path);
    return 0;
}

GC_ADAPTER_EVENT
void adapter_finalize(
    GCT_INT id)
{
    gcl_user("%s with id: %d.", __func__, id);
    gc_server_helper_finalize();
}

GC_ADAPTER_EVENT
GCT_INT application_initialize(
    GCT_INT id,
    GCT_CSTR name,
    GCT_CSTR type_name,
    GCT_CSTR file_name)
{
	return 0;
}

GC_ADAPTER_EVENT
void application_stop(
    GCT_INT i)
{
}

GC_ADAPTER_EVENT
void application_shutdown(
    GCT_INT i)
{
}

static struct conn * create_conn(GCT_INTPTR handle, int app_id)
{
    struct conn * me = calloc(1, sizeof(struct conn));
    if (me == NULL) {
        return NULL;
    }

    me->handle = handle;
    me->app_id = app_id;
    return me;
}

GC_ADAPTER_EVENT
GCT_INTPTR on_connect(
    GCT_INT app_id,
    GCT_INTPTR handle,
    GCT_INT pid,
    GCT_CSTR rip,
    GCT_INT rport,
    GCT_CSTR lip,
    GCT_INT lport,
    GCT_CSTR udata)
{
    struct conn * conn = create_conn(handle, (int)app_id);
    if (conn == NULL) {
        return GC_DROP_CONNECTION;
    }

    return (GCT_INTPTR)conn;
}

void destroy_conn(struct conn * me)
{
    free(me);
}

GC_ADAPTER_EVENT
void on_disconnect(
    GCT_INTPTR user)
{
    destroy_conn((void *)user);
}

GC_ADAPTER_EVENT
GCT_INT on_group_message(
    GCT_INTPTR cid,
    GCT_PTR data,
    GCT_SIZE len)
{
    return 0;
}

GC_ADAPTER_EVENT
void on_message_sent(
    GCT_INTPTR peer,
    GCT_INTPTR cookie,
    GCT_SIZE len,
    GCT_INT status)
{
}

static void on_conn_message(struct conn * me, const uint8_t * data, int len)
{
    static const int min_len = 32 + 2;
    if (len < min_len) {
        gcl_err("Received message is too short, len = %d" GC_LOCFMT, len, GC_LOC);
        return;
    }

    int status = check_sha256(data + 2, len - 2 - 32);
    if (status != 0) {
        gcl_err("SHA-256 mismatch for receviced message.");
        return;
    }


    int text_len;
    char * text = gc_lprintf(&text_len, "R: app-%d receiced %d bytes.", me->app_id, len);
    if (text == NULL) {
        gcl_err("OOM during printing text" GC_LOCFMT, GC_LOC);
        return;
    }
    ++text_len; // include trailing zero to message

    int msg_len = text_len + 32;
    char * msg = realloc(text, msg_len);
    if (msg == NULL) {
        free(text);
        gcl_err("OOM during reallocation %d bytes" GC_LOCFMT, msg_len, GC_LOC);
        return;
    }

    write_sha256(msg + 2, text_len - 2);
    gc_conn_add_msg(me->handle, msg, msg_len, 0);
    free(msg);

    gc_global_lock();
    int total = ++qmessages;
    gc_global_unlock();
    if (total % REPORT_MOD == 0) {
        gcl_notice("Received %d messages.", total);
    }
}

GC_ADAPTER_EVENT
void on_message(
    GCT_INTPTR user,
    GCT_PTR data,
    GCT_SIZE len)
{
    on_conn_message((void *)user, (const uint8_t *)data, len);
}

GC_ADAPTER_EVENT
void on_client_connect(
    GCT_INTPTR peer)
{
}

GC_ADAPTER_EVENT
void on_client_disconnect(
    GCT_INTPTR peer)
{
}

GC_ADAPTER_EVENT
void on_client_connection_error(
    GCT_INTPTR peer,
    GCT_CSTR reason)
{
}

GC_ADAPTER_EVENT
void on_client_message(
    GCT_INTPTR peer,
    GCT_PTR data,
    GCT_SIZE len)
{
}

GC_ADAPTER_EVENT
void on_client_message_sent(
    GCT_INTPTR conn,
    GCT_INTPTR cookie,
    GCT_SIZE len,
    GCT_INT status)
{
}
