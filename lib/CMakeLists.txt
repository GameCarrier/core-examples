set(gc_helper_common_src sha256.c print.c)
if(WIN32)
    list(APPEND gc_helper_common_src posix/getopt.c)
    list(APPEND gc_helper_common_src platform/win/hrtime.c)
else()
    list(APPEND gc_helper_common_src platform/unix/hrtime.c)
endif()

add_library(gc_server_helper_c STATIC server.c server/logging.c ${gc_helper_common_src})
target_include_directories(gc_server_helper_c PRIVATE ${gc_include})

add_library(gc_client_helper_c STATIC client.c client/logging.c ${gc_helper_common_src})
target_include_directories(gc_client_helper_c PRIVATE ${gc_include})
