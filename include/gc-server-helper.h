#ifndef GAME_CARRIER_SERVER_HELPER_H_INCLUDED
#define GAME_CARRIER_SERVER_HELPER_H_INCLUDED

#include <game-carrier/server.h>

#ifdef __cplusplus
extern "C" {
#endif // #ifdef __cplusplus

#include <gc-helper.h>

extern struct core_api gc_api;

int gc_server_helper_initialize(int id, struct core_api * api);
void gc_server_helper_finalize(void);

static inline void gc_conn_add_msg(GCT_INTPTR handle, const void * data, int len, int cookie)
{
    gc_api.connectionAddMessage(handle, (GCT_PTR)data, (GCT_SIZE)len, (GCT_INTPTR)cookie);
}

static inline void * gc_smart_lock_create(void)
{
    return (void*) gc_api.smartLockCreate();
}

static inline void gc_smart_lock_destroy(void * lock)
{
    gc_api.smartLockDestroy((GCT_INTPTR)lock);
}

static inline int gc_smart_lock(void * lock)
{
    return gc_api.smartLock((GCT_INTPTR)lock);
}

static inline void gc_smart_unlock(void * lock)
{
    gc_api.smartUnlock((GCT_INTPTR)lock);
}

static inline void gc_log_message(int level, const char * msg)
{
    gc_api.logMessage(level, (void*)msg);
}

void gc_global_lock(void);
void gc_global_unlock(void);

#ifdef __cplusplus
}
#endif // #ifdef __cplusplus

#endif // #ifndef GAME_CARRIER_SERVER_HELPER_H_INCLUDED
