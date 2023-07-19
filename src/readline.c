/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2021 John Sanpe <sanpeqf@gmail.com>
 */

#include <bfrl/readline.h>
#include <bfdev/ctype.h>
#include <bfdev/string.h>
#include <bfdev/ascii.h>
#include <bfdev/errptr.h>
#include <bfdev/minmax.h>
#include <export.h>

#define READLINE_ALT_OFFSET 0x80

static inline unsigned int
readline_read(struct bfrl_state *rstate, char *str, unsigned int len)
{
    return rstate->read(str, len, rstate->data);
}

static inline void
readline_write(struct bfrl_state *rstate, const char *str, unsigned int len)
{
    rstate->write(str, len, rstate->data);
}

static void
readline_reset(struct bfrl_state *rstate)
{
    rstate->pos = 0;
    rstate->len = 0;
    rstate->curr = NULL;
    rstate->esc_state = BFRL_ESC_NORM;
}

static void
readline_fill(struct bfrl_state *rstate, unsigned int len)
{
    while (len--)
        readline_write(rstate, " ", 1);
}

#include "cursor.c"
#include "history.c"
#include "clipbrd.c"

static bool
readline_handle(struct bfrl_state *state, char code)
{
    struct bfrl_history *history;
    unsigned int tmp;
    bool complete = false;

    if (state->keylock && code != BFDEV_ASCII_DC3)
        return false;

    switch ((unsigned char)code) {
        case BFDEV_ASCII_SOH: /* ^A : Cursor Home */
            cursor_home(state);
            break;

        case BFDEV_ASCII_STX: /* ^B : Cursor Left */
            cursor_left(state);
            break;

        case BFDEV_ASCII_ETX: /* ^C : Break Readline */
            state->len = state->pos = 0;
            state->curr = NULL;
            state->clippos = 0;
            return true;

        case BFDEV_ASCII_EOT: /* ^D : Delete */
            if (state->pos < state->len)
                readline_delete(state, 1);
            workspace_save(state);
            state->curr = NULL;
            break;

        case BFDEV_ASCII_ENQ: /* ^E : Cursor End */
            cursor_end(state);
            break;

        case BFDEV_ASCII_ACK: /* ^F : Cursor Right */
            cursor_right(state);
            break;

        case BFDEV_ASCII_BS: /* ^H : Backspace */
            if (state->pos)
                readline_backspace(state, 1);
            workspace_save(state);
            state->curr = NULL;
            break;

        case BFDEV_ASCII_LF: /* ^J : Line Feed */
            goto linefeed;

        case BFDEV_ASCII_VT: /* ^K : Clear After */
            if (state->len)
                readline_delete(state, state->len - state->pos);
            workspace_save(state);
            state->curr = NULL;
            break;

        case BFDEV_ASCII_FF: /* ^L : Form Feed */
            readline_clear(state);
            state->clippos = 0;
            break;

        case BFDEV_ASCII_CR: /* ^M : Carriage Return */
        linefeed:
            state->clippos = 0;
            return true;

        case BFDEV_ASCII_SO: /* ^N : History Complete Next */
            complete = true;
            goto history_next;

        case BFDEV_ASCII_SI: /* ^O : Clipboard Select */
            state->clipview = true;
            state->clippos = state->pos;
            break;

        case BFDEV_ASCII_DLE: /* ^P : History Complete Prev */
            complete = true;
            goto history_prev;

        case BFDEV_ASCII_DC1: /* ^Q : History Clear */
            history_clear(state);
            break;

        case BFDEV_ASCII_DC2: /* ^R : Clipboard Clear */
            state->cliplen = 0;
            break;

        case BFDEV_ASCII_DC3: /* ^S : Readline Lock */
            state->keylock ^= true;
            break;

        case BFDEV_ASCII_DC4: /* ^T : Repeat Execution */
            history = history_prev(state, state->buff, state->len, complete);
            if (history) {
                cursor_home(state);
                readline_delete(state, state->len);
                readline_insert(state, history->cmd, history->len);
                state->clippos = 0;
            }
            goto linefeed;

        case BFDEV_ASCII_NAK: /* ^U : Clear Before */
            if (state->pos)
                readline_backspace(state, state->pos);
            workspace_save(state);
            state->curr = NULL;
            break;

        case BFDEV_ASCII_SYN: /* ^V : History Next */
        history_next:
            history = history_next(state, complete);
            cursor_home(state);
            readline_delete(state, state->len);
            if (history)
                readline_insert(state, history->cmd, history->len);
            else
                workspace_restory(state);
            state->clippos = 0;
            break;

        case BFDEV_ASCII_ETB: /* ^W : History Prev */
        history_prev:
            history = history_prev(state, state->buff, state->len, complete);
            if (history) {
                cursor_home(state);
                readline_delete(state, state->len);
                readline_insert(state, history->cmd, history->len);
                state->clippos = 0;
            }
            break;

        case BFDEV_ASCII_CAN: /* ^X : Clipboard Cut */
            clipbrd_save(state, &tmp);
            cursor_offset(state, tmp);
            readline_delete(state, state->cliplen);
            workspace_save(state);
            state->curr = NULL;
            break;

        case BFDEV_ASCII_EM: /* ^Y : Clipboard Yank */
            clipbrd_save(state, NULL);
            break;

        case BFDEV_ASCII_SUB: /* ^Z : Clipboard Paste */
            clipbrd_restory(state);
            workspace_save(state);
            state->curr = NULL;
            break;

        case READLINE_ALT_OFFSET + 'b': /* ^[b : Backspace Word */
            for (tmp = state->pos; tmp-- > 1;) {
                if (isalnum(state->buff[tmp]) &&
                    !isalnum(state->buff[tmp - 1])) {
                    readline_backspace(state, state->pos - tmp);
                    break;
                }
            }
            if (!tmp)
                readline_backspace(state, state->pos);
            break;

        case READLINE_ALT_OFFSET + 'd': /* ^[d : Delete Word */
            for (tmp = state->pos; ++tmp < state->len;) {
                if (isalnum(state->buff[tmp - 1]) &&
                    !isalnum(state->buff[tmp])) {
                    readline_delete(state, tmp - state->pos);
                    break;
                }
            }
            if (tmp == state->len)
                readline_delete(state, state->len - state->pos);
            break;

        case READLINE_ALT_OFFSET + 'l': /* ^[l : Cursor Left Word */
            for (tmp = state->pos; tmp-- > 1;) {
                if (isalnum(state->buff[tmp]) &&
                    !isalnum(state->buff[tmp - 1])) {
                    cursor_offset(state, tmp);
                    break;
                }
            }
            if (!tmp)
                cursor_home(state);
            break;

        case READLINE_ALT_OFFSET + 'r': /* ^[r : Cursor Right Word */
            for (tmp = state->pos; ++tmp < state->len;) {
                if (!isalnum(state->buff[tmp - 1]) &&
                    isalnum(state->buff[tmp])) {
                    cursor_offset(state, tmp);
                    break;
                }
            }
            if (tmp == state->len)
                cursor_end(state);
            break;

        default:
            if (isprint(code)) {
                readline_insert(state, &code, 1);
                workspace_save(state);
                state->curr = NULL;
            }
    }

    return false;
}

