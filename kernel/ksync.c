#include <maia/kernel.h>

static void copy_bytes(uint8_t *dst, const uint8_t *src, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        dst[i] = src[i];
    }
}

maia_status_t maia_mutex_init(maia_mutex_t *mtx) {
    if (mtx == NULL) {
        return MAIA_ERR_INVALID;
    }
    mtx->locked        = 0U;
    mtx->owner_task_id = -1;
    mtx->lock_count    = 0U;
    return MAIA_OK;
}

maia_status_t maia_mutex_trylock(maia_mutex_t *mtx, int32_t task_id) {
    if (mtx == NULL || task_id < 0) {
        return MAIA_ERR_INVALID;
    }
    if (mtx->locked == 0U) {
        mtx->locked        = 1U;
        mtx->owner_task_id = task_id;
        mtx->lock_count    = 1U;
        return MAIA_OK;
    }
    if (mtx->owner_task_id == task_id) {
        mtx->lock_count += 1U;
        return MAIA_OK;
    }
    return MAIA_ERR_BUSY;
}

maia_status_t maia_mutex_unlock(maia_mutex_t *mtx, int32_t task_id) {
    if (mtx == NULL || task_id < 0) {
        return MAIA_ERR_INVALID;
    }
    if (mtx->locked == 0U || mtx->owner_task_id != task_id) {
        return MAIA_ERR_PERM;
    }
    if (mtx->lock_count > 1U) {
        mtx->lock_count -= 1U;
        return MAIA_OK;
    }
    mtx->lock_count    = 0U;
    mtx->owner_task_id = -1;
    mtx->locked        = 0U;
    return MAIA_OK;
}

maia_status_t maia_sem_init(maia_sem_t *sem, uint32_t initial_count, uint32_t max_count) {
    if (sem == NULL || max_count == 0U || initial_count > max_count) {
        return MAIA_ERR_INVALID;
    }
    sem->count     = initial_count;
    sem->max_count = max_count;
    return MAIA_OK;
}

maia_status_t maia_sem_trywait(maia_sem_t *sem) {
    if (sem == NULL) {
        return MAIA_ERR_INVALID;
    }
    if (sem->count == 0U) {
        return MAIA_ERR_EMPTY;
    }
    sem->count -= 1U;
    return MAIA_OK;
}

maia_status_t maia_sem_post(maia_sem_t *sem) {
    if (sem == NULL) {
        return MAIA_ERR_INVALID;
    }
    if (sem->count >= sem->max_count) {
        return MAIA_ERR_FULL;
    }
    sem->count += 1U;
    return MAIA_OK;
}

uint32_t maia_sem_getvalue(const maia_sem_t *sem) {
    if (sem == NULL) {
        return 0U;
    }
    return sem->count;
}

maia_status_t maia_queue_init(maia_queue_t *q, void *storage, size_t item_size, size_t capacity) {
    if (q == NULL || storage == NULL || item_size == 0U || capacity == 0U) {
        return MAIA_ERR_INVALID;
    }
    q->buffer    = (uint8_t *)storage;
    q->item_size = item_size;
    q->capacity  = capacity;
    q->head      = 0U;
    q->tail      = 0U;
    q->count     = 0U;
    return MAIA_OK;
}

maia_status_t maia_queue_send(maia_queue_t *q, const void *item) {
    if (q == NULL || item == NULL || q->buffer == NULL) {
        return MAIA_ERR_INVALID;
    }
    if (q->count >= q->capacity) {
        return MAIA_ERR_FULL;
    }
    copy_bytes(&q->buffer[q->tail * q->item_size], (const uint8_t *)item, q->item_size);
    q->tail = (q->tail + 1U) % q->capacity;
    q->count += 1U;
    return MAIA_OK;
}

maia_status_t maia_queue_recv(maia_queue_t *q, void *out_item) {
    if (q == NULL || out_item == NULL || q->buffer == NULL) {
        return MAIA_ERR_INVALID;
    }
    if (q->count == 0U) {
        return MAIA_ERR_EMPTY;
    }
    copy_bytes((uint8_t *)out_item, &q->buffer[q->head * q->item_size], q->item_size);
    q->head = (q->head + 1U) % q->capacity;
    q->count -= 1U;
    return MAIA_OK;
}

size_t maia_queue_count(const maia_queue_t *q) {
    if (q == NULL) {
        return 0U;
    }
    return q->count;
}

