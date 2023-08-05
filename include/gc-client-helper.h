#ifndef GAME_CARRIER_CLIENT_HELPER_H_INCLUDED
#define GAME_CARRIER_CLIENT_HELPER_H_INCLUDED

#include <game-carrier/client.h>

#ifdef __cplusplus
extern "C" {
#endif // #ifdef __cplusplus

#include <gc-helper.h>

extern const char * app_name;
extern int opt_verbose;

struct gc_uri
{
    int proto;
    char * host;
    int port;
    char * app;
};

int gc_client_helper_initialize(void);
void gc_client_helper_finalize(void);

struct gc_uri * parse_uri(const char * uri, int def_proto, int def_port);
void destroy_uri(struct gc_uri * me);

#ifdef __cplusplus
}
#endif // #ifdef __cplusplus

#endif // #ifndef GAME_CARRIER_CLIENT_HELPER_H_INCLUDED
