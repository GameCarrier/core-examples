add_library(minimal_app SHARED app.c)
target_link_libraries(minimal_app PRIVATE ${gc_server_libs})
target_include_directories(minimal_app PRIVATE ${gc_include})
configure_file(config.json.in "${CMAKE_BINARY_DIR}/configs/c-minimal-app.json")
set_target_properties(minimal_app PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/apps"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/apps"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/apps")
