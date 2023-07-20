# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
#

cmake_minimum_required(VERSION 3.9)
project(bfrl VERSION "1.0" LANGUAGES C)

include(GNUInstallDirs)
include(CheckIncludeFiles)

option(ENABLE_EXAMPLES "Build examples" OFF)

set(CMAKE_MODULE_PATH
    ${PROJECT_SOURCE_DIR}/cmake
)

set(SRC_SOURCE
    "src/readline.c"
)

configure_file(
    ${CMAKE_MODULE_PATH}/config.h.in
    ${PROJECT_BINARY_DIR}/generated/bfrl/config.h
)

FILE(GLOB_RECURSE SRC_HEADER "include/*.h")
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_BINARY_DIR}/generated)

set(BFRL_LIBRARY
    ${SRC_HEADER}
    ${SRC_SOURCE}
)

add_library(bfrl_static STATIC ${BFRL_LIBRARY})
add_library(bfrl_shared SHARED ${BFRL_LIBRARY})
add_library(bfrl ALIAS bfrl_shared)

target_link_libraries(bfrl_static bfdev)
target_link_libraries(bfrl_shared bfdev)

if(ENABLE_EXAMPLES)
    enable_testing()
    add_subdirectory(examples)
endif()

if(${CMAKE_PROJECT_NAME} STREQUAL "bfsh")
    install(DIRECTORY
        ${BFDEV_HEADER_PATH}/bfrl
        ${BFDEV_GENERATED_PATH}/bfrl
        DESTINATION
        "${CMAKE_INSTALL_INCLUDEDIR}"
    )

    install(TARGETS
        bfrl_static bfrl_shared
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()
