cmake_minimum_required(VERSION 3.17)

project(
    GameCarrierExamples
    VERSION 0.1
    DESCRIPTION "Examples for Game Carrier Framework"
    LANGUAGES C CXX)

include(cmake/game-carrier.cmake)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)



#
# Debug/Release
#
if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
  # set(CMAKE_BUILD_TYPE RelWithDebInfo)
  # set(CMAKE_BUILD_TYPE Debug)
endif()
message(STATUS "====> Build type: ${CMAKE_BUILD_TYPE}")



#
# Configure warnnings
#
if(WIN32)
    if(${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
        add_compile_options(/fsanitize=address)
    endif()
    # Disable "possible loss of data" warnings.
    add_compile_options(/wd4267 /wd4245 /wd4244 /wd4996 /Zi)
else()
    add_compile_options(-Wall)
    if("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
        add_compile_options(-fsanitize=address)
        if(APPLE OR "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
            list(APPEND CMAKE_EXE_LINKER_FLAGS "-fsanitize=address")
            list(APPEND CMAKE_MODULE_LINKER_FLAGS "-fsanitize=address")
            list(APPEND CMAKE_SHARED_LINKER_FLAGS "-fsanitize=address")
        endif()
    endif()
endif()

# Setup Visual Studio
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)
endif()


if(WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(DEBUG_SUFFIX d)
    endif()
    set(WIN_RT libcmt${DEBUG_SUFFIX} libcpmt${DEBUG_SUFFIX} libucrt${DEBUG_SUFFIX} libvcruntime${DEBUG_SUFFIX})
endif()


find_game_carrier()
if("${GAME_CARRIER_FOUND}" STREQUAL "NO")
    message(FATAL_ERROR "Cannot find Game Carrier Framework, please download and install it from https://gamecarrier.com")
endif()

list(APPEND gc_include ${GAME_CARRIER_INCLUDE_DIR})
set(gc_client_ldflags ${GAME_CARRIER_CLIENT_LDFLAGS})
message(STATUS "====> GC Client static lib: ${GAME_CARRIER_CLIENT_LIB}")

list(APPEND gc_include "${PROJECT_SOURCE_DIR}/include")

if (WIN32)
    set(OS_LIBS ${WIN_RT} ws2_32 userenv iphlpapi crypt32)
else()
    set(OS_LIBS "")
endif()

set(gc_server_libs gc_server_helper_c)
set(gc_client_libs ${OS_LIBS} ${GAME_CARRIER_CLIENT_LIB} gc_client_helper_c)

if (WIN32)
    set(gc_app_dir "${CMAKE_BINARY_DIR}/bin/apps/${CMAKE_BUILD_TYPE}")
else()
    set(gc_app_dir "${CMAKE_BINARY_DIR}/bin/apps")
endif()

message(STATUS "====> Cient libs: ${gc_client_libs}")
message(STATUS "====> Server libs: ${gc_server_libs}")
message(STATUS "====> Include dirs: ${gc_include}")
message(STATUS "====> Linker flags: ${gc_client_ldflags}")

add_subdirectory(lib)
add_subdirectory(c)
