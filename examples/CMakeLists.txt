# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
#

add_executable(console console.c)
target_link_libraries(console bfrl)
add_test(console console)

if(${CMAKE_PROJECT_NAME} STREQUAL "bfrl")
    install(FILES
        console.c
        DESTINATION
        ${CMAKE_INSTALL_DOCDIR}/examples
    )

    install(TARGETS
        console
        DESTINATION
        ${CMAKE_INSTALL_DOCDIR}/bin
    )
endif()
