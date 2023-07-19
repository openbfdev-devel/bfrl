/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#ifdef _BFRL_READLINE_H_

static inline void
cursor_save(struct bfrl_state *rstate)
{
    readline_write(rstate, "\e[s", 3);
}

static inline void
cursor_restore(struct bfrl_state *rstate)
{
    readline_write(rstate, "\e[u", 3);
}

static bool
cursor_left(struct bfrl_state *rstate)
{
    if (rstate->pos) {
        readline_write(rstate, "\e[D", 3);
        --rstate->pos;
        return true;
    }

    return false;
}

static bool
cursor_right(struct bfrl_state *rstate)
{
    if (rstate->pos < rstate->len) {
        readline_write(rstate, "\e[C", 3);
        ++rstate->pos;
        return true;
    }

    return false;
}

static bool
cursor_offset(struct bfrl_state *rstate, unsigned int offset)
{
    bool retval = true;

    while (rstate->pos != offset && retval) {
        if (rstate->pos > offset)
            retval = cursor_left(rstate);
        else if (rstate->pos < offset)
            retval = cursor_right(rstate);
    }

    return retval;
}

static void
cursor_home(struct bfrl_state *rstate)
{
    cursor_offset(rstate, 0);
}

static void
cursor_end(struct bfrl_state *rstate)
{
    cursor_offset(rstate, rstate->len);
}

static int
readline_insert(struct bfrl_state *rstate, const char *str, unsigned int len)
{
    const struct bfdev_alloc *alloc = rstate->alloc;

    if (rstate->len + len >= rstate->bsize) {
        unsigned int nbsize = rstate->bsize;
        void *nblk;

        while (rstate->len + len >= nbsize)
            nbsize *= 2;

        nblk = bfdev_realloc(alloc, rstate->buff, nbsize);
        if (!nblk)
            return -BFDEV_ENOMEM;

        rstate->buff = nblk;
        rstate->bsize = nbsize;
    }

    memmove(rstate->buff + rstate->pos + len, rstate->buff + rstate->pos, rstate->len - rstate->pos + 1);
    memmove(rstate->buff + rstate->pos, str, len);
    rstate->pos += len;
    rstate->len += len;

    readline_write(rstate, &rstate->buff[rstate->pos - len], len);
    cursor_save(rstate);
    readline_write(rstate, &rstate->buff[rstate->pos], rstate->len - rstate->pos);
    cursor_restore(rstate);

    return -BFDEV_ENOERR;
}

static void
readline_delete(struct bfrl_state *rstate, unsigned int len)
{
    bfdev_min_adj(len, rstate->len - rstate->pos);

    memmove(rstate->buff + rstate->pos, rstate->buff + rstate->pos + len, rstate->len - rstate->pos + 1);
    rstate->len -= len;

    cursor_save(rstate);
    readline_write(rstate, &rstate->buff[rstate->pos], rstate->len - rstate->pos);
    readline_fill(rstate, len);
    cursor_restore(rstate);
}

static void
readline_backspace(struct bfrl_state *rstate, unsigned int len)
{
    unsigned int pos = rstate->pos;

    bfdev_min_adj(len, rstate->pos);
    cursor_offset(rstate, pos - len);
    readline_delete(rstate, len);
}

static void
readline_clear(struct bfrl_state *rstate)
{
    readline_reset(rstate);
    readline_write(rstate, "\e[2J\e[1;1H", 10);
    readline_write(rstate, rstate->prompt, rstate->plen);
}

#endif /* _BFRL_READLINE_H_ */