static bool
readline_getcode(struct bfrl_state *state, char *code)
{
    if (!readline_read(state, code, 1))
        return false;

    switch (state->esc_state) {
        case BFRL_ESC_NORM:
            switch (*code) {
                case BFDEV_ASCII_ESC: /* Escape */
                    state->esc_state = BFRL_ESC_ESC;
                    break;

                case BFDEV_ASCII_DEL: /* Backspace */
                    *code = BFDEV_ASCII_BS;
                    return true;

                default:
                    return true;
            }
            break;

        case BFRL_ESC_ESC:
            switch (*code) {
                case '[':
                    state->esc_state = BFRL_ESC_CSI;
                    state->esc_param = 0;
                    break;

                case 'O':
                    state->esc_state = BFRL_ESC_SS3;
                    state->esc_param = 0;
                    break;

                case 'a' ... 'z': /* Alt Alpha */
                    state->esc_state = BFRL_ESC_NORM;
                    *code += READLINE_ALT_OFFSET;
                    return true;

                case BFDEV_ASCII_DEL: /* Alt Backspace */
                    *code = READLINE_ALT_OFFSET + 'b';
                    return true;

                default:
                    state->esc_state = BFRL_ESC_NORM;
                    return true;
            }
            break;

        case BFRL_ESC_CSI:
            if (*code >= '0' && *code <= '9') {
                state->esc_param = state->esc_param * 10 + (*code - '0');
                return false;
            }

            if (*code == ';')
                break;

            state->esc_state = BFRL_ESC_NORM;
            switch (*code) {
                case 'A': /* Cursor Up */
                    if (!state->esc_param)
                        *code = BFDEV_ASCII_DLE;
                    else if (state->esc_param == 15) /* Ctrl */
                        *code = BFDEV_ASCII_ETB;
                    return true;

                case 'B': /* Cursor Down */
                    if (!state->esc_param)
                        *code = BFDEV_ASCII_SO;
                    else if (state->esc_param == 15) /* Ctrl */
                        *code = BFDEV_ASCII_SYN;
                    return true;

                case 'C': /* Cursor Right */
                    if (!state->esc_param)
                        *code = BFDEV_ASCII_ACK;
                    else if (state->esc_param == 15) /* Ctrl */
                        *code = READLINE_ALT_OFFSET + 'r';
                    return true;

                case 'D': /* Cursor Left */
                    if (!state->esc_param)
                        *code = BFDEV_ASCII_STX;
                    else if (state->esc_param == 15) /* Ctrl */
                        *code = READLINE_ALT_OFFSET + 'l';
                    return true;

                case 'E': /* Cursor Middle */
                    *code = BFDEV_ASCII_DC4;
                    return true;

                case 'F': /* Control End */
                    if (!state->esc_param)
                        *code = BFDEV_ASCII_ENQ;
                    else if (state->esc_param == 15) /* Ctrl */
                        *code = BFDEV_ASCII_VT;
                    return true;

                case 'H': /* Control Home */
                    if (!state->esc_param)
                        *code = BFDEV_ASCII_SOH;
                    else if (state->esc_param == 15) /* Ctrl */
                        *code = BFDEV_ASCII_NAK;
                    return true;

                case '~':
                    switch (state->esc_param) {
                        case 2: /* Control Insert */
                            *code = BFDEV_ASCII_SUB;
                            return true;

                        case 3: /* Control Delete */
                            *code = BFDEV_ASCII_EOT;
                            return true;

                        case 5: /* Control Page Up */
                            *code = BFDEV_ASCII_ETB;
                            return true;

                        case 6: /* Control Page Down */
                            *code = BFDEV_ASCII_SYN;
                            return true;

                        case 23: /* Alt Control Insert */
                            *code = BFDEV_ASCII_CAN;
                            return true;

                        case 25: /* Ctrl Control Insert */
                            *code = BFDEV_ASCII_EM;
                            return true;

                        case 27: /* Ctrl Alt Control Insert */
                            *code = BFDEV_ASCII_SI;
                            return true;

                        case 35: /* Ctrl Control Delete */
                            *code = READLINE_ALT_OFFSET + 'd';
                            return true;

                        default:
                            break;
                    }
                    break;

                default:
                    break;
            }
            break;

        case BFRL_ESC_SS3:
            state->esc_state = BFRL_ESC_NORM;
            switch (*code) {
                case 'F': /* ^E */
                    *code = BFDEV_ASCII_ENQ;
                    return true;

                case 'H': /* ^A */
                    *code = BFDEV_ASCII_SOH;
                    return true;

                default:
                    break;
            }
            break;

        default:
            state->esc_state = BFRL_ESC_NORM;
            break;
    }

    return false;
}

