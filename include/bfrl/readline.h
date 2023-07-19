/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#ifndef _BFRL_READLINE_H_
#define _BFRL_READLINE_H_

#include <bfrl/config.h>
#include <bfdev/errno.h>
#include <bfdev/allocator.h>
#include <bfdev/list.h>

#ifndef BFRL_BUFFER_DEF
# define BFRL_BUFFER_DEF 64
#endif

#ifndef BFRL_WORKSPACE_DEF
# define BFRL_WORKSPACE_DEF  64
#endif

#ifndef BFRL_CLIPBRD_DEF
# define BFRL_CLIPBRD_DEF 64
#endif

typedef unsigned int (*bfrl_read_t)(char *str, unsigned int len, void *data);
typedef void (*bfrl_write_t)(const char *str, unsigned int len, void *data);

enum bfrl_esc {
    BFRL_ESC_NORM = 0,
    BFRL_ESC_ESC,
    BFRL_ESC_CSI,
    BFRL_ESC_SS3,
};

struct bfrl_history {
    struct bfdev_list_head list;
    unsigned int len;
    char cmd[0];
};

struct bfrl_state {
    const struct bfdev_alloc *alloc;
    bfrl_read_t read;
    bfrl_write_t write;
    void *data;

    const char *prompt;
    unsigned int plen;

    char *buff;
    unsigned int len;
    unsigned int pos;
    unsigned int bsize;
    bool keylock;
    char esc_param;
    enum bfrl_esc esc_state;

    const char *workspace;
    unsigned int worklen;
    unsigned int worksize;

    const char *clipbrd;
    unsigned int cliplen;
    unsigned int clippos;
    unsigned int clipsize;
    bool clipview;

    struct bfdev_list_head history;
    struct bfrl_history *curr;
};

extern char *bfrl_readline(struct bfrl_state *state, const char *dprompt, const char *cprompt);
extern struct bfrl_state *bfrl_alloc(const struct bfdev_alloc *alloc, bfrl_read_t read, bfrl_write_t write, void *data);
extern void bfrl_free(struct bfrl_state *state);

#endif /* _BFRL_READLINE_H_ */
