/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2023 John Sanpe <sanpeqf@gmail.com>
 */

#ifdef _BFRL_READLINE_

static int
workspace_save(struct bfrl_state *rstate)
{
    const struct bfdev_alloc *alloc = rstate->alloc;

    if (rstate->len > rstate->worksize) {
        unsigned int nbsize = rstate->worksize;
        void *nblk;

        while (rstate->len > nbsize)
            nbsize *= 2;

        nblk = bfdev_malloc(alloc, nbsize);
        if (!nblk)
            return -BFDEV_ENOMEM;

        bfdev_free(alloc, rstate->workspace);
        rstate->workspace = nblk;
        rstate->worksize = nbsize;
    }

    rstate->worklen = rstate->len;
    memcpy((char *)rstate->workspace, rstate->buff, rstate->len);

    return -BFDEV_ENOERR;
}

static void
workspace_restory(struct bfrl_state *rstate)
{
    if (rstate->worklen)
        readline_insert(rstate, rstate->workspace, rstate->worklen);
}

static struct bfrl_history *
history_prev(struct bfrl_state *rstate, const char *cmd, unsigned int len, bool complete)
{
    struct bfrl_history *prev;
    int retval;

    if (!rstate->curr) {
        retval = workspace_save(rstate);
        if (retval)
            return BFDEV_ERR_PTR(retval);
    }

    if (complete && rstate->worklen) {
        if (rstate->curr)  {
            for (prev = bfdev_list_next_entry(rstate->curr, list);
                 !bfdev_list_entry_check_head(prev, &rstate->history, list);
                 prev = bfdev_list_next_entry(prev, list)) {
                if (!strncmp(rstate->workspace, prev->cmd, rstate->worklen))
                    goto finish;
            }
        } else {
            for (prev = bfdev_list_first_entry(&rstate->history, struct bfrl_history, list);
                 !bfdev_list_entry_check_head(prev, &rstate->history, list);
                 prev = bfdev_list_next_entry(prev, list)) {
                if (!strncmp(rstate->workspace, prev->cmd, rstate->worklen))
                    goto finish;
            }
        }
        return NULL;
    }

    else if (rstate->curr) {
        prev = bfdev_list_next_entry(rstate->curr, list);
        if (bfdev_list_entry_check_head(prev, &rstate->history, list))
            return NULL;
    } else {
        prev = bfdev_list_first_entry_or_null(&rstate->history,
                    struct bfrl_history, list);
    }

finish:
    rstate->curr = prev;
    return prev;
}

static struct bfrl_history *
history_next(struct bfrl_state *rstate, bool complete)
{
    struct bfrl_history *next;

    if (!rstate->curr)
        return NULL;

    if (complete && rstate->worklen) {
        for (next = bfdev_list_prev_entry(rstate->curr, list);
             !bfdev_list_entry_check_head(next, &rstate->history, list);
             next = bfdev_list_prev_entry(next, list)) {
            if (!strncmp(rstate->workspace, next->cmd, rstate->worklen))
                goto finish;
        }
        next = NULL;
        goto finish;
    }

    else {
        next = bfdev_list_prev_entry(rstate->curr, list);
        if (bfdev_list_entry_check_head(next, &rstate->history, list))
            next = NULL;
    }

finish:
    rstate->curr = next;
    return next;
}

static int
history_add(struct bfrl_state *rstate, const char *cmd, unsigned int len)
{
    const struct bfdev_alloc *alloc = rstate->alloc;
    struct bfrl_history *history;

    if (rstate->worklen)
        rstate->worklen = 0;

    history = bfdev_list_first_entry_or_null(&rstate->history, struct bfrl_history, list);
    if (history && history->len == len && !strncmp(history->cmd, cmd, len))
        return -BFDEV_ENOERR;

    history = bfdev_malloc(alloc, sizeof(*history) + len);
    if (!history)
        return -BFDEV_ENOMEM;

    history->len = len;
    bfdev_list_head_init(&history->list);

    memcpy(history->cmd, cmd, len);
    bfdev_list_add(&rstate->history, &history->list);

    return -BFDEV_ENOERR;
}

static void
history_clear(struct bfrl_state *rstate)
{
    struct bfrl_history *history, *next;

    bfdev_list_for_each_entry_safe(history, next, &rstate->history, list) {
        bfdev_list_del(&history->list);
        bfdev_free(rstate->alloc, history);
    }

    rstate->curr = NULL;
}

#endif /* _BFRL_READLINE_ */
