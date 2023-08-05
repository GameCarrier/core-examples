#include <gc-server-helper.h>

#include <errno.h>

extern void init_hrtime(void);
extern void done_hrtime(void);

struct core_api gc_api;
static void * helper_global_lock;
static int adapter_id = -1;

void gc_global_lock(void)
{
    gc_smart_lock(helper_global_lock);
}

void gc_global_unlock(void)
{
    gc_smart_unlock(helper_global_lock);
}

int gc_server_helper_initialize(int id, struct core_api * api)
{
    if (adapter_id != -1) {
        gcl_err("Adapter is initalized twice!");
        return EINVAL;
    }

    gc_api = *api;
    helper_global_lock = gc_smart_lock_create();

    if (helper_global_lock == NULL) {
        gcl_err("Cannot initialize helper global smart lock" GC_LOCFMT, GC_LOC);
        return ENOMEM;
    }

    adapter_id = id;
    init_hrtime();
    return 0;
}

void gc_server_helper_finalize(void)
{
    if (adapter_id < 0) {
        gcl_err("Try to finalize uninitialized adapter" GC_LOCFMT, GC_LOC);
        return;
    }

    gc_smart_lock_destroy(helper_global_lock);
    adapter_id = -1;

    done_hrtime();
}
