#ifndef MAIA_TEST_REENT_H
#define MAIA_TEST_REENT_H

struct _reent {
    int _errno;
    unsigned initialized;
};

#define _REENT_INIT_PTR(ptr) \
    do { (ptr)->_errno = 0; (ptr)->initialized = 1U; } while (0)

void _reclaim_reent(struct _reent *reent);

#endif
