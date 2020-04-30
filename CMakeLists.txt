# -*- mode:cmake -*-
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
set(CMAKE_POLICY_DEFAULT_CMP0054 NEW)

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

set(CMAKE_C_STANDARD 99)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

set(TARGET j1939)
project(${TARGET} VERSION 0.0.1)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(DefineCMakeDefaults)
include(DefineCompilerFlags)
include(DefinePlatformDefaults)
include(CompilerChecks.cmake)

set(PGN_POOL_SIZE 16 CACHE STRING "PGN Pool size")


# config.h checks
include(ConfigureChecks.cmake)
configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set(J1939_DIR ${PROJECT_SOURCE_DIR}/src)
set(J1939_INC_DIR ${PROJECT_SOURCE_DIR}/include)

set(J1939_EXAMPLE_DIR ${PROJECT_SOURCE_DIR}/example)

set(J1939_SRC
    ${J1939_DIR}/j1939.c
    ${J1939_DIR}/j1939_ecu.c
    ${J1939_DIR}/hasht.c
    ${J1939_DIR}/pgn_pool.c
    ${J1939_DIR}/time.c
)

include_directories(
    ${J1939_DIR}
    ${J1939_INC_DIR}
)

add_library(${TARGET} STATIC ${J1939_SRC})

add_definitions(-DHAVE_CONFIG_H)

target_compile_options(${TARGET}
                       PRIVATE
                       ${DEFAULT_C_COMPILE_FLAGS})
set_property(TARGET
             ${TARGET}
             PROPERTY
             LINK_FLAGS
             "${DEFAULT_LINK_FLAGS}")

if(LIBJ1939_BUILD_EXAMPLE AND UNIX)
    add_definitions(-DTP_TASK_YIELD=1)
    add_definitions(-D_GNU_SOURCE)

    set(EXAMPLE_COMMON ${J1939_EXAMPLE_DIR}/linux_socketcan.c)

    add_executable(j1939_socketcan
        ${J1939_EXAMPLE_DIR}/j1939_socketcan.c
        ${EXAMPLE_COMMON}
    )
    set_property(TARGET j1939_socketcan PROPERTY LINK_FLAGS "${DEFAULT_LINK_FLAGS}")
    target_link_libraries(j1939_socketcan ${TARGET} rt pthread)
    target_compile_options(j1939_socketcan PRIVATE ${DEFAULT_C_COMPILE_FLAGS})

    add_executable(j1939_tp_client
        ${J1939_EXAMPLE_DIR}/j1939_tp_client.c
        ${EXAMPLE_COMMON}
    )
    set_property(TARGET j1939_tp_client PROPERTY LINK_FLAGS "${DEFAULT_LINK_FLAGS}")
    target_link_libraries(j1939_tp_client ${TARGET} rt pthread)
    target_compile_options(j1939_tp_client PRIVATE ${DEFAULT_C_COMPILE_FLAGS})

    add_executable(j1939_tp_server
        ${J1939_EXAMPLE_DIR}/j1939_tp_server.c
        ${EXAMPLE_COMMON}
    )
    set_property(TARGET j1939_tp_server PROPERTY LINK_FLAGS "${DEFAULT_LINK_FLAGS}")
    target_link_libraries(j1939_tp_server ${TARGET} rt pthread)
    target_compile_options(j1939_tp_server PRIVATE ${DEFAULT_C_COMPILE_FLAGS})
endif()

#
# Doxygen
#
if (GENERATE_DOC)
    find_package(Doxygen
                 REQUIRED dot
                 OPTIONAL_COMPONENTS mscgen dia)

    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_MAN NO)
    set(DOXYGEN_GENERATE_LATEX NO)
    set(DOXYGEN_PROJECT_NAME "Lib J1939")

    doxygen_add_docs(
         doxygen
        ${PROJECT_SOURCE_DIR}
        COMMENT "Generate html pages"
    )
endif()