static inline void
readline_setup(struct bfrl_state *state, const char *prompt)
{
    readline_reset(state);
    if (!prompt)
        state->plen = 0;
    else {
        state->prompt = prompt;
        state->plen = strlen(prompt);
        readline_write(state, state->prompt, state->plen);
    }
}

char *
bfrl_readline(struct bfrl_state *state, const char *dprompt, const char *cprompt)
{
    unsigned int offset = 0;
    char code;

    readline_setup(state, dprompt);

    for (;;) {
        if (!readline_getcode(state, &code))
            ;

        else if (readline_handle(state, code)) {
            readline_write(state, "\n", 1);
            if (!state->len || state->buff[state->len - 1] != '\\')
                break;

            offset += state->len - 1;
            state->buff += state->len - 1;
            state->bsize -= state->len - 1;

            readline_setup(state, cprompt);
        }
    }

    state->buff[state->len] = '\0';
    state->buff -= offset;
    state->bsize += offset;
    state->len += offset;

    if (state->len)
        history_add(state, state->buff, state->len);
    else
        return NULL;

    return state->buff;
}

struct bfrl_state *
bfrl_alloc(const struct bfdev_alloc *alloc, bfrl_read_t read,
           bfrl_write_t write, void *data)
{
    struct bfrl_state *state;

    state = bfdev_zalloc(alloc, sizeof(*state));
    if (!state)
        return NULL;

    state->read = read;
    state->write = write;
    state->data = data;

    state->bsize = BFRL_BUFFER_DEF;
    state->buff = bfdev_malloc(alloc, state->bsize);
    if (!state->buff)
        return NULL;

    state->worksize = BFRL_WORKSPACE_DEF;
    state->workspace = bfdev_malloc(alloc, state->worksize);
    if (!state->workspace)
        return NULL;

    state->clipsize = BFRL_CLIPBRD_DEF;
    state->clipbrd = bfdev_malloc(alloc, state->clipsize);
    if (!state->clipbrd)
        return NULL;

    bfdev_list_head_init(&state->history);
    return state;
}

void
bfrl_free(struct bfrl_state *state)
{
    history_clear(state);
    bfdev_free(state->alloc, state->workspace);
    bfdev_free(state->alloc, state->clipbrd);
    bfdev_free(state->alloc, state->buff);
    bfdev_free(state->alloc, state);
}
