/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#ifdef _BFRL_READLINE_H_

static int
clipbrd_save(struct bfrl_state *rstate, unsigned int *clippos)
{
    const struct bfdev_alloc *alloc = rstate->alloc;
    unsigned int start, length;

    if (rstate->clipview) {
        start = bfdev_min(rstate->clippos, rstate->pos);
        length = bfdev_max(rstate->clippos, rstate->pos) - start;
    } else {
        start = 0;
        length = rstate->len;
    }

    if (clippos)
        *clippos = start;

    if (length > rstate->clipsize) {
        unsigned int nbsize = rstate->clipsize;
        void *nblk;

        while (length > nbsize)
            nbsize *= 2;

        nblk = bfdev_malloc(alloc, nbsize);
        if (!nblk)
            return -BFDEV_ENOMEM;

        bfdev_free(alloc, rstate->clipbrd);
        rstate->clipbrd = nblk;
        rstate->clipsize = nbsize;
    }

    rstate->clipview = false;
    rstate->cliplen = length;
    memcpy((char *)rstate->clipbrd, rstate->buff + start, length);

    return -BFDEV_ENOERR;
}

static void
clipbrd_restory(struct bfrl_state *rstate)
{
    if (rstate->cliplen)
        readline_insert(rstate, rstate->clipbrd, rstate->cliplen);
}

#endif /* _BFRL_READLINE_H_ */
