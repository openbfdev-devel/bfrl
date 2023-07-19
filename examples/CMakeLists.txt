# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
#

add_executable(console console.c)
target_link_libraries(console bfrl)
add_test(console console)
